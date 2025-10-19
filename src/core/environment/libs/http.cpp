//
// Created by savage on 18.04.2025.
//

#include "../environment.h"
#include "curl_wrapper.h"
#include "http_status.h"
#include "hex.h"
#include "blake3.h"
#include "src/rbx/engine/game.h"
#include "src/rbx/taskscheduler/yielding.h"

__forceinline void Blake3Hash(_Out_ std::string &out, std::size_t dataSize,
                              const std::vector<char> &bytes
) {
    // Initialize the hasher.
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);

    blake3_hasher_update(&hasher, bytes.data(), bytes.size());

    out.clear();
    out.resize(dataSize);

    blake3_hasher_finalize(&hasher, reinterpret_cast<std::uint8_t *>(out.data()), dataSize);
};

__forceinline std::optional<const std::string> get_hwid() {
    HW_PROFILE_INFO profile_info;
    if (!GetCurrentHwProfileA(&profile_info)) {
        return {};
    }

    std::string hash_digest;
    Blake3Hash(hash_digest, 25, // long ahh hwid
               std::vector<char>{
                       profile_info.szHwProfileGuid,
                       profile_info.szHwProfileGuid + sizeof(profile_info.szHwProfileGuid)
               }
    );

    std::string output{};
    CryptoPP::HexEncoder encoder;
    encoder.Attach(new CryptoPP::StringSink(output));
    encoder.Put(reinterpret_cast<std::uint8_t *>(hash_digest.data()), hash_digest.size());
    encoder.MessageEnd();


    return output;
}

int request(lua_State* L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, OBF("Url"));
    if (lua_isnoneornil(L, -1))
        luaL_argerror(L, 1, OBF("'Url' field is missing"));

    std::string url = lua_tostring(L, -1);
    lua_pop(L, 1);

    std::string method = OBF("get");

    lua_getfield(L, 1, OBF("Method"));
    if (!lua_isnoneornil(L, -1))
        method = lua_tostring(L, -1);

    lua_pop(L, 1);

    std::transform(method.begin(), method.end(), method.begin(), tolower);

    if (method != OBF("get") && method != OBF("post") && method != OBF("put") && method != OBF("head") & method != OBF("delete"))
        luaL_argerror(L, 1, OBF("'Method' field is invalid"));

    const auto possible_hwid = get_hwid();
    if (!possible_hwid.has_value())
        luaL_error(L, OBF("Cannot compute Hardware ID. Win32 error"));

    CHeaders headers;
    headers.Insert(OBF("User-Agent"), OBF("Visual"));
    headers.Insert(OBF("Visual-Fingerprint"), possible_hwid.value());
    headers.Insert(OBF("Visual-User-Identifier"), possible_hwid.value());

    lua_getfield(L, 1, OBF("Headers"));
    if (!lua_isnil(L, -1)) {
        if (!lua_istable(L, -1)) {
            luaL_argerrorL(L, 1, OBF("'Headers' field is not a table"));
        }

        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            if (!lua_isstring(L, -2) || !lua_isstring(L, -1)) {
                luaL_argerrorL(L, 1, OBF("'Headers' field contains an invalid header pair"));
            }

            headers.Insert(_strdup(lua_tostring(L, -2)), _strdup(lua_tostring(L, -1)));

            lua_pop(L, 1);
        }

        lua_pop(L, 1);
    }

    CCookies cookies;

    lua_getfield(L, 1, OBF("Cookies"));
    if (!lua_isnil(L, -1)) {
        if (!lua_istable(L, -1)) {
            luaL_argerrorL(L, 1, OBF("'Cookies' field is not a table"));
        }
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            if (!lua_isstring(L, -2) || !lua_isstring(L, -1)) {
                luaL_argerrorL(L, 1, OBF("'Cookies' field contains an invalid cookie pair"));
            }

            cookies.Insert(_strdup(lua_tostring(L, -1)));
            lua_pop(L, 1);
        }

        lua_pop(L, 1);
    }

    std::string body;
    lua_getfield(L, 1, OBF("Body"));
    if (!lua_isnil(L, -1)) {
        if (!lua_isstring(L, -1))
            luaL_argerrorL(L, 1, OBF("'Body' field is not a valid string"));

        body = luaL_checkstring(L, -1);
        lua_pop(L, 1);
    }

    return g_yielding->yield(L, [url, method, body, cookies, headers]() -> yielded_func_t {
        CResponse response;

        if (method == OBF("get"))
            response = g_curl_wrapper->GET(url, cookies, headers);
        else if (method == OBF("post")) {
            response = g_curl_wrapper->POST_NOT_JSON(url, body, cookies, headers);
        } else if (method == OBF("head"))
            response = g_curl_wrapper->HEAD(url, cookies, headers);
        else if (method == OBF("put"))
            response = g_curl_wrapper->PUT(url, cookies, headers);
        else if (method == OBF("delete"))
            response = g_curl_wrapper->DELETE(url, cookies, headers);
        else
            response.StatusCode = -1;

        if (response.StatusCode == -1)
            throw std::runtime_error(OBF("something failed blatantly wrong!"));

        return [response](lua_State *L) -> int {
            lua_newtable(L);
            lua_pushlstring(L, response.Res.c_str(), response.Res.size());
            lua_setfield(L, -2, OBF("Body"));

            lua_pushinteger(L, response.StatusCode);
            lua_setfield(L, -2, OBF("StatusCode"));

            lua_pushlstring(L, response.StatusMessage.c_str(), response.StatusMessage.size());
            lua_setfield(L, -2, OBF("StatusMessage"));

            lua_pushboolean(L, !HttpStatus::IsError(response.StatusCode));
            lua_setfield(L, -2, OBF("Success"));

            lua_newtable(L);
            for (const auto& header : response.Headers) {
                size_t pos = header.find(':');
                if (pos != std::string::npos) {
                    std::string key = header.substr(0, pos);
                    std::string value = header.substr(pos + 2);
                    lua_pushlstring(L, value.c_str(), value.size());
                    lua_setfield(L, -2, key.c_str());
                }
            }
            lua_setfield(L, -2, OBF("Headers"));

            return 1;
        };
    });
}

