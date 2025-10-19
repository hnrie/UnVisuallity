//
// Created by savage on 17.04.2025.
//

#pragma once
#include <string>
#include "lua.h"

class execution {
public:
    uintptr_t capabilities = 0xFFFFFFFFFFFFFFFF;

    std::string compile(const std::string&);
    std::string decompress_bytecode(const std::string &Bytecode) const;

    int load_string(lua_State*, const std::string&, const std::string&);

    bool run_code(lua_State*, const std::string&);
};

inline const auto g_execution = std::make_unique<execution>();