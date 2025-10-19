//
// Created by user on 21/04/2025.
//

#pragma once
#include <filesystem>

class communication {
public:
    void start();
};

inline const auto g_communication = std::make_unique<communication>();