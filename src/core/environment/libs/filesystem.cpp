//
// Created by reveny on 21/04/2025.
//
#include <fstream>
#include "globals.h"
#include "lapi.h"
#include "lgc.h"
#include "lobject.h"
#include "lstate.h"
#include "../environment.h"
#include "src/core/execution/execution.h"

std::filesystem::path get_given_path(lua_State* L, const bool malicious_check=true) {
    std::string given_path = luaL_checklstring(L, 1, nullptr);

    std::ranges::replace(given_path, '/', '\\');

    if (given_path.find(OBF("..")) != std::string::npos)
        luaL_argerrorL(L, 1, OBF("directory escape"));

    if (given_path.find(OBF("C:")) != std::string::npos)
        luaL_argerror(L, 1, OBF("C Drive ..."));

    const auto actual_path = globals::workspace_path / given_path;


    const std::vector<std::string> blacklisted_extensions =
    {
        OBF(".exe"), OBF(".com"), OBF(".bat"), OBF(".cmd"), OBF(".vbs"),
        OBF(".vbe"), OBF(".js"), OBF(".jse"), OBF(".wsf"), OBF(".wsh"),
        OBF(".ps1"), OBF(".ps1_sys"), OBF(".ps2"), OBF(".ps2_sys"), OBF(".ps3"),
        OBF(".ps3_sys"), OBF(".ps4"), OBF(".ps4_sys"), OBF(".ps5"), OBF(".ps5_sys"),
        OBF(".ps6"), OBF(".ps6_sys"), OBF(".ps7"), OBF(".ps7_sys"), OBF(".ps8"),
        OBF(".ps8_sys"), OBF(".psm1"), OBF(".psm1_sys"), OBF(".psd1"), OBF(".psd1_sys"),
        OBF(".psh1"), OBF(".psh1_sys"), OBF(".msc"), OBF(".msc_sys"), OBF(".msh"),
        OBF(".msh_sys"), OBF(".msh1"), OBF(".msh1_sys"), OBF(".msh2"), OBF(".msh2_sys"),
        OBF(".mshxml"), OBF(".mshxml_sys"), OBF(".vshost"), OBF(".vshost_sys"), OBF(".vbscript"),
        OBF(".vbscript_sys"), OBF(".wsf"), OBF(".wsf_sys"), OBF(".wsh"), OBF(".wsh_sys"),
        OBF(".wsh1"), OBF(".wsh1_sys"), OBF(".wsh2"), OBF(".wsh2_sys"), OBF(".wshxml"),
        OBF(".wshxml_sys"),

        OBF(".ps1"), OBF(".ps1_sys"), OBF(".ps2"), OBF(".ps2_sys"),
        OBF(".ps3"), OBF(".ps3_sys"), OBF(".ps4"), OBF(".ps4_sys"), OBF(".ps5"),
        OBF(".ps5_sys"), OBF(".ps6"), OBF(".ps6_sys"), OBF(".ps7"), OBF(".ps7_sys"),
        OBF(".ps8"), OBF(".ps8_sys"), OBF(".psm1"), OBF(".psm1_sys"), OBF(".psd1"),
        OBF(".psd1_sys"), OBF(".psh1"), OBF(".psh1_sys"), OBF(".msc"), OBF(".msc_sys"),
        OBF(".msh"), OBF(".msh_sys"), OBF(".msh1"), OBF(".msh1_sys"), OBF(".msh2"),
        OBF(".msh2_sys"), OBF(".mshxml"), OBF(".mshxml_sys"), OBF(".vshost"), OBF(".vshost_sys"),
        OBF(".vbscript"), OBF(".vbscript_sys"), OBF(".wsf"), OBF(".wsf_sys"), OBF(".wsh"), OBF(".wsh_sys"),
    };

    if (malicious_check) {
        for (const std::string& ext : blacklisted_extensions) {
            if (actual_path.extension().string() == ext) {
                luaL_argerrorL(L, 1, OBF("blacklisted extension"));
            }
        }
    }

    return actual_path;
}

