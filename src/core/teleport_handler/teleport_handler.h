//
// Created by user on 16/05/2025.
//
#pragma once
#include <memory>

class teleport_handler {
public:
    std::uintptr_t get_datamodel();
    void initialize();
};

inline const auto g_teleport_handler = std::make_unique<teleport_handler>();