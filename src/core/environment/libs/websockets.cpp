//
// Created by user on 03/05/2025.
//

#include "globals.h"
#include "lapi.h"
#include "lgc.h"
#include "lobject.h"
#include "lstate.h"
#include "ltable.h"
#include "../environment.h"
#include "src/rbx/engine/game.h"
#include "lmem.h"
#include "websockets.h"

#include <map>
#include <thread>

std::unordered_map<void*, std::shared_ptr<websocket_object>> websockets = { };

int websocket_send(lua_State *L) {
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TSTRING);

    void *userdata = lua_touserdata(L, 1);
    std::string message = lua_tostring(L, 2);

    if (!websockets.contains(userdata))
        luaL_errorL(L, OBF("failed to find websocket object"));

    std::shared_ptr<websocket_object> websocket = websockets[userdata];

    if (websocket->closed)
        return 0;
    //rbx::standard_out::printf(1, "status: %d", websocket->websocket_client.getReadyState());
    websocket->send_message(message);
    return 0;
}

int websocket_close(lua_State *L) {
    luaL_checktype(L, 1, LUA_TUSERDATA);

    void *userdata = lua_touserdata(L, 1);

    if (!websockets.contains(userdata))
        luaL_errorL(L, OBF("failed to find websocket object"));

    std::shared_ptr<websocket_object> websocket = websockets[userdata];

    if (websocket->closed)
        return 0;

    //rbx::standard_out::printf(1, "status: %d", websocket->websocket_client.getReadyState());
    websocket->websocket_client.close();

    websocket->closed = true;

    websockets.erase(userdata);

    lua_resetthread(globals::our_state);
    lua_getref(websocket->L, websocket->on_close_ref);
    if (lua_isnoneornil(websocket->L, -1)) {
        lua_pop(websocket->L, 1);

        luaL_error(L, OBF("something failed blatantly wrong!"));
    }
    lua_getfield(websocket->L, -1, "Fire");
    if (!lua_isnoneornil(websocket->L, -1)) {
        lua_insert(websocket->L, -2);
        if (lua_pcall(websocket->L, 1, 0 ,0)) {
            const auto error = lua_tostring(websocket->L, -1);
            lua_pop(websocket->L, 1);
        }
    }
    lua_settop(L, 0);


    lua_unref(websocket->L, websocket->on_message_ref);
    lua_unref(websocket->L, websocket->on_close_ref);
    lua_unref(L, websocket->L_ref);

    return 0;
}

int websocket_index(lua_State *L) {
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TSTRING);

    void *userdata = lua_touserdata(L, 1);
    const std::string key = lua_tostring(L, 2);

    if (!websockets.contains(userdata))
        luaL_errorL(L, OBF("failed to find websocket object"));

    std::shared_ptr<websocket_object> websocket = websockets[userdata];

    if (websocket->closed)
        luaL_error(L, OBF("websocket is closed!"));

    if (!websocket->on_message_ref || !websocket->on_close_ref)
        luaL_error(L, OBF("websocket is closed!"));

    // EVENTS
    if (key == OBF("OnMessage")) {
        lua_getref(L, websocket->on_message_ref);
        lua_getfield(L, -1, OBF("Event"));
        return 1;
    } else if (key == OBF("OnClose")) {
        lua_getref(L, websocket->on_close_ref);
        lua_getfield(L, -1, OBF("Event"));
        return 1;
    }
    // METHODS
    else if (key == OBF("Send") || key == OBF("send")) {
        lua_pushcclosurek(L, websocket_send, nullptr, 0, nullptr);
        return 1;
    } else if (key == OBF("Close") || key == OBF("close")) {
        lua_pushcclosurek(L, websocket_close, nullptr, 0, nullptr);
        return 1;
    }

    return 0;
}

