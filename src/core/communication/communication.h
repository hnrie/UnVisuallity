//
// Created by user on 21/04/2025.
//

#pragma once
#include <filesystem>

/**
 * @brief Manages the networking channel used to communicate with the host application.
 */
class communication {
public:
    /**
     * @brief Starts the background communication loop.
     */
    void start();
};

/**
 * @brief Global accessor for the communication subsystem.
 */
inline const auto g_communication = std::make_unique<communication>();