//
// Created by user on 11/05/2025.
//

#include <map>

#include "globals.h"
#include "lapi.h"
#include "lgc.h"
#include "lobject.h"
#include "lstate.h"
#include "ltable.h"
#include "../environment.h"
#include "src/core/execution/execution.h"

int isparallel(lua_State *L)
{
    lua_check(L, 0);
    const auto userdata = reinterpret_cast<uintptr_t>(L->userdata);
    if (!userdata) {
        lua_pushboolean(L, false);
        return 1;
    }
    if (!L->userdata->actor.expired()) {
        lua_pushboolean(L, true);
        return 1;
    }

    lua_getglobal(L, OBF("game"));
    uintptr_t data_model = *static_cast<uintptr_t*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    BYTE parallel_value = *reinterpret_cast<BYTE*>(data_model + 0x110);
    rbx::standard_out::printf(1, "data_model: 0x%llx\n", data_model);

    lua_pushboolean(L, parallel_value == 1);
    return 1;
}

int getactors(lua_State *L)
{
    lua_check(L, 0);
    int idx = 0;
    lua_newtable(L);

    auto script_context = reinterpret_cast<uintptr_t>(L->userdata->SharedExtraSpace->ScriptContext);

    auto& actor_list = *reinterpret_cast<std::unordered_map<std::weak_ptr<uintptr_t*>, uintptr_t>*>(script_context + 0x140 + 0x318);
    for (auto & it : actor_list) {
        if (it.first.expired())
            continue;

        const auto act = it.first.lock();



        lua_pushinteger(L, ++idx);
        rbx::lua_bridge::s_push_instance(L, act);
        lua_settable(L, -3);
    }

    return 1;
}

int getactorthreads(lua_State* L)
{
    lua_check(L, 0);
    int idx = 0;
    lua_newtable(L);

    const auto script_context = reinterpret_cast<uintptr_t>(L->userdata->SharedExtraSpace->ScriptContext);
    const auto& actor_list = *reinterpret_cast<std::unordered_map<std::weak_ptr<uintptr_t*>, uintptr_t>*>(script_context + 0x140 + 0x318);

    for (auto &it : actor_list) {
        if (it.first.expired())
            continue;

        const uintptr_t actor_states = *reinterpret_cast<uintptr_t*>(script_context + 0x140 + 0x300);
        lua_State* thread = rbx::script_context::decrypt_state(*reinterpret_cast<uintptr_t*>(actor_states + 8 * idx) + 0x88);

        lua_rawcheckstack(L, 2);
        luaC_threadbarrier(L);

        lua_pushinteger(L, ++idx);

        L->top->value.gc = reinterpret_cast<GCObject*>(thread);
        L->top->tt = LUA_TTHREAD;;
        L->top++;

        lua_settable(L, -3);
    }

    return 1;
};

std::map<std::string, int> communication_channels = { };

std::string gen_comm_channel_id(const int len) {
    static constexpr char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)]; // NOLINT(*-msc50-cpp)
    }

    return tmp_s;
}

int create_comm_channel(lua_State *L) {
    lua_check(L, 0);

    lua_getglobal(L, OBF("Instance"));
    lua_getfield(L, -1, OBF("new"));
    lua_pushstring(L, OBF("BindableEvent"));

    lua_pcall(L, 1, 1, 0);
    const int channel_ref = lua_ref(L, -1);

    const std::string comm_channel_id = gen_comm_channel_id(16);

    communication_channels[comm_channel_id] = channel_ref;

    lua_pop(L, lua_gettop(L));

    lua_pushstring(L, comm_channel_id.c_str());
    lua_getref(L, channel_ref);
    return 2;
}
int get_comm_channel(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TSTRING);

    const std::string comm_channel_id = lua_tolstring(L, 1, nullptr);

    if (!communication_channels.contains(comm_channel_id))
        lua_pushnil(L);
    else {
        luaC_threadbarrier(L);

        lua_rawcheckstack(L, 1);
        lua_getref(globals::our_state, communication_channels[comm_channel_id]);
        lua_xmove(globals::our_state, L, 1);
        lua_rawgetfield(L, LUA_REGISTRYINDEX, OBF("Instance"));
        lua_setmetatable(L, -2);
    }

    return 1;
}

