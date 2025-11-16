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
#ifdef _WIN32
#include <lazy_importer/include/lazy_importer.hpp>
#endif
#include <optional>
#include <unordered_map>

/**
 * @brief Exposes helper libraries and utilities to the embedded Lua runtime.
 */
class environment {
public:
    /**
     * @brief Initializes the global environment and registers default libraries.
     *
     * @param L Target Lua state to seed with the environment helpers.
     */
    static void initialize(lua_State *L);

    /**
     * @brief Resets transient state after teleportation or shutdown.
     */
    void reset();

    std::unordered_map<std::uintptr_t, std::unordered_map<std::string, bool>> scriptable_map;
    static std::vector<std::string> teleport_queue;

    /**
     * @brief Registers the HTTP helper library with Roblox's Lua state.
     *
     * @param L Lua state receiving the HTTP bindings.
     */
    void load_http_lib(lua_State *L);

    /**
     * @brief Registers the closure helper library for upvalue management.
     *
     * @param L Lua state receiving the closure helpers.
     */
    void load_closure_lib(lua_State *L);

    /**
     * @brief Clears any state maintained by the closure helper library.
     */
    void reset_closure_lib();

    /**
     * @brief Registers miscellaneous helper functions.
     *
     * @param L Lua state receiving the helpers.
     */
    void load_misc_lib(lua_State *L);

    /**
     * @brief Registers the script management library.
     *
     * @param L Lua state receiving the helpers.
     */
    void load_scripts_lib(lua_State *L);

    /**
     * @brief Clears any state stored by the script management library.
     */
    void reset_scripts_lib();

    /**
     * @brief Registers the cryptographic helper functions.
     *
     * @param L Lua state receiving the crypt helpers.
     */
    void load_crypt_lib(lua_State *L);

    /**
     * @brief Registers the cache helper library.
     *
     * @param L Lua state receiving the cache helpers.
     */
    void load_cache_lib(lua_State *L);

    /**
     * @brief Registers helper functions for metatables.
     *
     * @param L Lua state receiving the metatable helpers.
     */
    void load_metatables_lib(lua_State *L);

    /**
     * @brief Registers filesystem helper functions.
     *
     * @param L Lua state receiving the filesystem helpers.
     */
    void load_filesystem_lib(lua_State *L);

    /**
     * @brief Registers debugging helper functions.
     *
     * @param L Lua state receiving the debugging helpers.
     */
    void load_debug_lib(lua_State *L);

    /**
     * @brief Registers drawing helper functions.
     *
     * @param L Lua state receiving the drawing helpers.
     */
    void load_drawing_lib(lua_State *L);

    /**
     * @brief Clears any state stored by the drawing library.
     */
    void reset_drawing_lib();

    /**
     * @brief Registers input helper functions.
     *
     * @param L Lua state receiving the input helpers.
     */
    void load_input_lib(lua_State *L);

    /**
     * @brief Registers actor helper functions for concurrent scripts.
     *
     * @param L Lua state receiving the actor helpers.
     */
    void load_actor_lib(lua_State *L);

    /**
     * @brief Registers RakNet helper functions for networking features.
     *
     * @param L Lua state receiving the RakNet helpers.
     */
    void load_raknet_lib(lua_State *L);

    /**
     * @brief Registers signal helper functions.
     *
     * @param L Lua state receiving the signal helpers.
     */
    void load_signals_lib(lua_State *L);

    /**
     * @brief Clears any state stored by the signal helper library.
     */
    void reset_signals_lib();

    /**
     * @brief Registers console helper functions.
     *
     * @param L Lua state receiving the console helpers.
     */
    void load_console_lib(lua_State* L);

    /**
     * @brief Implements the HTTP GET helper exposed to Lua scripts.
     *
     * @param L Lua state issuing the HTTP request.
     * @return int Number of return values pushed onto the stack.
     */
    static int http_get(lua_State* L);

    /**
     * @brief Enumerates Roblox objects from Lua scripts.
     *
     * @param L Lua state requesting the objects.
     * @return int Number of return values pushed onto the stack.
     */
    static int get_objects(lua_State* L);

    /**
     * @brief Registers websocket helper functions.
     *
     * @param L Lua state receiving the websocket helpers.
     */
    void load_websockets_lib(lua_State *L);

    /**
     * @brief Clears any state stored by the websocket helper library.
     */
    void reset_websocket_lib();
};

/**
 * @brief Global accessor for the environment subsystem.
 */
inline const auto g_environment = std::make_unique<environment>();