int writefile(lua_State *L) {
    lua_check(L, 2);
    luaL_checktype(L, 1, LUA_TSTRING);
    luaL_checktype(L, 2, LUA_TSTRING);

    const auto path = get_given_path(L);

    size_t contents_sz;
    const char* contents_cstr = luaL_checklstring(L, 2, &contents_sz);
    std::string contents = std::string(contents_cstr, contents_sz);

    std::ofstream file_stream{};
    file_stream.open(path, std::ios::out | std::ios::binary);

    file_stream.write(contents.data(), contents.size());
    file_stream.close();

    return 0;
}

int readfile(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TSTRING);

    const auto path = get_given_path(L, false);
    if (!std::filesystem::exists(path))
        luaL_argerror(L, 1, OBF("file does not exist"));

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        luaL_errorL(L, OBF("failed to open file for reading"));

    auto file_sz = file.seekg(0, std::ios::end).tellg();
    file.seekg(0);

    std::string file_contents{};
    file_contents.resize(file_sz);
    file.read(file_contents.data(), file_sz);

    lua_pushlstring(L, file_contents.data(), file_contents.size());
    return 1;
}

int listfiles(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TSTRING);

    const auto path = get_given_path(L, false);

    if (!std::filesystem::exists(path))
        luaL_argerrorL(L, 1, OBF("path does not exist"));

    lua_newtable(L);

    for (int i = 0; const auto& file_entry: std::filesystem::directory_iterator(path)) {
        std::string file_path = file_entry.path().string().substr(globals::workspace_path.string().size() + 1);
        lua_pushstring(L, file_path.c_str());
        lua_rawseti(L, -2, ++i);
    }

    return 1;
}

int isfile(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TSTRING);

    const auto path = get_given_path(L, false);

    lua_pushboolean(L, std::filesystem::is_regular_file(path));
    return 1;
}

int isfolder(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TSTRING);

    const auto path = get_given_path(L, false);

    lua_pushboolean(L, std::filesystem::is_directory(path));
    return 1;
}

int appendfile(lua_State *L) {
    lua_check(L, 2);
    luaL_checktype(L, 1, LUA_TSTRING);
    luaL_checktype(L, 2, LUA_TSTRING);

    const auto path = get_given_path(L);
    if (!std::filesystem::exists(path))
        luaL_argerror(L, 1, OBF("file does not exist"));

    const auto contents = luaL_tolstring(L, 2, nullptr);

    std::ofstream file_stream{};
    file_stream.open(path, std::ios::app | std::ios::binary);

    if (!file_stream.is_open())
        luaL_error(L, OBF("failed to open file for writing"));

    file_stream << contents;

    file_stream.close();

    return 0;
}

int delfile(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TSTRING);

    const auto path = get_given_path(L, false);
    if (!std::filesystem::exists(path))
        luaL_argerror(L, 1, OBF("file does not exist"));

    std::filesystem::remove(path);
    return 0;
}

int delfolder(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TSTRING);

    const auto path = get_given_path(L, false);
    if (!std::filesystem::exists(path))
        luaL_argerror(L, 1, OBF("folder does not exist"));

    std::filesystem::remove_all(path);
    return 0;
}

int makefolder(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TSTRING);
    const auto path = get_given_path(L, false);

    std::filesystem::create_directories(path);
    return 0;
}

extern std::uintptr_t max_caps;
extern void set_closure_capabilities(Proto *proto, uintptr_t* caps);

int loadfile(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TSTRING);

    const auto path = get_given_path(L);
    if (!std::filesystem::exists(path))
        luaL_argerror(L, 1, OBF("file does not exist"));

    std::ifstream file(path, std::ios::out | std::ios::binary);
    if (!file.is_open())
        luaL_errorL(L, OBF("failed to open file for reading"));

    auto file_sz = file.seekg(0, std::ios::end).tellg();
    file.seekg(0);

    std::string file_contents{};
    file_contents.resize(file_sz);
    file.read(file_contents.data(), file_sz);

    std::string bytecode = g_execution->compile(file_contents);
    if (rbx::luau::vm_load(L, &bytecode, OBF("loadfile"), 0) != LUA_OK) {
        lua_pushnil(L);
        lua_pushvalue(L, -2);
        return 2;
    }

    Closure* closure = clvalue(luaA_toobject(L, -1));
    set_closure_capabilities(closure->l.p, &max_caps);

    lua_setsafeenv(L, LUA_GLOBALSINDEX, false);

    return 1;
}