int run_on_actor(lua_State *L) {
    lua_check(L, 2);
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TSTRING);

    const std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    lua_getfield(L, 1, OBF("ClassName"));
    const auto class_name = std::string_view(lua_tolstring(L, -1, nullptr));
    lua_pop(L, 1);

    if (class_name != OBF("Actor"))
        luaL_argerrorL(L, 1, OBF("Actor expected"));

    const uintptr_t actor = *static_cast<uintptr_t*>(lua_touserdata(L, 1));
    const std::string code = lua_tostring(L, 2);

    lua_remove(L, 2);
    lua_remove(L, 1);
    const int arguments = lua_gettop(L) ;

    lua_State *actor_state = nullptr;

    const auto script_context = reinterpret_cast<uintptr_t>(L->userdata->SharedExtraSpace->ScriptContext);
    const auto& actor_list = *reinterpret_cast<std::unordered_map<std::weak_ptr<uintptr_t*>, uintptr_t>*>(script_context + 0x140 + 0x318);
    const auto& actor_states = *reinterpret_cast<uintptr_t*>(script_context + 0x140 + 0x300);

    int idx = 0;
    for (auto &it : actor_list)
    {
        if (it.first.expired())
            continue;

        lua_State* thread = rbx::script_context::decrypt_state(*reinterpret_cast<uintptr_t*>(actor_states + 8 * idx++) + 0x88);

        if (thread->userdata == nullptr || thread->userdata->actor.expired())
            continue;

        if (reinterpret_cast<uintptr_t>(thread->userdata->actor.lock().get()) == actor) {
            actor_state = thread;
            break;
        }
    }

    if (!actor_state) {
        lua_getglobal(L, OBF("Instance"));
        lua_getfield(L, -1, OBF("new"));
        lua_pushstring(L, OBF("LocalScript"));

        lua_call(L, 1, 1);


        uintptr_t new_script = *static_cast<uintptr_t*>(lua_touserdata(L, -1));
        uintptr_t identity = 2;

        *reinterpret_cast<std::uintptr_t*>(new_script + rbx::offsets::instance::parent) = actor;


        const uintptr_t encrypted_state = rbx::script_context::get_global_state(script_context + 0x140, &identity, &new_script);

        actor_state = rbx::script_context::decrypt_state(encrypted_state + 0x88);

        if(actor_state->tt != LUA_TTHREAD)
            luaL_errorL(L, OBF("failed to get actor state: 2"));

      /*
        *  lua_getfield(L, -1, OBF("Destroy"));
        lua_pushvalue(L, -2);
        lua_call(L, 1, 0);
        lua_settop(L, 0);
       */
    }


    const auto custom_actor_State = lua_newthread(actor_state);
    const auto custom_actor_state_ref = lua_ref(actor_state, -1);
    lua_pop(actor_state, 1);

    *reinterpret_cast<std::uintptr_t*>(reinterpret_cast<std::uintptr_t>(custom_actor_State->userdata) + 0x58) = actor;
    *reinterpret_cast<std::uintptr_t*>(reinterpret_cast<std::uintptr_t>(custom_actor_State->userdata) + 0x50) = 0;

    custom_actor_State->userdata->identity = 8;
    custom_actor_State->userdata->capabilities = g_execution->capabilities;

    lua_newtable(custom_actor_State);
    lua_setglobal(custom_actor_State, OBF("shared"));

    lua_newtable(custom_actor_State);
    lua_setglobal(custom_actor_State, OBF("_G"));


    g_environment->load_misc_lib(custom_actor_State);
    g_environment->load_http_lib(custom_actor_State);
    g_environment->load_closure_lib(custom_actor_State);
    g_environment->load_debug_lib(custom_actor_State);
    g_environment->load_filesystem_lib(custom_actor_State);
    g_environment->load_scripts_lib(custom_actor_State);
    g_environment->load_cache_lib(custom_actor_State);
    g_environment->load_crypt_lib(custom_actor_State);
    g_environment->load_metatables_lib(custom_actor_State);
    g_environment->load_signals_lib(custom_actor_State);
    g_environment->load_input_lib(custom_actor_State);
    g_environment->load_drawing_lib(custom_actor_State);
    g_environment->load_misc_lib(custom_actor_State);
    g_environment->load_console_lib(custom_actor_State);
    g_environment->load_actor_lib(custom_actor_State);
    g_environment->load_websockets_lib(custom_actor_State);

    const int count = g_execution->load_string(custom_actor_State, OBF("@actor"), code);
    if (count == 2) {
        const char *err = lua_tostring(custom_actor_State, -1);
        lua_pop(custom_actor_State, 2);

        luaL_errorL(L, OBF("failed to run code on actor: %s"), err);
    }

    for (int i = 1; i <= arguments; i++) {
        lua_pushvalue(L, i);
        lua_xmove(L, custom_actor_State, 1);
    }

    std::int64_t status[0x2]{0, 0};

    auto wtr = new rbx::weak_thread_ref_t();
    wtr->thread = custom_actor_State;
    wtr->thread_ref = custom_actor_state_ref;


    rbx::script_context::resume(reinterpret_cast<void*>(script_context + 0x610), status, &wtr, arguments, false, nullptr);
    return 0;
}

