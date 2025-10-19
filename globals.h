//
// Created by savage on 17.04.2025.
//

#pragma once

#include <Windows.h>
#include <filesystem>
#include <unordered_set>

#include "lua.h"

struct Closure;

namespace globals {
    inline bool initialized;
    inline bool authenticated;
    inline HMODULE dll_module;
    inline std::filesystem::path workspace_path;
    inline std::filesystem::path autoexec_path;
    inline std::filesystem::path bin_path;


    inline uintptr_t datamodel;
    inline uintptr_t script_context;


    inline lua_State* roblox_state;
    inline lua_State* our_state;

    inline std::unordered_set<Closure*> closure_set;
}
