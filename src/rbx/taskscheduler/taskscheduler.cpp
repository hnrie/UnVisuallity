//
// Created by savage on 17.04.2025.
//

#include "taskscheduler.h"

#include <mutex>
#include <regex>

#include "globals.h"
#include "yielding.h"
#include "../engine/game.h"
#include "src/core/execution/execution.h"
#include "xorstr/xorstr.h"
#include "lazy_importer/include/lazy_importer.hpp"
#include "pattern_scanner/lxzp_scanner.hpp"

std::mutex script_queue_mutex{};
std::mutex yield_queue_mutex{};

bool is_valid_ptr__(void* ptr, size_t size) {
    if (!ptr) return false;

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0)
        return false;

    uintptr_t start = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t end = start + size;
    uintptr_t region_start = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
    uintptr_t region_end = region_start + mbi.RegionSize;

    return (start >= region_start && end <= region_end) &&
        (mbi.State == MEM_COMMIT) &&
        ((mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) != 0);
}

uintptr_t taskscheduler::get_job_by_name(std::string job_name) {

        uintptr_t base = (uintptr_t)GetModuleHandleA(OBF("RobloxPlayerBeta.exe"));
        uintptr_t taskscheduler = *reinterpret_cast<uintptr_t*>(base + rbx::rvas::taskscheduler::taskschedulermk2);
        if (!taskscheduler)
            taskscheduler = *reinterpret_cast<uintptr_t*>(base + rbx::rvas::taskscheduler::taskschedulermk22);

        uintptr_t jobstart = *(uintptr_t*)(taskscheduler + rbx::offsets::taskscheduler::job_start);
        uintptr_t jobend = *(uintptr_t*)(taskscheduler + rbx::offsets::taskscheduler::job_end);

    for (uintptr_t job_ptr = jobstart; job_ptr != jobend; job_ptr += 16Ui64) {
        uintptr_t* job = reinterpret_cast<uintptr_t*>(job_ptr);
        if (!job || !is_valid_ptr__(job, sizeof(uintptr_t))) continue;

        std::string curr_job_name = *reinterpret_cast<std::string*>(*job + rbx::offsets::taskscheduler::job_name);
        if (curr_job_name.empty()) continue;

        if (curr_job_name == job_name)
        {
            return *job;
        }
    }

        return 0;
}

uintptr_t taskscheduler::get_script_context() {
 uintptr_t base = (uintptr_t)GetModuleHandleA(0);

     uintptr_t taskscheduler = *reinterpret_cast<uintptr_t*>(base + rbx::rvas::taskscheduler::taskschedulermk2);
     if (!taskscheduler)
         taskscheduler = *reinterpret_cast<uintptr_t*>(base + rbx::rvas::taskscheduler::taskschedulermk22);

     uintptr_t luagc = 0;
     uintptr_t jobstart = *(uintptr_t*)(taskscheduler + rbx::offsets::taskscheduler::job_start);
     uintptr_t jobend = *(uintptr_t*)(taskscheduler + rbx::offsets::taskscheduler::job_end);

     while (jobstart != jobend) {
         std::string job_name = *(std::string*)(*(uintptr_t*)(jobstart)+rbx::offsets::taskscheduler::job_name);

         if (*(std::string*)(*(uintptr_t*)(jobstart)+rbx::offsets::taskscheduler::job_name) == OBF(rbx::job_names::waiting_hybrid_scripts_job)) {

             // We do this so we won't get the Homepage WaitingHybridScriptsJob :P
             uintptr_t job = *(uintptr_t*)jobstart;
             uintptr_t script_context_ptr = *(uintptr_t*)(job + rbx::offsets::luagc::script_context);
             uintptr_t datamodel = *(uintptr_t*)(script_context_ptr + rbx::offsets::instance::parent);
             uintptr_t datamodel_name = *(uintptr_t*)(datamodel + rbx::offsets::instance::name);
             std::string datamodel_name_t = *(std::string*)(datamodel_name);

             if (datamodel_name_t != OBF(rbx::job_names::lua_app)) {
                 luagc = *reinterpret_cast<uintptr_t*>(jobstart);
                 break;
             }
         }
         jobstart += 0x10;
     }


    if (!luagc)
        return 0;//LI_FN(MessageBoxA).safe()(nullptr, OBF("Failed to initialize (1)"), OBF("Visual"), MB_OK);

    uintptr_t script_context_ptr = *reinterpret_cast<uintptr_t*>(luagc + rbx::offsets::luagc::script_context);
    if (!script_context_ptr)
        return 0;

    return script_context_ptr;
}

lua_State* taskscheduler::get_roblox_state() {
    uintptr_t script_context = get_script_context();
    if (!script_context)
        return nullptr;

    *reinterpret_cast<BOOLEAN*>(script_context + rbx::offsets::script_context::require_check) = TRUE;

    auto Address = script_context + rbx::offsets::script_context::encrypted_state;
    auto EncryptedState = reinterpret_cast<uint32_t*>(Address);

    uint32_t Low = EncryptedState[0] ^ static_cast<uint32_t>(Address);
    uint32_t High = EncryptedState[1] ^ static_cast<uint32_t>(Address);
    return reinterpret_cast<lua_State*>((static_cast<uint64_t>(High) << 32) | Low);
}