int run_on_thread(lua_State *L) {
    lua_check(L, 2);
    luaL_checktype(L, 1, LUA_TTHREAD);
    luaL_checktype(L, 2, LUA_TSTRING);

    lua_State *thread = lua_tothread(L, 1);
    const std::string code = lua_tostring(L, 2);
    const auto script_context = reinterpret_cast<uintptr_t>(L->userdata->SharedExtraSpace->ScriptContext);

    lua_remove(L, 2);
    lua_remove(L, 1);
    const int arguments = lua_gettop(L);

    lua_pushthread(thread);
    const auto state_ref = lua_ref(thread, -1);

    thread->userdata->identity = 8;
    thread->userdata->capabilities = g_execution->capabilities;

    lua_newtable(thread);
    lua_setglobal(thread, OBF("shared"));

    lua_newtable(thread);
    lua_setglobal(thread, OBF("_G"));


    g_environment->load_misc_lib(thread);
    g_environment->load_http_lib(thread);
    g_environment->load_closure_lib(thread);
    g_environment->load_debug_lib(thread);
    g_environment->load_filesystem_lib(thread);
    g_environment->load_scripts_lib(thread);
    g_environment->load_cache_lib(thread);
    g_environment->load_crypt_lib(thread);
    g_environment->load_metatables_lib(thread);
    g_environment->load_signals_lib(thread);
    g_environment->load_input_lib(thread);
    g_environment->load_drawing_lib(thread);
    g_environment->load_misc_lib(thread);
    g_environment->load_console_lib(thread);
    g_environment->load_actor_lib(thread);
    g_environment->load_websockets_lib(thread);

    const int count = g_execution->load_string(thread, OBF("@thread"), code);
    if (count == 2) {
        const char *err = lua_tostring(thread, -1);
        lua_pop(thread, 2);

        luaL_errorL(L, OBF("failed to run code on thread: %s"), err);
    }

    for (int i = 1; i <= arguments; i++) {
        lua_pushvalue(L, i);
        lua_xmove(L, thread, 1);
    }

    std::int64_t status[0x2]{0, 0};

    auto wtr = new rbx::weak_thread_ref_t();
    wtr->thread = thread;
    wtr->thread_ref = state_ref;

    rbx::script_context::resume(reinterpret_cast<void*>(script_context + 0x610), status, &wtr, arguments, false, nullptr);
    return 0;
}


void environment::load_actor_lib(lua_State *L) {
    lua_pushcclosure(L, isparallel, nullptr, 0);
    lua_setglobal(L, "isparallel");
    lua_pushcclosure(L, isparallel, nullptr, 0);
    lua_setglobal(L, "checkparallel");

    lua_pushcclosure(L, getactors, nullptr, 0);
    lua_setglobal(L, "getactors");

    lua_pushcclosure(L, getactorthreads, nullptr, 0);
    lua_setglobal(L, "getactorthreads");

    lua_pushcclosure(L, create_comm_channel, nullptr, 0);
    lua_setglobal(L, "create_comm_channel");
    lua_pushcclosure(L, get_comm_channel, nullptr, 0);
    lua_setglobal(L, "get_comm_channel");

    lua_pushcclosure(L, run_on_actor, nullptr, 0);
    lua_setglobal(L, "run_on_actor");
    lua_pushcclosure(L, run_on_actor, nullptr, 0);
    lua_setglobal(L, "runonactor");

    lua_pushcclosure(L, run_on_thread, nullptr, 0);
    lua_setglobal(L, "run_on_thread");
    lua_pushcclosure(L, run_on_thread, nullptr, 0);
    lua_setglobal(L, "runonthread");
}