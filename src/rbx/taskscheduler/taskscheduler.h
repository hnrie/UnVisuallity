//
// Created by savage on 17.04.2025.
//

#pragma once

#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include "lua.h"

class taskscheduler {
    std::queue<std::string> execution_requests;
    std::queue<std::function<void()>> yielding_requests;
public:
    // BASIC TASKSCHEDULER STUFF
    uintptr_t get_job_by_name(std::string);

    // MORE "ADVANCED" STUFF
    uintptr_t get_script_context();
    lua_State* get_roblox_state();

    // FPS SHI
    void set_fps(double fps);
    double get_fps();

    // TASKSCHEDULER HOOK
    int scheduler_hook();

    void initialize_hook();
    void queue_script(const std::string&);
    void queue_yield(const std::function<void()>&);
};

inline const auto g_taskscheduler = std::make_unique<taskscheduler>();