int websocket_connect(lua_State *L) {
    lua_check(L, 1);

    luaL_checktype(L, 1, LUA_TSTRING);

    const std::string url = lua_tostring(L, 1);

    if (!url.contains(OBF("ws://")) && !url.contains(OBF("wss://")) || (url == OBF("ws://") || url == OBF("wss://")))
        luaL_argerror(L, 1, OBF("invalid protocol specified ('ws://' or 'wss://' expected)"));

    lua_getglobal(L, OBF("Instance"));
    lua_getfield(L, -1, OBF("new"));
    //lua_remove(L, -2);
    lua_pushstring(L, OBF("BindableEvent"));
    lua_call(L, 1, 1);

    if (lua_isnoneornil(L, -1))
        luaL_errorL(L, OBF("failed to create first reference"));

    int on_message = lua_ref(L, -1);
    lua_pop(L, 2);

    lua_getglobal(L, OBF("Instance"));
    lua_getfield(L, -1, OBF("new"));
    //lua_remove(L, -2);
    lua_pushstring(L, OBF("BindableEvent"));
    lua_call(L, 1, 1);

    if (lua_isnoneornil(L, -1))
        luaL_errorL(L, OBF("failed to create second reference"));

    int on_close = lua_ref(L, -1);
    lua_pop(L, 2);

    std::shared_ptr<websocket_object> websocket = std::make_shared<websocket_object>();
    bool success = websocket->initialize(url, on_message, on_close);


    if (!success)
        luaL_error(L, OBF("failed to connect to %s"), url.c_str());

    websocket->L = lua_newthread(L);
    websocket->L_ref = lua_ref(L, -1);
    lua_pop(L, 1);

    const auto userdata = lua_newuserdata(L, sizeof(void*));
    websockets[userdata] = websocket;

    lua_newtable(L);

    // Metamethods of our Metatable
    lua_pushstring(L, OBF("WebsocketObject"));
    lua_setfield(L, -2, OBF("__type"));

    lua_pushcclosurek(L, websocket_index, nullptr, 0, nullptr);
    lua_setfield(L, -2, OBF("__index"));

    lua_setmetatable(L, -2);

    return 1;
}

void environment::load_websockets_lib(lua_State *L) {
    //ix::initNetSystem();

    lua_newtable(L);

    lua_pushcclosure(L, websocket_connect, nullptr, 0);
    lua_setfield(L, -2, "connect");

    lua_pushcclosure(L, websocket_connect, nullptr, 0);
    lua_setfield(L, -2, "Connect");

    lua_setglobal(L, "WebSocket");
}

bool websocket_object::initialize(const std::string& _url, int ref_1, int ref_2) {
    this->on_message_ref = ref_1;
    this->on_close_ref = ref_2;
//rbx::standard_out::printf(1, "url: %s", _url.c_str());
    this->websocket_client.setUrl(_url);

    this->websocket_client.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        //auto str_msg = std::string(msg->str.begin(), msg->str.end());
        switch (msg->type) {
            //rbx::standard_out::printf(1, "ud");
            case ix::WebSocketMessageType::Message:
                //rbx::standard_out::printf(1, "got message");
                if (!this->on_message_ref || this->closed)
                    return;

                lua_resetthread(globals::our_state);
                lua_getref(L, this->on_message_ref);
                if (lua_isnoneornil(L, -1)) {
                    lua_pop(L, 1);

                    luaL_error(L, OBF("something failed blatantly wrong!"));
                }
                lua_getfield(L, -1, "Fire");
                if (!lua_isnoneornil(L, -1)) {
                    lua_insert(L, -2);
                    lua_pushlstring(L, msg->str.data(), msg->str.size());
                    if (lua_pcall(L, 2, 0, 0)) {
                        const auto error = lua_tostring(L, -1);
                        lua_pop(L, 1);
                    }
                }
                lua_settop(L, 0);

                break;

            case ix::WebSocketMessageType::Error:
                //rbx::standard_out::printf(1, "error: %s", msg->errorInfo.reason.c_str());
                break;
            default:
                break;
        }
    });

   ix::WebSocketInitResult result = this->websocket_client.connect(5);

   if (!result.success) {
      // rbx::standard_out::printf(1, "error: %s", result.errorStr.c_str());
       return false;
   }

    this->websocket_client.start();

    return true;
}

void environment::reset_websocket_lib()
{
    for (const auto& websocket : websockets)
    {
        websocket.second->stop();
    }

    websockets.clear();
}