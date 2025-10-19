//
// Created by user on 16/05/2025.
//

#include "teleport_handler.h"

#include <thread>
#include <Windows.h>
#include <string>

#include "globals.h"
#include "lualib.h"
#include "lstate.h"
#include "src/core/environment/environment.h"
#include "src/core/execution/execution.h"
#include "src/rbx/engine/game.h"
#include "src/rbx/taskscheduler/taskscheduler.h"

extern bool is_valid_ptr__(void* ptr, size_t size);

std::uintptr_t teleport_handler::get_datamodel() {
        __try {
                const uintptr_t roblox_base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
                const uintptr_t data_model_fake = *reinterpret_cast<uintptr_t*>(roblox_base + rbx::rvas::data_model::get);
                if (!data_model_fake) return NULL;

                auto data_model = reinterpret_cast<uintptr_t*>(data_model_fake + 0x1B8);

                if (!data_model_fake || !is_valid_ptr__(data_model, sizeof(uintptr_t))) {
                        return 0;
                }

                uintptr_t *place_id_ptr = reinterpret_cast<uintptr_t*>(*data_model + 0x198);
                if (!place_id_ptr || !is_valid_ptr__(place_id_ptr, sizeof(uintptr_t))) {
                        return 0;
                }

                if (*place_id_ptr == 0) { g_environment->reset_drawing_lib(); return 0; } // BUNS HOMEPAGE BUNS

                uintptr_t *game_loaded_ptr = reinterpret_cast<uintptr_t*>(*data_model + 0x650);
                if (!game_loaded_ptr || !is_valid_ptr__(game_loaded_ptr, sizeof(uintptr_t))) {
                        return 0;
                }

                if (*game_loaded_ptr != 31) return 0; // BUNS HOMEPAGE BUNS


                return *data_model;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
                return 0;
        }
}

[[noreturn]] void teleport_handler::initialize()
{
        std::thread([&]
        {
                uintptr_t last_data_model = get_datamodel();
                while (true)
                {
                        uintptr_t current_data_model = get_datamodel();

                        if (current_data_model && last_data_model != current_data_model)
                        {
                                last_data_model = current_data_model;


                                std::thread([&]
                                {
                                        uintptr_t script_context = g_taskscheduler->get_script_context(); // ptrs are guarenteed to be valid
                                        if (!script_context) return;
globals::script_context = script_context;
//rbx::standard_out::printf(rbx::message_type::message_info, "script_context: %p", script_context);

globals::datamodel = last_data_model;
//rbx::standard_out::printf(rbx::message_type::message_info, "datamodel: %p", datamodel);


lua_State* roblox_state = g_taskscheduler->get_roblox_state();
globals::roblox_state = roblox_state;


lua_State* our_state = lua_newthread(roblox_state);
lua_ref(roblox_state, -1);
globals::our_state = our_state;
lua_pop(roblox_state, 1);

luaL_sandboxthread(our_state);


our_state->userdata->identity = 8;
our_state->userdata->capabilities = g_execution->capabilities;

g_taskscheduler->initialize_hook();


g_environment->reset();
environment::initialize(our_state);
                                }).detach();
                        }

                        std::this_thread::sleep_for(std::chrono::seconds(2));
                }
        }).detach();
}
