#pragma once

#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include "lua.h"

/**
 * @brief Wraps Roblox's task scheduler to execute custom scripts.
 */
class taskscheduler {
    std::queue<std::string> execution_requests;
    std::queue<std::function<void()>> yielding_requests;
public:
    /**
     * @brief Retrieves a scheduler job pointer by its name.
     *
     * @param name Name of the Roblox job.
     * @return uintptr_t Native pointer to the job, or 0 if missing.
     */
    uintptr_t get_job_by_name(std::string name);

    /**
     * @brief Returns the ScriptContext pointer used for execution.
     *
     * @return uintptr_t Pointer to Roblox's ScriptContext.
     */
    uintptr_t get_script_context();

    /**
     * @brief Obtains the active Roblox Lua state.
     *
     * @return lua_State* Roblox thread used for script execution.
     */
    lua_State* get_roblox_state();

    /**
     * @brief Configures the scheduler's desired frame rate.
     *
     * @param fps Target frames per second.
     */
    void set_fps(double fps);

    /**
     * @brief Reads the current frame rate configuration.
     *
     * @return double Target frames per second.
     */
    double get_fps();

    /**
     * @brief Hook invoked from Roblox's scheduler each tick.
     *
     * @return int Result propagated back to Roblox's scheduler.
     */
    int scheduler_hook();

    /**
     * @brief Installs the scheduler hook trampoline.
     */
    void initialize_hook();

    /**
     * @brief Queues Luau code for execution on the next scheduler tick.
     *
     * @param chunk Script contents to run.
     */
    void queue_script(const std::string& chunk);

    /**
     * @brief Queues a C++ callback that should be executed on the scheduler thread.
     *
     * @param callback Function invoked during the next tick.
     */
    void queue_yield(const std::function<void()>& callback);
};

namespace scheduler_global {
    /**
     * @brief Global accessor for the Roblox task scheduler wrapper.
     */
    inline const auto instance = std::make_unique<taskscheduler>();
}
