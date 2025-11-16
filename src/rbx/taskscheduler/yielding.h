//
// Created by savage on 17.04.2025.
//

#pragma once

#include <functional>
#include <memory>
#include "lua.h"

using yielded_func_t = std::function<int(lua_State *)>;

/**
 * @brief Helper for yielding Luau threads and resuming them later.
 */
class yielding {
public:
    /**
     * @brief Suspends a Lua thread and schedules a continuation.
     *
     * @param thread Thread to yield.
     * @param func Callback returning the function executed when resuming.
     * @return int Result propagated back to Roblox once resumed.
     */
    int yield(lua_State *thread, const std::function<yielded_func_t()>& func) const;
};

/**
 * @brief Global accessor for the yielding helper.
 */
inline const auto g_yielding = std::make_unique<yielding>();