std::map<std::string_view, std::string_view> http_cache = { };

int environment::http_get(lua_State *L) {
    lua_check(L, 3);
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TSTRING);//luaL_argexpected(L, lua_isstring(L, 2), 2, OBF("string"));

    auto url = std::string(lua_tolstring(L, 2, nullptr));
    bool cache = luaL_optboolean(L, 3, false);

    if (url.find(OBF("http://")) != 0 && url.find(OBF("https://")) != 0)
        luaL_argerror(L, 2, OBF("invalid protocol specified ('http://' or 'https://' expected)"));

    if (http_cache.contains(url))
    {
        std::string_view response = http_cache.at(url);

        lua_pushlstring(L, response.data(), response.size());
        return 1;
    }

    CHeaders headers;
    headers.Insert(OBF("User-Agent"), OBF("Roblox/WinInet"));

    return g_yielding->yield(L, [url, headers, cache]() -> yielded_func_t {
        CResponse response = g_curl_wrapper->GET(url, {}, headers);

        if (cache)
            http_cache[url] = response.Res;

        return [response](lua_State *L) -> int {
            lua_pushlstring(L, response.Res.data(), response.Res.size());
            return 1;
        };
    });
}

void environment::load_http_lib(lua_State *L) {
    static const luaL_Reg http[] = {
        {"request", request},
        {"http_request", request},
        {"httpget", http_get},

        {nullptr, nullptr}
    };

    lua_pushcclosure(L,request, nullptr, 0);
    lua_setglobal(L, "request");
    lua_pushcclosure(L,request, nullptr, 0);
    lua_setglobal(L, "http_request");

    lua_pushcclosure(L,http_get, nullptr, 0);
    lua_setglobal(L, "httpget");

    lua_newtable(L);
    lua_pushcclosurek(L, request, nullptr, 0, nullptr);
    lua_setfield(L, -2, OBF("request"));
    lua_setglobal(L, "http");
}
