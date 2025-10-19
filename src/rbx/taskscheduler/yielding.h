//
// Created by savage on 17.04.2025.
//

#pragma once

#include <functional>
#include <memory>
#include "lua.h"

using yielded_func_t = std::function<int(lua_State *)>;

class yielding {
public:
    int yield(lua_State *thread, const std::function<yielded_func_t()>& func) const;
};

inline const auto g_yielding = std::make_unique<yielding>();