int dofile(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TSTRING);

    const auto path = get_given_path(L);
    if (!std::filesystem::exists(path))
        luaL_argerror(L, 1, OBF("file does not exist"));

    std::ifstream file(path, std::ios::out | std::ios::binary);
    if (!file.is_open())
        luaL_errorL(L, OBF("failed to open file for reading"));

    auto file_sz = file.seekg(0, std::ios::end).tellg();
    file.seekg(0);

    std::string file_contents{};
    file_contents.resize(file_sz);
    file.read(file_contents.data(), file_sz);

    std::string bytecode = g_execution->compile(file_contents);
    if (rbx::luau::vm_load(L, &bytecode, OBF("dofile"), 0) != LUA_OK) {
        lua_pushnil(L);
        lua_pushvalue(L, -2);
        return 2;
    }

    Closure* closure = clvalue(luaA_toobject(L, -1));
    set_closure_capabilities(closure->l.p, &max_caps);

    lua_setsafeenv(L, LUA_GLOBALSINDEX, false);

    lua_getglobal(L, OBF("task"));
    lua_getfield(L, -1, OBF("defer"));
    lua_pushthread(L);
    lua_call(L, 1, 0);

    return 0;
}

int getcustomasset(lua_State* L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TSTRING);

    const auto path = get_given_path(L);
    if (!std::filesystem::exists(path))
        luaL_argerror(L, 1, OBF("file does not exist"));
    bool no_cache = luaL_optboolean(L, 2, true);

    const auto content = std::filesystem::current_path() / OBF("content") / path.filename().string();

    if (!std::filesystem::exists(content.parent_path()))
        std::filesystem::create_directories(content.parent_path());

    if (no_cache) {
        if (std::filesystem::exists(content)) /* no cache, so much to do now */
            std::filesystem::remove(content);

        std::filesystem::copy(path, content, std::filesystem::copy_options::recursive);
    }

    lua_pushstring(L,  std::format("rbxasset://{}", path.filename().string()).c_str());
    return 1;
}

void environment::load_filesystem_lib(lua_State *L) {
    std::filesystem::path workspace_folder = globals::bin_path.parent_path() / OBF("workspace");
    if (!std::filesystem::exists(workspace_folder))
        std::filesystem::create_directories(workspace_folder);
    ///rbx::standard_out::printf(1, "path set to: %s", workspace_folder.string().c_str());
    globals::workspace_path = workspace_folder;

    lua_pushcclosure(L, writefile, nullptr, 0);
    lua_setglobal(L, "writefile");

    lua_pushcclosure(L, readfile, nullptr, 0);
    lua_setglobal(L, "readfile");

    lua_pushcclosure(L, listfiles, nullptr, 0);
    lua_setglobal(L, "listfiles");

    lua_pushcclosure(L, isfile, nullptr, 0);
    lua_setglobal(L, "isfile");

    lua_pushcclosure(L, isfolder, nullptr, 0);
    lua_setglobal(L, "isfolder");

    lua_pushcclosure(L, appendfile, nullptr, 0);
    lua_setglobal(L, "appendfile");

    lua_pushcclosure(L, delfile, nullptr, 0);
    lua_setglobal(L, "delfile");

    lua_pushcclosure(L, delfolder, nullptr, 0);
    lua_setglobal(L, "delfolder");

    lua_pushcclosure(L, makefolder, nullptr, 0);
    lua_setglobal(L, "makefolder");

    lua_pushcclosure(L, loadfile, nullptr, 0);
    lua_setglobal(L, "loadfile");

    lua_pushcclosure(L, dofile, nullptr, 0);
    lua_setglobal(L, "dofile");

    lua_pushcclosure(L, getcustomasset, nullptr, 0);
    lua_setglobal(L, "getcustomasset");
}
