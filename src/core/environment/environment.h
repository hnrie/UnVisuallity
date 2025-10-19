//
// Created by savage on 18.04.2025.
//

#pragma once

#include <lua.h>
#include <lualib.h>
#include <queue>
#include <string>
#include <unordered_set>
#include <xorstr/xorstr.h>
#include <lazy_importer/include/lazy_importer.hpp>
#include <optional>
#include <unordered_map>

class environment {
public:
    static void initialize(lua_State *L);
    void reset();

    std::unordered_map<std::uintptr_t, std::unordered_map<std::string, bool>> scriptable_map;
    static std::vector<std::string> teleport_queue;
    //std::optional<bool> get_scriptable_state(uintptr_t instance, const std::string &property_name);
    //void change_scriptable_state(uintptr_t instance, const std::string &property_name, bool state);

    // LIBS
    void load_http_lib(lua_State *L);
    void load_closure_lib(lua_State *L);
    void reset_closure_lib();
    void load_misc_lib(lua_State *L);
    void load_scripts_lib(lua_State *L);
    void reset_scripts_lib();
    void load_crypt_lib(lua_State *L);
    void load_cache_lib(lua_State *L);
    void load_metatables_lib(lua_State *L);
    void load_filesystem_lib(lua_State *L);
    void load_debug_lib(lua_State *L);
    void load_drawing_lib(lua_State *L);
    void reset_drawing_lib();
    void load_input_lib(lua_State *L);
    void load_actor_lib(lua_State *L);
    void load_raknet_lib(lua_State *L);
    void load_signals_lib(lua_State *L);
    void reset_signals_lib();
    void load_console_lib(lua_State* L);

    // FUNCS FOR GAME HOOK
    static int http_get(lua_State* L);
    static int get_objects(lua_State* L);

    void load_websockets_lib(lua_State *L);
    void reset_websocket_lib();
};

inline const auto g_environment = std::make_unique<environment>();