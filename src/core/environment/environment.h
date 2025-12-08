#pragma once

#include <string>
#include <vector>
#include <map>
#include <Windows.h>
#include <memory>
#include <optional>

#include "lua.h"

/**
 * @brief Manages the Luau execution environment and library loading.
 */
class environment {
public:
    std::map<uintptr_t, std::map<std::string, bool>> scriptable_map;
    static std::vector<std::string> teleport_queue;

    /**
     * @brief Loads the custom closure library into the Lua state.
     *
     * @param L Lua state to load the library into.
     */
    void load_closure_lib(lua_State* L);

    /**
     * @brief Resets the closure library state (e.g., clears closure maps).
     */
    void reset_closure_lib();

    /**
     * @brief Loads the debug library into the Lua state.
     *
     * @param L Lua state to load the library into.
     */
    void load_debug_lib(lua_State* L);

    /**
     * @brief Loads the custom drawing library into the Lua state.
     *
     * @param L Lua state to load the library into.
     */
    void load_drawing_lib(lua_State* L);

    /**
     * @brief Resets drawing library resources.
     */
    void reset_drawing_lib();

    /**
     * @brief Loads file system utilities into the Lua state.
     *
     * @param L Lua state to load the library into.
     */
    void load_filesystem_lib(lua_State* L);

    /**
     * @brief Loads HTTP utilities (e.g., request) into the Lua state.
     *
     * @param L Lua state to load the library into.
     */
    void load_http_lib(lua_State* L);

    /**
     * @brief Implements HttpGet for retrieving content from URLs.
     *
     * @param L Lua state invoking the function.
     * @return int Number of return values.
     */
    static int http_get(lua_State* L);

    /**
     * @brief Implements GetObjects for retrieving assets.
     *
     * @param L Lua state invoking the function.
     * @return int Number of return values.
     */
    static int get_objects(lua_State* L);

    /**
     * @brief Loads input handling utilities into the Lua state.
     *
     * @param L Lua state to load the library into.
     */
    void load_input_lib(lua_State* L);

    /**
     * @brief Loads metatable manipulation utilities into the Lua state.
     *
     * @param L Lua state to load the library into.
     */
    void load_metatables_lib(lua_State* L);

    /**
     * @brief Loads miscellaneous utilities into the Lua state.
     *
     * @param L Lua state to load the library into.
     */
    void load_misc_lib(lua_State* L);

    /**
     * @brief Loads script manipulation utilities into the Lua state.
     *
     * @param L Lua state to load the library into.
     */
    void load_scripts_lib(lua_State* L);

    /**
     * @brief Resets script library state.
     */
    void reset_scripts_lib();

    /**
     * @brief Loads signaling utilities into the Lua state.
     *
     * @param L Lua state to load the library into.
     */
    void load_signals_lib(lua_State* L);

    /**
     * @brief Resets signals library state.
     */
    void reset_signals_lib();

    /**
     * @brief Loads caching utilities into the Lua state.
     *
     * @param L Lua state to load the library into.
     */
    void load_cache_lib(lua_State* L);

    /**
     * @brief Loads cryptography utilities into the Lua state.
     *
     * @param L Lua state to load the library into.
     */
    void load_crypt_lib(lua_State* L);

    /**
     * @brief Loads WebSocket utilities into the Lua state.
     *
     * @param L Lua state to load the library into.
     */
    void load_websockets_lib(lua_State* L);

    /**
     * @brief Resets WebSocket library state.
     */
    void reset_websocket_lib();

    /**
     * @brief Loads console interaction utilities into the Lua state.
     *
     * @param L Lua state to load the library into.
     */
    void load_console_lib(lua_State* L);

    /**
     * @brief Loads actor manipulation utilities into the Lua state.
     *
     * @param L Lua state to load the library into.
     */
    void load_actor_lib(lua_State* L);

    /**
     * @brief Initializes the environment by setting up hooks and loading libraries.
     *
     * @param L Lua state to initialize.
     */
    static void initialize(lua_State* L);

    /**
     * @brief Resets the entire environment state.
     */
    void reset();
};

namespace environment_global {
    /**
     * @brief Shared environment instance.
     */
    inline const auto instance = std::make_unique<environment>();
}
