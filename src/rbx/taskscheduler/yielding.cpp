//
// Created by user on 14/05/2025.
//

#include "yielding.h"
#include <thread>

#include "ldo.h"
#include "rng.h"
#include "taskscheduler.h"
#include "xorstr/xorstr.h"

int yielding::yield(lua_State *thread, const std::function<yielded_func_t()>& func) const
{
    lua_pushthread(thread);
    const auto yielded_thread_ref = lua_ref(thread, -1); // don't gc me thxx
    const auto second_ref = lua_ref(thread, -1);
    lua_pop(thread, 1);

    std::thread([=, this]
                {
                    yielded_func_t resume_func;

                    try
                    {
                        resume_func = func();
                    }
                    catch (const std::exception& ex)
                    {
                        rbx::standard_out::printf(1, OBF("an error occured while yielding!"));
                        lua_unref(thread, yielded_thread_ref);
                        lua_unref(thread, second_ref);
                        return;
                    }

                    g_taskscheduler->queue_yield([=]() -> void
                                    {

                                        int res_count = 0;
                                        void* script_ctx = thread->userdata->SharedExtraSpace->ScriptContext;
                                        const auto roblox_base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));


                                        const BYTE old = *reinterpret_cast<BYTE*>(roblox_base + rbx::rvas::fast_flags::lock_violation_script_crash);
                                        const BYTE old1 = *reinterpret_cast<BYTE*>(roblox_base + rbx::rvas::fast_flags::lock_violation_instance_crash);

                                        try
                                        {
                                            res_count = resume_func(thread);
                                        }
                                        catch (lua_exception ex) {
                                            lua_pushstring(thread, ex.what());

                                            std::int64_t status[0x2]{0, 0};

                                            auto wtr = new rbx::weak_thread_ref_t();
                                            wtr->thread = thread;
                                            wtr->thread_ref = second_ref;

                                            *reinterpret_cast<BYTE*>(roblox_base + rbx::rvas::fast_flags::lock_violation_script_crash) = 0;
                                            *reinterpret_cast<BYTE*>(roblox_base + rbx::rvas::fast_flags::lock_violation_instance_crash) = 0;
                                            
                                            const std::string err_cpy = ex.what();
                                            rbx::script_context::resume(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(script_ctx) + 0x610), status, &wtr, 0, true, &err_cpy);
                                            
                                            *reinterpret_cast<BYTE*>(roblox_base + rbx::rvas::fast_flags::lock_violation_script_crash) = old;
                                            *reinterpret_cast<BYTE*>(roblox_base + rbx::rvas::fast_flags::lock_violation_instance_crash) = old1;
                                            
                                            return;
                                        }
                                        catch (std::exception ex) {
                                           // rbx::standard_out::printf(1, "CATCH HANDLER RANNN!!!!!!!!!");
                                            lua_pushstring(thread, ex.what());

                                            std::int64_t status[0x2]{0, 0};

                                            auto wtr = new rbx::weak_thread_ref_t();
                                            wtr->thread = thread;
                                            wtr->thread_ref = second_ref;
                                            
                                            *reinterpret_cast<BYTE*>(roblox_base + rbx::rvas::fast_flags::lock_violation_script_crash) = 0;
                                            *reinterpret_cast<BYTE*>(roblox_base + rbx::rvas::fast_flags::lock_violation_instance_crash) = 0;

                                            const std::string err_cpy = ex.what();
                                            rbx::script_context::resume(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(script_ctx) + 0x610), status, &wtr, 0, true, &err_cpy);

                                            *reinterpret_cast<BYTE*>(roblox_base + rbx::rvas::fast_flags::lock_violation_script_crash) = old;
                                            *reinterpret_cast<BYTE*>(roblox_base + rbx::rvas::fast_flags::lock_violation_instance_crash) = old1;

                                            lua_unref(thread, yielded_thread_ref);
                                            lua_unref(thread, second_ref);
                                            return;
                                        }

                                        std::int64_t status[0x2]{0, 0};

                                        auto wtr = new rbx::weak_thread_ref_t();
                                        wtr->thread = thread;
                                        wtr->thread_ref = second_ref;

                                        *reinterpret_cast<BYTE*>(roblox_base + rbx::rvas::fast_flags::lock_violation_script_crash) = 0;
                                        *reinterpret_cast<BYTE*>(roblox_base + rbx::rvas::fast_flags::lock_violation_instance_crash) = 0;
                                            
                                        rbx::script_context::resume(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(script_ctx) + 0x610), status, &wtr, res_count, false, nullptr);

                                        *reinterpret_cast<BYTE*>(roblox_base + rbx::rvas::fast_flags::lock_violation_script_crash) = old;
                                        *reinterpret_cast<BYTE*>(roblox_base + rbx::rvas::fast_flags::lock_violation_instance_crash) = old1;
                        
                                        lua_unref(thread, yielded_thread_ref);
                                        lua_unref(thread, second_ref);
                                    });
                }).detach();

    return lua_yield(thread, 0);
}
