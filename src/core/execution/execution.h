#pragma once

#include <string>
#include <vector>
#include <memory>

#include "lua.h"
#include "lstate.h"

/**
 * @brief Manages script execution, compilation, and environment setup.
 */
class execution {
public:
    static uintptr_t capabilities;

    /**
     * @brief Compiles Luau source code into bytecode.
     *
     * @param code Source code string.
     * @return std::string Compiled bytecode.
     */
    static std::string compile(const std::string& code);

    /**
     * @brief Loads and executes a script string in the given Lua state.
     *
     * @param L Lua state to execute in.
     * @param chunk_name Name of the script chunk (for debug info).
     * @param code Source code to execute.
     * @return int Status code of the execution.
     */
    int load_string(lua_State* L, const std::string& chunk_name, const std::string& code);

    /**
     * @brief Decompresses custom compressed bytecode.
     *
     * @param source Compressed bytecode string.
     * @return std::string Decompressed bytecode.
     */
    [[nodiscard]] std::string decompress_bytecode(const std::string& source) const;

    /**
     * @brief Runs a script in a new thread within the given Lua state.
     *
     * @param state Lua state to create the thread in.
     * @param code Source code to execute.
     * @return bool True if execution started successfully.
     */
    bool run_code(lua_State* state, const std::string& code);
};

namespace execution_global {
    /**
     * @brief Shared execution instance.
     */
    inline const auto instance = std::make_unique<execution>();
}
