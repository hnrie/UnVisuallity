//
// Created by user on 16/05/2025.
//
#pragma once
#include <memory>

/**
 * @brief Tracks Roblox teleport events and exposes the new DataModel pointer.
 */
class teleport_handler {
public:
    /**
     * @brief Returns the current DataModel pointer after teleportation.
     *
     * @return std::uintptr_t Native pointer to the Roblox DataModel instance.
     */
    std::uintptr_t get_datamodel();

    /**
     * @brief Hooks Roblox teleport events and begins monitoring.
     */
    void initialize();
};

/**
 * @brief Global accessor for the teleport handler.
 */
inline const auto g_teleport_handler = std::make_unique<teleport_handler>();