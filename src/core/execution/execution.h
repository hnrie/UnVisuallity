//
// Created by savage on 17.04.2025.
//

#pragma once
#include <string>
#include "lua.h"

/**
 * @brief Handles code compilation and execution inside the Roblox Lua runtime.
 */
class execution {
public:
    uintptr_t capabilities = 0xFFFFFFFFFFFFFFFF;

    /**
     * @brief Compiles Luau source code to bytecode.
     *
     * @param source Luau source code to compile.
     * @return std::string Serialized bytecode output.
     */
    std::string compile(const std::string& source);

    /**
     * @brief Decompresses serialized bytecode into plain text.
     *
     * @param Bytecode Compressed bytecode blob.
     * @return std::string Decompressed bytecode string.
     */
    std::string decompress_bytecode(const std::string &Bytecode) const;

    /**
     * @brief Loads a script into the provided Lua state without executing it.
     *
     * @param L Lua state receiving the chunk.
     * @param chunk Script contents.
     * @param chunk_name Friendly name for debugging.
     * @return int Result code from lua_load.
     */
    int load_string(lua_State* L, const std::string& chunk, const std::string& chunk_name);

    /**
     * @brief Compiles and executes a script immediately.
     *
     * @param L Lua state used for execution.
     * @param chunk Script contents to run.
     * @return bool True when execution succeeded.
     */
    bool run_code(lua_State* L, const std::string& chunk);
};

/**
 * @brief Global accessor for the execution subsystem.
 */
inline const auto g_execution = std::make_unique<execution>();