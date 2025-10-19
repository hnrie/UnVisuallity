//
// Created by savage on 17.04.2025.
//

#pragma once
#include "game_structures.h"

template <typename T>
inline T rebase(const uintptr_t rva) {
#ifdef _WIN32
    return rva != NULL ? (T)(rva + reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr))) : static_cast<T>((NULL));
#else
    return (T)rva;
#endif
};

namespace rbx {
    namespace rvas {

        namespace standard_out {
            inline const uintptr_t printf = 0x1833E60; // updated
        }

        namespace data_model
        {
            inline const uintptr_t get = 0x7268A88; // updated
        }

        namespace taskscheduler {
            inline const uintptr_t taskschedulermk2 = 0x75BE1B8; //
            inline const uintptr_t taskschedulermk22 = 0x67a9b98; //
        }

        namespace script_context {
            inline const uintptr_t get_global_state = 0xED2BD0; // updated
            inline const uintptr_t decrypt_state = 0x11D1A50; // updated

            inline const uintptr_t task_defer = 0x141C030; // updated

            inline const uintptr_t resume = 0x11B1930; // updated
        }

        namespace property_descriptor {
            inline const uintptr_t get_property = 0xDD6180; // updated
            inline const uintptr_t ktable = 0x6A23E80; // updated
        }

        namespace lua_bridge {
            inline const uintptr_t push = 0x122EF60; // updated
        }

        namespace identity {
            inline const uintptr_t get_identity_struct = 0x3D23FC0; // updated
            inline const uintptr_t identity_struct = 0x6304418; // updated
        }

        namespace fast_flags {
            inline const uintptr_t singleton = 0x63201A8; // updated
            inline const uintptr_t set_fast_flag = 0x3F28490; // updated
            inline const uintptr_t get_fast_flag = 0x37EE1E0; // updated

            inline const uintptr_t lock_violation_script_crash = 0x6119778; // updated
            inline const uintptr_t lock_violation_instance_crash = 0x61256A8; // updated
         }

        namespace click_detector {
            inline const uintptr_t fire = 0x1E992A0; // updated | xref: E8 ? ? ? ? 48 8B 45 ? 48 89 38 | 0
            inline const uintptr_t fire_right = 0x1E99440; // updated | 3
            inline const uintptr_t hover_enter = 0x1E9A840;// updated | 1
            inline const uintptr_t hover_leave = 0x1E9A9E0;// updated   | 2
        }

        namespace proximity_prompt {
            inline const uintptr_t fire = 0x1EF47E0; // updated
        }

        namespace base_part {
            inline const uintptr_t touch_interest = 0x21A2E80; // updated
        }

        namespace lua_arguments {
            inline constexpr uintptr_t get = 0xEE59E0; // updated  | "Argument %d missing or nil"
            inline constexpr uintptr_t variant_cast_int = 0x1810D40; //  updated | 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B 19 48 8B F9 0F 29 74 24
            inline constexpr uintptr_t variant_cast_int64 = 0x1810F50; // updated | 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B 19 48 8B F9 E8 ? ? ? ? 33 F6
            inline constexpr uintptr_t variant_cast_float = 0x1811390; // updated  | 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 83 EC ? 48 8B 19 48 8B F9 0F 29 74 24 ? E8 ? ? ? ? 48 8B 2F 45 33 F6 48 3B D8 75 ? E8 ? ? ? ? 48 3B E8 0F 85 ? ? ? ? 4C 39 77 ? 48 8D 47 ? 49 0F 44 C6 F2 0F 10 30
        }

        namespace app_data {
            inline constexpr uintptr_t singleton = 0x72687F8; // updated
        }

        namespace luau {
            inline const uintptr_t luau_execute = 0x36825D0;// updated | 80 79 ? ? 0F 85 ? ? ? ? E9
            inline const uintptr_t luah_dummynode = 0x51B4328; // updated
            inline const uintptr_t luao_nilobject = 0x51B4908; // updated
            inline const uintptr_t luac_step = 0x2729fa0; // updated

            inline const uintptr_t vm_load = 0x1366360; // updated
        }

    }

    namespace job_names {
        inline constexpr const char* waiting_hybrid_scripts_job = "WaitingHybridScriptsJob";
        inline constexpr const char* lua_app = "LuaApp";
        inline constexpr const char* ugc = "Ugc";
    }

    namespace offsets {

        namespace taskscheduler { // didnt change
            inline constexpr uintptr_t job_start = 0x1d0;
            inline constexpr uintptr_t job_end = 0x1d8;
            inline constexpr uintptr_t job_name = 0x18;

            inline constexpr uintptr_t max_fps = 0x1B0;
        }

        namespace instance { // didnt change
            inline constexpr uintptr_t class_descriptor = 0x18;
            inline constexpr uintptr_t parent = 0x50;
            inline constexpr uintptr_t name = 0x80;
            inline constexpr uintptr_t children = 0x80;
        }

        namespace luagc {
            inline constexpr uintptr_t script_context = 0x1f8; // didnt change
        }

        namespace base_part {
            inline constexpr uintptr_t primitive = 0x178;
            inline constexpr uintptr_t world = 0x1D0;
        }