void taskscheduler::set_fps(double fps) {
    uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleA(0));
    uintptr_t taskscheduler = *reinterpret_cast<uintptr_t*>(base + rbx::rvas::taskscheduler::taskschedulermk2);
    if (!taskscheduler)
        taskscheduler = *reinterpret_cast<uintptr_t*>(base + rbx::rvas::taskscheduler::taskschedulermk22);

    static const double min_frame_delay = 1.0 / 10000.0;
    double frame_delay = fps <= 0.0 ? min_frame_delay : 1.0 / fps;

    *reinterpret_cast<double*>(taskscheduler + rbx::offsets::taskscheduler::max_fps) = frame_delay;
}

double taskscheduler::get_fps() {
    uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleA(0));
    uintptr_t taskscheduler = *reinterpret_cast<uintptr_t*>(base + rbx::rvas::taskscheduler::taskschedulermk2);
    if (!taskscheduler)
        taskscheduler = *reinterpret_cast<uintptr_t*>(base + rbx::rvas::taskscheduler::taskschedulermk22);

    return 1.0 / *reinterpret_cast<double*>(taskscheduler + rbx::offsets::taskscheduler::max_fps);
}

int taskscheduler::scheduler_hook() {
    if (!execution_requests.empty()) {
        const std::string top = execution_requests.front();
        execution_requests.pop();

        g_execution->run_code(globals::our_state, top);
    }
    if (!yielding_requests.empty())
    {
        std::function<void()> yielding_request = yielding_requests.front();
        yielding_requests.pop();

        yielding_request( );
    }

    return 0;
}

using vt_function_t = void*(__fastcall*)(void*, void*, void*);
vt_function_t original_step = nullptr;

std::mutex step_mutex{};

void* scheduler_hook_1(void *a1, void *a2, void *a3) {
    if (!step_mutex.try_lock())
        return original_step(a1, a2, a3);

    g_taskscheduler->scheduler_hook();

    step_mutex.unlock();
    return original_step(a1, a2, a3);
}

void taskscheduler::initialize_hook() {
    uintptr_t base = (uintptr_t)GetModuleHandleA(0);
    uintptr_t taskscheduler = *reinterpret_cast<uintptr_t*>(base + rbx::rvas::taskscheduler::taskschedulermk2);
    if (!taskscheduler)
        taskscheduler = *reinterpret_cast<uintptr_t*>(base + rbx::rvas::taskscheduler::taskschedulermk22);

    uintptr_t waiting_hybrid_scripts_job = 0;
    uintptr_t jobstart = *(uintptr_t*)(taskscheduler + rbx::offsets::taskscheduler::job_start);
    uintptr_t jobend = *(uintptr_t*)(taskscheduler + rbx::offsets::taskscheduler::job_end);

    while (jobstart != jobend) {
        std::string job_name = *(std::string*)(*(uintptr_t*)(jobstart)+rbx::offsets::taskscheduler::job_name);

        if (*(std::string*)(*(uintptr_t*)(jobstart)+rbx::offsets::taskscheduler::job_name) == OBF("WaitingHybridScriptsJob")) {

            // We do this so we won't get the Homepage WaitingHybridScriptsJob :P
            uintptr_t job = *(uintptr_t*)jobstart;
            uintptr_t script_context_ptr = *(uintptr_t*)(job + rbx::offsets::luagc::script_context);
            uintptr_t datamodel = *(uintptr_t*)(script_context_ptr + rbx::offsets::instance::parent);
            uintptr_t datamodel_name = *(uintptr_t*)(datamodel + rbx::offsets::instance::name);
            std::string datamodel_name_t = *(std::string*)(datamodel_name);

             if (datamodel_name_t == OBF(rbx::job_names::ugc)) {
                waiting_hybrid_scripts_job = *reinterpret_cast<uintptr_t*>(jobstart);
                break;
            }
        }
        jobstart += 0x10;
    }

    if (!waiting_hybrid_scripts_job)
        LI_FN(MessageBoxA).safe()(nullptr, OBF("Failed to synchronize"), OBF("Visual"), MB_OK);

    // This is a fragile way to hook the scheduler. If the VTable changes, this will break.
    constexpr int SCHEDULER_HOOK_INDEX = 6;
    constexpr int VFTABLE_HOOK_SIZE = 55;

    if (waiting_hybrid_scripts_job) {
        void **_vtable = new void*[VFTABLE_HOOK_SIZE + 1];
        memcpy(_vtable, *reinterpret_cast<void**>(waiting_hybrid_scripts_job), sizeof(uintptr_t) * VFTABLE_HOOK_SIZE);
        original_step = reinterpret_cast<vt_function_t>(_vtable[SCHEDULER_HOOK_INDEX]);
        _vtable[SCHEDULER_HOOK_INDEX] = scheduler_hook_1;
        *reinterpret_cast< void** >(waiting_hybrid_scripts_job) = _vtable;
    }

}

void taskscheduler::queue_script(const std::string& script) {
    std::unique_lock ul(script_queue_mutex);

    execution_requests.emplace(script);
}

void taskscheduler::queue_yield(const std::function<void()>& callback) {
    std::unique_lock ul(yield_queue_mutex);

    yielding_requests.emplace(callback);
}