        namespace app_data {
            inline constexpr uintptr_t game_status = 0x310;
        }

        namespace script_context {
            inline constexpr uintptr_t global_state = 0x140; // 
            inline constexpr uintptr_t decrypt_state = 0x88; // didnt change

            inline constexpr uintptr_t require_check = 0x870; //
            inline constexpr uintptr_t encrypted_state = 0x210;
        }

    }

    namespace standard_out {
        inline auto printf = rebase<void(__fastcall*)(int32_t, const char*, ...)>(rvas::standard_out::printf);
    }

    namespace identity {
        inline auto get_identity_struct = rebase<uintptr_t(__fastcall*)(uintptr_t)>(rvas::identity::get_identity_struct);
    }

    namespace lua_bridge {
        inline auto push_instance = rebase<void(__fastcall*)(lua_State*, std::uintptr_t)>(rvas::lua_bridge::push);
        inline auto v_push_instance = rebase<void(__fastcall*)(lua_State*, void*)>(rvas::lua_bridge::push);
        inline auto s_push_instance = rebase<void(__fastcall*)(lua_State*, std::shared_ptr<uintptr_t*>)>(rvas::lua_bridge::push);
        inline auto ss_push_instance = rebase<void(__fastcall*)(lua_State*, std::shared_ptr<uintptr_t>)>(rvas::lua_bridge::push);
    }

    namespace script_context {
        inline auto decrypt_state = rebase<lua_State*(__fastcall*)(uintptr_t)>(rvas::script_context::decrypt_state);
        inline auto get_global_state = rebase<uintptr_t(__fastcall*)(uintptr_t, uintptr_t*, uintptr_t*)>(rvas::script_context::get_global_state);

        inline auto task_defer = rebase<int(__fastcall*)(lua_State*)>(rvas::script_context::task_defer);

        inline auto resume = rebase<void(__fastcall *)(void *script_ctx, std::int64_t unk[0x2],
                                                      rbx::weak_thread_ref_t **wtr, int32_t n_ret,
                                                      bool is_error, const std::string *error_msg)>(rvas::script_context::resume);
    }

    namespace property_descriptor {
        inline auto get_property = rebase<std::uintptr_t*(__fastcall*)(uintptr_t, uintptr_t*)>(rvas::property_descriptor::get_property);
        inline const uintptr_t* ktable = rebase<uintptr_t*>(rvas::property_descriptor::ktable);
    }

    namespace click_detector {
        inline auto fire = rebase<void(__fastcall*)(uintptr_t click_detector, float distance, uintptr_t local_player)>(rvas::click_detector::fire);
        inline auto fire_right = rebase<void(__fastcall*)(uintptr_t click_detector, float distance, uintptr_t local_player)>(rvas::click_detector::fire_right);
        inline auto hover_enter = rebase<void(__fastcall*)(uintptr_t click_detector, uintptr_t local_player)>(rvas::click_detector::hover_enter);
        inline auto hover_leave = rebase<void(__fastcall*)(uintptr_t click_detector, uintptr_t local_player)>(rvas::click_detector::hover_leave);
    }

    namespace proximity_prompt {
        inline auto fire = rebase<void(__fastcall*)(uintptr_t)>(rvas::proximity_prompt::fire);
    }

    namespace base_part {
        inline auto touch_interest = rebase<uintptr_t(__fastcall*)(uintptr_t, uintptr_t, uintptr_t, int, char)>(rvas::base_part::touch_interest);
    }



    namespace lua_arguments {
        inline auto get = rebase<void(__fastcall*)(lua_State*, int, rbx::variant_t*, bool, int)>(rvas::lua_arguments::get);
        inline auto variant_cast_int = rebase<uintptr_t(__fastcall*)(void*)>(rvas::lua_arguments::variant_cast_int);
        inline auto variant_cast_int64 = rebase<uintptr_t(__fastcall*)(void*)>(rvas::lua_arguments::variant_cast_int64);
        inline auto variant_cast_float = rebase<uintptr_t(__fastcall*)(void*)>(rvas::lua_arguments::variant_cast_float);
    }

    namespace fast_flags {
        inline auto set_fast_flag = rebase<bool(__fastcall*)(uintptr_t, std::string*, std::string*, uintptr_t, int, int)>(rvas::fast_flags::set_fast_flag);
        inline auto get_fast_flag = rebase<bool(__fastcall*)(uintptr_t, std::string*, std::string*, int)>(rvas::fast_flags::get_fast_flag);
    }

    namespace luau {
        inline auto luah_dummynode = rebase<uintptr_t>(rvas::luau::luah_dummynode);
        inline auto luao_nilobject = rebase<uintptr_t>(rvas::luau::luao_nilobject);
        inline auto luau_execute = rebase<void(__fastcall*)(lua_State*)>(rvas::luau::luau_execute);
        inline auto luac_step = rebase<size_t(__fastcall*)(lua_State*, bool assist)>(rvas::luau::luac_step);
        inline auto vm_load = rebase<int(__fastcall*)(lua_State*, void*, const char*, int)>(rvas::luau::vm_load);
    }

}