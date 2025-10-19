//
// Created by reveny on 21/04/2025.
//
#include <unordered_map>
#include <set>
#include <array>

#include <fstream>
#include "globals.h"
#include "lapi.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"
#include "../environment.h"
#include "src/core/execution/execution.h"
#include "sha.h"
#include "filters.h"
#include "hex.h"
#include <mutex>

#include "curl_wrapper.h"
#include "src/rbx/taskscheduler/taskscheduler.h"


extern std::optional<bool> get_scriptable_state(uintptr_t instance, const std::string& property_name);

extern void change_scriptable_state(uintptr_t instance, const std::string &property_name, bool state) noexcept;

int getscriptbytecode(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TUSERDATA);

    std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    auto script = *reinterpret_cast<std::uintptr_t*>(lua_touserdata(L, 1));

    lua_getfield(L, 1, OBF("ClassName"));
    std::string_view class_name = std::string_view(lua_tolstring(L, -1, nullptr));
    lua_pop(L, 1);

    if (class_name == OBF("LocalScript") || class_name == OBF("Script")) {
        const auto protected_string = *reinterpret_cast<std::uintptr_t*>(script + 0x1B0);
        if (!protected_string || IsBadReadPtr(reinterpret_cast<const void *>(protected_string), 0x10)) { lua_pushnil(L); return 1; }

        const auto encrypted_bytecode = std::string(
                *reinterpret_cast<char **>(protected_string + 0x10),
                *reinterpret_cast<uintptr_t *>(protected_string + 0x20));

        std::string decompressed = g_execution->decompress_bytecode(encrypted_bytecode);

        lua_pushlstring(L, decompressed.data(), decompressed.size());
        return 1;
    } else if (class_name == OBF("ModuleScript")) {
        const auto protected_string = *reinterpret_cast<std::uintptr_t*>(script + 0x158);
        if (!protected_string || IsBadReadPtr(reinterpret_cast<const void *>(protected_string), 0x10)) { lua_pushnil(L); return 1; }

        const auto encrypted_bytecode = std::string(
                *reinterpret_cast<char **>(protected_string + 0x10),
                *reinterpret_cast<uintptr_t *>(protected_string + 0x20));

        std::string decompressed = g_execution->decompress_bytecode(encrypted_bytecode);

        lua_pushlstring(L, decompressed.data(), decompressed.size());
        return 1;
    } else {
        luaL_argerrorL(L, 1, OBF("invalid script type"));
    }
}

int getscripthash(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TUSERDATA);

    std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    auto script = *reinterpret_cast<std::uintptr_t*>(lua_touserdata(L, 1));

    lua_getfield(L, 1, OBF("ClassName"));
    std::string_view class_name = std::string_view(lua_tolstring(L, -1, nullptr));
    lua_pop(L, 1);

    if (class_name == OBF("LocalScript") || class_name == OBF("Script")) {
        const auto protected_string = *reinterpret_cast<std::uintptr_t*>(script + 0x1B0);
        if (!protected_string || IsBadReadPtr(reinterpret_cast<const void *>(protected_string), 0x10)) { lua_pushnil(L); return 1; }

        const auto encrypted_bytecode = std::string(
                *reinterpret_cast<char **>(protected_string + 0x10),
                *reinterpret_cast<uintptr_t *>(protected_string + 0x20));

        std::string decompressed = g_execution->decompress_bytecode(encrypted_bytecode);

        std::string digest{};
        CryptoPP::SHA384 hash;
        CryptoPP::StringSource ss(decompressed, true, new CryptoPP::HashFilter(hash, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest), false)));

        lua_pushlstring(L, digest.c_str(), digest.size());
        return 1;
    } else if (class_name == OBF("ModuleScript")) {
        const auto protected_string = *reinterpret_cast<std::uintptr_t*>(script + 0x158);
        if (!protected_string || IsBadReadPtr(reinterpret_cast<const void *>(protected_string), 0x10)) { lua_pushnil(L); return 1; }

        const auto encrypted_bytecode = std::string(
                *reinterpret_cast<char **>(protected_string + 0x10),
                *reinterpret_cast<uintptr_t *>(protected_string + 0x20));

        std::string decompressed = g_execution->decompress_bytecode(encrypted_bytecode);

        std::string digest{};
        CryptoPP::SHA384 hash;
        CryptoPP::StringSource ss(decompressed, true, new CryptoPP::HashFilter(hash, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest), false)));

        lua_pushlstring(L, digest.c_str(), digest.size());
        return 1;
    } else {
        luaL_argerrorL(L, 1, OBF("invalid script type"));
    }
}

extern void set_closure_capabilities(Proto *proto, uintptr_t* caps);
int getscriptclosure(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TUSERDATA);

    std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    auto script = *reinterpret_cast<std::uintptr_t*>(lua_touserdata(L, 1));

    lua_getfield(L, 1, OBF("ClassName"));
    std::string_view class_name = std::string_view(lua_tolstring(L, -1, nullptr));
    lua_pop(L, 1);

    if (class_name == OBF("LocalScript") || class_name == OBF("Script")) {
        const auto protected_string = *reinterpret_cast<std::uintptr_t*>(script + 0x1B0);
        if (!protected_string || IsBadReadPtr(reinterpret_cast<const void *>(protected_string), 0x10)) { lua_pushnil(L); return 1; }

        auto encrypted_bytecode = std::string(
                *reinterpret_cast<char **>(protected_string + 0x10),
                *reinterpret_cast<uintptr_t *>(protected_string + 0x20));

        lua_State *temp_thread = lua_newthread(L);
        lua_pop(L, 1);

        luaL_sandboxthread(temp_thread);

        temp_thread->userdata->identity = 8;
        temp_thread->userdata->capabilities = (uintptr_t)0x200000000000003Fi64 | 0xFFFFFFF00i64;

        lua_pushvalue(L, 1);
        lua_xmove(L, temp_thread, 1);
        lua_setglobal(temp_thread, OBF("script"));

        int res = rbx::luau::vm_load(temp_thread, &encrypted_bytecode, OBF("getscriptclosure"), 0);
        if (res == LUA_OK) {
            Closure *closure = clvalue(luaA_toobject(temp_thread, -1));
            set_closure_capabilities(closure->l.p, &g_execution->capabilities);

            lua_pop(temp_thread, lua_gettop(temp_thread));

            lua_rawcheckstack(L, 1);
            luaC_threadbarrier(L);

            L->top->value.gc = (GCObject*)closure;
            L->top->tt = LUA_TFUNCTION;
            L->top++;

            return 1;
        } else { lua_pushnil(L);  return 1; }

    } else if (class_name == OBF("ModuleScript")) {
        const auto protected_string = *reinterpret_cast<std::uintptr_t*>(script + 0x158);
        if (!protected_string || IsBadReadPtr(reinterpret_cast<const void *>(protected_string), 0x10)) { lua_pushnil(L); return 1; }

        auto encrypted_bytecode = std::string(
                *reinterpret_cast<char **>(protected_string + 0x10),
                *reinterpret_cast<uintptr_t *>(protected_string + 0x20));


        lua_State *temp_thread = lua_newthread(L);
        lua_pop(L, 1);

        luaL_sandboxthread(temp_thread);

        temp_thread->userdata->identity = 8;
        temp_thread->userdata->capabilities = (uintptr_t)0x200000000000003Fi64 | 0xFFFFFFF00i64;

        lua_pushvalue(L, 1);
        lua_xmove(L, temp_thread, 1);
        lua_setglobal(temp_thread, OBF("script"));

        int res = rbx::luau::vm_load(temp_thread, &encrypted_bytecode, OBF("getscriptclosure"), 0);
        if (res == LUA_OK) {
            Closure *closure = clvalue(luaA_toobject(temp_thread, -1));
            set_closure_capabilities(closure->l.p, &g_execution->capabilities);

            lua_pop(temp_thread, lua_gettop(temp_thread));

            lua_rawcheckstack(L, 1);
            luaC_threadbarrier(L);

            L->top->value.gc = reinterpret_cast<GCObject*>(closure);
            L->top->tt = LUA_TFUNCTION;
            L->top++;

            return 1;
        } else { lua_pushnil(L);  return 1; }
    } else {
        luaL_argerrorL(L, 1, OBF("invalid script type"));
    }
}


int getcallingscript(lua_State *L) {
    lua_check(L, 0);

    if (L->userdata->script.expired() || L->userdata->script.lock() == nullptr) {
        lua_pushnil(L);
        return 1;
    }

    luaC_threadbarrier(L);
    lua_rawcheckstack(L, 1);

    rbx::lua_bridge::push_instance(L, reinterpret_cast<uintptr_t>(L->userdata) + 0x50);
    return 1;
}

/* Ported from ChocoSploit */
int getsenv(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TUSERDATA);

    std::string UserdataTypeName = luaL_typename(L, 1);
    if (UserdataTypeName != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    const auto Script = *static_cast<std::uintptr_t*>(lua_touserdata(L, 1));

    lua_getfield(L, 1, OBF("ClassName"));
    std::string_view ClassName = std::string_view(lua_tolstring(L, -1, nullptr));
    lua_pop(L, 1);


    if (ClassName == OBF("LocalScript") || ClassName == OBF("Script")) {
        std::uintptr_t Node = *reinterpret_cast<std::uintptr_t*>(Script + 0x188);
        if (!Node) {
            lua_pushnil(L);
            return 1;
        }

        std::uintptr_t WeakThreadRef = *reinterpret_cast<std::uintptr_t*>(Node + 0x8);
        if (!WeakThreadRef) {
            lua_pushnil(L);
            return 1;
        }

        std::uintptr_t LiveThreadRef = *reinterpret_cast<std::uintptr_t*>(WeakThreadRef + 0x20);
        if (!LiveThreadRef) {
            lua_pushnil(L);
            return 1;
        }

        lua_State *ScriptThread = *reinterpret_cast<lua_State**>(LiveThreadRef + 0x8);
        if (!ScriptThread) {
            lua_pushnil(L);
            return 1;
        }


        if (ScriptThread->global->mainthread != L->global->mainthread)
            luaL_error(L, OBF("thread is running on a different VM"));

        luaC_threadbarrier(L);
        luaC_threadbarrier(ScriptThread);

        lua_pushvalue(ScriptThread, LUA_GLOBALSINDEX);
        lua_xmove(ScriptThread, L, 1);
        return 1;
    } else if (ClassName == OBF("ModuleScript")) {

        for (lua_Page* Current = L->global->allgcopages; Current;)
        {
            lua_Page* Next = Current->listnext; // block visit might destroy the page

            char* Start;
            char* End;
            int BusyBlocks;
            int BlockSize;
            luaM_getpagewalkinfo(Current, &Start, &End, &BusyBlocks, &BlockSize);

            for (char* Position = Start; Position != End; Position += BlockSize)
            {
                const auto gco = reinterpret_cast<GCObject*>(Position);

                if (!gco)
                    continue;

                if (isdead(L->global, gco))
                    continue;

                // skip memory blocks that are already freed
                if (gco->gch.tt != LUA_TTHREAD)
                    continue;

                const auto thread = reinterpret_cast<lua_State*>(Position);

                if (thread->userdata->script.expired())
                    continue;

                if (reinterpret_cast<uintptr_t>(thread->userdata->script.lock().get()) == Script) {
                    luaC_threadbarrier(L);
                    luaC_threadbarrier(thread);

                    lua_pushvalue(thread, LUA_GLOBALSINDEX);
                    lua_xmove(thread, L, 1);
                    return 1;
                }
            }

            Current = Next;
        }

        lua_pushnil(L);
        return 1;
    } else {
        luaL_argerrorL(L, 1, OBF("invalid script type"));
    }

    return 0;
}

int getinstances(lua_State *L) {
    lua_check(L, 0);

    lua_newtable(L);

    lua_rawcheckstack(L, 1);
    luaC_threadbarrier(L);

    lua_pushvalue(L, LUA_REGISTRYINDEX);
    lua_pushlightuserdata(L, reinterpret_cast<void*>(rbx::lua_bridge::v_push_instance));
    lua_rawget(L, -2);
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 2);

        return 1;
    }

    int i = 0;

    lua_pushnil(L);
    while (lua_next(L, -2))
    {
        if (strcmp(luaL_typename(L, -1), OBF("Instance")) != 0)
        {
            lua_pop(L, 1);
        }
        else
        {
            lua_rawseti(L, -5, ++i);
        }
    }
    lua_pop(L, 2);

    return 1;
}

int getnilinstances(lua_State *L) {
    lua_check(L, 0);
    lua_newtable(L);

    lua_rawcheckstack(L, 1);
    luaC_threadbarrier(L);

    lua_pushvalue(L, LUA_REGISTRYINDEX);
    lua_pushlightuserdata(L, reinterpret_cast<void*>(rbx::lua_bridge::v_push_instance));
    lua_rawget(L, -2);
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 2);

        return 1;
    }

    int i = 0;

    lua_pushnil(L);
    while (lua_next(L, -2))
    {
        if (strcmp(luaL_typename(L, -1), OBF("Instance")) != 0)
        {
            lua_pop(L, 1);
        }
        else
        {
            const auto tt = lua_getfield(L, -1, OBF("Parent"));
            lua_pop(L, 1);

            if (tt == LUA_TNIL) {
                lua_rawseti(L, -5, ++i);
            } else {
                lua_pop(L, 1);
            }
        }
    }
    lua_pop(L, 2);

    return 1;
}

int getscripts(lua_State *L) {
    lua_check(L, 0);
    lua_newtable(L);

    lua_rawcheckstack(L, 1);
    luaC_threadbarrier(L);

    lua_pushvalue(L, LUA_REGISTRYINDEX);
    lua_pushlightuserdata(L, reinterpret_cast<void*>(rbx::lua_bridge::v_push_instance));
    lua_rawget(L, -2);
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 2);

        return 1;
    }

    int i = 0;

    lua_pushnil(L);
    while (lua_next(L, -2))
    {
        if (strcmp(luaL_typename(L, -1), OBF("Instance")) != 0)
        {
            lua_pop(L, 1);
        }
        else
        {
            lua_getfield(L, -1, OBF("ClassName"));
            const auto class_name = std::string_view(lua_tolstring(L, -1, nullptr));
            lua_pop(L, 1);

            if (class_name == OBF("LocalScript")  || class_name == OBF("Script") ||  class_name == OBF("ModuleScript")) {
                lua_rawseti(L, -5, ++i);
            } else {
                lua_pop(L, 1);
            }
        }
    }
    lua_pop(L, 2);

    return 1;
}

int getrunningscripts(lua_State *L) {
    lua_newtable(L);

    int i = 0;
    std::unordered_map<std::shared_ptr<std::uintptr_t*>, bool> running_scripts = { };

    lua_rawcheckstack(L, 1);
    luaC_threadbarrier(L);

    lua_pushvalue(L, LUA_REGISTRYINDEX);
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        if (lua_isthread(L, -1)) {
            lua_State *thread = lua_tothread(L, -1);

            if (thread->userdata != nullptr && !thread->userdata->script.expired()) {
                const auto script = *reinterpret_cast<std::shared_ptr<std::uintptr_t*>*>((uintptr_t)thread->userdata + 0x50);

                rbx::lua_bridge::s_push_instance(L, script);
                lua_getfield(L, -1, OBF("ClassName"));
                const char *class_name = lua_tostring(L, -1);
                lua_pop(L, 1);
                if (thread->global->mainthread == L->global->mainthread && strcmp(class_name, OBF("ModuleScript")) != 0 && !running_scripts.contains(script)) {
                    running_scripts[script] = true;
                    lua_rawseti(L, -5, ++i);
                } else {
                    lua_pop(L, 1);
                }
            }
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    return 1;
}

bool is_roblox_script(uintptr_t script_va) {
    uintptr_t current_instance = *reinterpret_cast<uintptr_t *>(script_va + 0x50);
    while (current_instance != NULL) {
        std::string name = **reinterpret_cast<std::string**>(current_instance + 0x78);
        if (name == OBF("CoreGui") || name == OBF("CorePackages"))
            return false;

        current_instance = *reinterpret_cast<uintptr_t *>(current_instance + 0x50);
    }

    return true;
};
int getloadedmodules(lua_State* L) {
    lua_check(L, 0);
   std::vector<std::shared_ptr<uintptr_t>> instances = { };


    const auto loaded_modules = *reinterpret_cast<std::set<std::weak_ptr<uintptr_t>>*>(reinterpret_cast<uintptr_t>(L->userdata->SharedExtraSpace->ScriptContext) + 0x688);

    for (const auto& module : loaded_modules) {
    if (!module.expired()) {
    auto locked_module = module.lock();
    uintptr_t raw_address = reinterpret_cast<uintptr_t>(locked_module.get());

    if (!is_roblox_script(raw_address))
    continue;

    instances.push_back(locked_module);

    }
    }

    lua_newtable(L);

    for (int i = 0; i < instances.size();) {
        rbx::lua_bridge::ss_push_instance(L, instances[i]);
        lua_rawseti(L, -2, ++i);
    }

    return 1;
}

int fireclickdetector(lua_State* L) {
    lua_check(L, 3);
    luaL_checktype(L, 1, LUA_TUSERDATA);

    const std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    lua_getfield(L, 1, OBF("ClassName"));
    const auto class_name = std::string_view(lua_tolstring(L, -1, nullptr));
    lua_pop(L, 1);

    if (class_name != OBF("ClickDetector"))
        luaL_argerrorL(L, 1, OBF("ClickDetector expected"));

    uintptr_t click_detector = *static_cast<uintptr_t*>(lua_touserdata(L, 1));
    float distance = luaL_optnumber(L, 2, 0);

    lua_getglobal(L, OBF("game"));
    lua_getfield(L, -1, OBF("GetService"));
    lua_pushvalue(L, -2);
    lua_pushstring(L, OBF("Players"));
    lua_pcall(L, 2, 1, 0);
    lua_getfield(L, -1, OBF("LocalPlayer"));
    uintptr_t local_player = *static_cast<uintptr_t *>(lua_touserdata(L, -1));
    lua_pop(L, 3);

    std::string event = luaL_optstring(L, 3, OBF("MouseClick"));

    if (event == OBF("MouseClick")) {
        rbx::click_detector::fire(click_detector, distance, local_player);
    }
    else if (event == OBF("RightMouseClick")) {
        rbx::click_detector::fire_right(click_detector, distance, local_player);
    }
    else if (event == OBF("MouseHoverEnter")) {
        rbx::click_detector::hover_enter(click_detector, local_player);
    }
    else if (event == OBF("MouseHoverLeave")) {
        rbx::click_detector::hover_leave(click_detector, local_player);
    }
    else {
        luaL_argerror(L, 3, OBF("invalid event"));
    }
    return 0;
}

int fireproximityprompt(lua_State* L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TUSERDATA);

    const std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    lua_getfield(L, 1, OBF("ClassName"));
    const auto class_name = std::string_view(lua_tolstring(L, -1, nullptr));
    lua_pop(L, 1);

    if (class_name != OBF("ProximityPrompt"))
        luaL_argerrorL(L, 1, OBF("ProximityPrompt expected"));

    const uintptr_t proximity_prompt = *static_cast<uintptr_t*>(lua_touserdata(L, 1));
    rbx::proximity_prompt::fire(proximity_prompt);

    return 0;
}

int firetouchinterest(lua_State* L) {
    lua_check(L, 3);
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TUSERDATA);
    luaL_argexpected(L, lua_isboolean(L, 3) || lua_isnumber(L, 3), 3, OBF("bool or number"));

    const std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    const std::string ud2_t = luaL_typename(L, 2);
    if (ud2_t != OBF("Instance"))
        luaL_typeerrorL(L, 2, OBF("Instance"));

    lua_getfield(L, 1, OBF("IsA"));
    lua_pushvalue(L, 1);
    lua_pushstring(L, OBF("BasePart"));
    lua_call(L, 2, 1);
    const bool is_base_part_1 = lua_toboolean(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 2, OBF("IsA"));
    lua_pushvalue(L, 2);
    lua_pushstring(L, OBF("BasePart"));
    lua_call(L, 2, 1);
    const bool is_base_part_2 = lua_toboolean(L, -1);
    lua_pop(L, 1);

    luaL_argexpected(L, is_base_part_1, 1, OBF("BasePart"));
    luaL_argexpected(L, is_base_part_2, 2, OBF("BasePart"));


   // rbx::standard_out::printf(1, "past 1");
    const uintptr_t base_part_1 = *static_cast<uintptr_t*>(lua_touserdata(L, 1));
    const uintptr_t base_part_2 = *static_cast<uintptr_t*>(lua_touserdata(L, 2));

    const uintptr_t base_part_primitive_1 = *reinterpret_cast<uintptr_t*>(base_part_1 + rbx::offsets::base_part::primitive);
    const uintptr_t base_part_primitive_2 = *reinterpret_cast<uintptr_t*>(base_part_2 + rbx::offsets::base_part::primitive);

    //rbx::standard_out::printf(1, "past 2");

    const uintptr_t base_part_world_1 = *reinterpret_cast<uintptr_t*>(base_part_primitive_1 + rbx::offsets::base_part::world);
    const uintptr_t base_part_world_2 = *reinterpret_cast<uintptr_t*>(base_part_primitive_2 + rbx::offsets::base_part::world);

    //rbx::standard_out::printf(1, "past3");

    if (!base_part_1 || !base_part_2 || base_part_world_2 != base_part_world_1)
        luaL_error(L, OBF("new overlap in different world"));

    //rbx::standard_out::printf(1, "past 4");

    const int touch_ended = lua_isboolean(L, 3) ? lua_toboolean(L, 3) : lua_tointeger(L, 3);


    rbx::base_part::touch_interest(base_part_world_2, base_part_primitive_2, base_part_primitive_1, touch_ended, 1);
   // rbx::standard_out::printf(1, "past 5");
    return 0;
}

int isscriptable(lua_State*L) {
    lua_check(L, 2);
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TSTRING);

    const std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    int atom{};

    uintptr_t instance = *reinterpret_cast<uintptr_t *>(lua_touserdata(L, 1));
    std::string property_name = lua_tostringatom(L, 2, &atom);

    std::uintptr_t prop_desc = rbx::property_descriptor::ktable[atom];
    if (!prop_desc)
        luaL_argerrorL(L, 2, OBF("property does not exist"));

    uintptr_t class_desc = *reinterpret_cast<uintptr_t *>(instance + 0x18);
    const auto property_descriptor = rbx::property_descriptor::get_property(class_desc + 0x3B8, &prop_desc);
    if (!property_descriptor)
        luaL_argerrorL(L, 2, OBF("property does not exist"));
    bool is_scriptable = *reinterpret_cast<DWORD*>(*property_descriptor + 0x48) & 0x20;
    std::optional<bool> saved_state = get_scriptable_state(instance, property_name);

    lua_pushboolean(L, saved_state.value_or(is_scriptable));
    return 1;
}

int setscriptable(lua_State*L) {
    lua_check(L, 3);
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TSTRING);
    luaL_checktype(L, 3, LUA_TBOOLEAN);

    const std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    int atom{};

    const uintptr_t instance = *static_cast<uintptr_t *>(lua_touserdata(L, 1));
    const std::string property_name = lua_tostringatom(L, 2, &atom);
    const bool state = lua_toboolean(L, 3);

    std::uintptr_t prop_desc = rbx::property_descriptor::ktable[atom];
    if (!prop_desc)
        luaL_argerrorL(L, 2, OBF("property does not exist"));

    uintptr_t class_desc = *reinterpret_cast<uintptr_t *>(instance + 0x18);
    const auto property_descriptor = rbx::property_descriptor::get_property(class_desc + 0x3B8, &prop_desc);
    if (!property_descriptor)
        luaL_argerrorL(L, 2, OBF("property does not exist"));
    bool is_scriptable = *reinterpret_cast<DWORD*>(*property_descriptor + 0x48) & 0x20;

    const std::optional<bool> saved_state = get_scriptable_state(instance, property_name);

    change_scriptable_state(instance, property_name, state);

    lua_pushboolean(L, saved_state.value_or(is_scriptable));
    return 1;
}

int gethiddenproperty(lua_State *L) {
    lua_check(L, 2);
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TSTRING);

    const std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    int atom{};

    const uintptr_t instance = *static_cast<uintptr_t *>(lua_touserdata(L, 1));
    const std::string property_name = lua_tostringatom(L, 2, &atom);

    std::uintptr_t prop_desc = rbx::property_descriptor::ktable[atom];
    if (!prop_desc)
        luaL_argerrorL(L, 2, OBF("property does not exist"));

   // rbx::standard_out::printf(1, "property: %s", property_name.c_str());
    const uintptr_t class_desc = *reinterpret_cast<uintptr_t *>(instance + 0x18);
    const auto property_descriptor = rbx::property_descriptor::get_property(class_desc + 0x3B8, &prop_desc);
    if (!property_descriptor)
        luaL_argerrorL(L, 2, OBF("property does not exist"));


    //rbx::standard_out::printf(1, "instance: %p", instance);
    const uintptr_t get_set_impl = *reinterpret_cast<uintptr_t *>(*property_descriptor + 0x88);
    const uintptr_t get_set_vft = *reinterpret_cast<uintptr_t *>(get_set_impl);
    const uintptr_t getter = *reinterpret_cast<uintptr_t *>(get_set_vft + 0x18);

    const uintptr_t ttype = *reinterpret_cast<uintptr_t*>(*property_descriptor + 0x50);
    const int type_number = *reinterpret_cast<int*>(ttype + 0x38);

    const bool is_scriptable = *reinterpret_cast<DWORD*>(*property_descriptor + 0x48) & 0x20;

    if (type_number == rbx::reflection::ReflectionType_Bool) {
        const auto bool_getter = reinterpret_cast<bool(__fastcall*)(uintptr_t, uintptr_t)>(getter);
        const bool result = bool_getter(get_set_impl, instance);

        lua_pushboolean(L, result);
        lua_pushboolean(L, !is_scriptable);
        return 2;
    } else if (type_number == rbx::reflection::ReflectionType_Int) {
        const auto int_getter = reinterpret_cast<int(__fastcall*)(uintptr_t, uintptr_t)>(getter);
        const int result = int_getter(get_set_impl, instance);

        lua_pushinteger(L, result);
        lua_pushboolean(L, !is_scriptable);
        return 2;
    } else if (type_number == rbx::reflection::ReflectionType_Float) {
        const auto float_getter = reinterpret_cast<float(__fastcall*)(uintptr_t, uintptr_t)>(getter);
        const float result = float_getter(get_set_impl, instance);

        lua_pushnumber(L, result);lua_pushboolean(L, !is_scriptable);
        return 2;
    } else if (type_number == rbx::reflection::ReflectionType_String) {
        const auto string_getter = reinterpret_cast<void(__fastcall*)(uintptr_t, std::string*, uintptr_t)>(getter);
        std::string result{};
        string_getter(get_set_impl, &result, instance);

        //rbx::standard_out::printf(1, "getter: %p", (getter - (uintptr_t) GetModuleHandleA(0)));
        lua_pushlstring(L, result.data(), result.size());
        lua_pushboolean(L, !is_scriptable);
        return 2;
    } else if (type_number == rbx::reflection::ReflectionType_BinaryString) {
        const auto string_getter = reinterpret_cast<void(__fastcall*)(uintptr_t, uintptr_t*, uintptr_t)>(getter);
        auto* result = new uintptr_t[100];
        string_getter(get_set_impl, result, instance);

        const std::string res = *reinterpret_cast<std::string*>(result);
        //rbx::standard_out::printf(1, "result: %p", *result);
        lua_pushstring(L, res.c_str());
        lua_pushboolean(L, !is_scriptable);
        return 2;
    } else if (type_number == rbx::reflection::ReflectionType_SharedString) {

        const auto shared_string_getter = reinterpret_cast<void(__fastcall*)(uintptr_t, rbx::shared_string_t**, uintptr_t)>(getter);
        rbx::shared_string_t* result{};
        shared_string_getter(get_set_impl, &result, instance);

        lua_pushlstring(L, result->content.c_str(), result->content.size());
        lua_pushboolean(L, !is_scriptable);
        return 2;
    } else if (type_number == rbx::reflection::ReflectionType_SystemAddress) {
        const auto system_address_getter = reinterpret_cast<float(__fastcall*)(uintptr_t, rbx::system_address_t*, uintptr_t)>(getter);
        rbx::system_address_t result{};
        system_address_getter(get_set_impl, &result, instance);

        lua_pushinteger(L, result.remote_id.peer_id);
        lua_pushboolean(L, !is_scriptable);
        return 2;
    } else { /* Fallback to setting scriptable :broken_heart: */

        const bool old_scriptable_state = get_scriptable_state(instance, property_name).value_or( is_scriptable );

        change_scriptable_state(instance, property_name, true);

        lua_getfield(L, 1, property_name.c_str());

        change_scriptable_state(instance, property_name, old_scriptable_state);

        lua_pushboolean(L, !is_scriptable);
        return 2;
    }

    return 0;
}

int sethiddenproperty(lua_State *L) {
    lua_check(L, 3);
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TSTRING);
    luaL_checkany(L, 3);

    std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    int atom{};

    uintptr_t instance = *reinterpret_cast<uintptr_t *>(lua_touserdata(L, 1));
    std::string property_name = lua_tostringatom(L, 2, &atom);

    std::uintptr_t prop_desc = rbx::property_descriptor::ktable[atom];
    if (!prop_desc)
        luaL_argerrorL(L, 2, OBF("property does not exist"));

    uintptr_t class_desc = *reinterpret_cast<uintptr_t *>(instance + 0x18);
    const auto property_descriptor = rbx::property_descriptor::get_property(class_desc + 0x3B8, &prop_desc);
    if (!property_descriptor)
        luaL_argerrorL(L, 2, OBF("property does not exist"));


    uintptr_t get_set_impl = *reinterpret_cast<uintptr_t *>(*property_descriptor + 0x88);
    uintptr_t get_set_vft = *reinterpret_cast<uintptr_t *>(get_set_impl);
    uintptr_t setter = *reinterpret_cast<std::uintptr_t *>(get_set_vft + 0x20);

    uintptr_t ttype = *reinterpret_cast<uintptr_t*>(*property_descriptor + 0x50);
    int type_number = *reinterpret_cast<int*>(ttype + 0x38);

    bool is_scriptable = *reinterpret_cast<DWORD*>(*property_descriptor + 0x48) & 0x20;

    if (type_number == rbx::reflection::ReflectionType_Bool) {
        bool property_val = luaL_checkboolean(L, 3);

        auto bool_setter = reinterpret_cast<bool(__fastcall*)(uintptr_t, uintptr_t, bool*)>(setter);
        bool_setter(get_set_impl, instance, &property_val);

        lua_pushboolean(L, !is_scriptable);
        return 1;
    } else if (type_number == rbx::reflection::ReflectionType_Int) {
        int property_val = luaL_checkinteger(L, 3);

        auto int_setter = reinterpret_cast<bool(__fastcall*)(uintptr_t, uintptr_t, int*)>(setter);
        int_setter(get_set_impl,instance, &property_val);

        lua_pushboolean(L, !is_scriptable);
        return 1;
    } else if (type_number == rbx::reflection::ReflectionType_Float) {
        float property_val = luaL_checknumber(L, 3);

        auto float_setter = reinterpret_cast<bool(__fastcall*)(uintptr_t, uintptr_t, float*)>(setter);
        float_setter(get_set_impl, instance, &property_val);

        lua_pushboolean(L, !is_scriptable);
        return 1;
    } else if (type_number == rbx::reflection::ReflectionType_String || type_number == rbx::reflection::ReflectionType_BinaryString) {
        std::string property_val = luaL_checkstring(L, 3);

        auto string_setter = reinterpret_cast<bool(__fastcall*)(std::uintptr_t, uintptr_t, std::string*)>(setter);
        string_setter(get_set_impl, instance, &property_val);

        lua_pushboolean(L, !is_scriptable);
        return 1;
    } else if (type_number == rbx::reflection::ReflectionType_SharedString) {
        std::string property_val = luaL_checkstring(L, 3);
        rbx::shared_string_t result{};
        result.content = property_val;

        auto shared_string_setter = reinterpret_cast<bool(__fastcall*)(std::uintptr_t, uintptr_t, rbx::shared_string_t*)>(setter);
        shared_string_setter(get_set_impl, instance, &result);

        lua_pushboolean(L, !is_scriptable);
        return 1;
    } else if (type_number == rbx::reflection::ReflectionType_SystemAddress) {
        int property_val = luaL_checkinteger(L, 3);
        rbx::system_address_t result{};
        result.remote_id.peer_id = property_val;

        auto system_address_setter = reinterpret_cast<bool(__fastcall*)(std::uintptr_t, uintptr_t, rbx::system_address_t*)>(setter);
        system_address_setter(get_set_impl, instance, &result);

        lua_pushboolean(L, !is_scriptable);
        return 1;
    } else {
        luaL_error(L, OBF("unsupported property type"));
    }
}

int isnetworkowner(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TUSERDATA);

    const std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    lua_getfield(L, 1, OBF("IsA"));
    lua_pushvalue(L, 1);
    lua_pushstring(L, OBF("BasePart"));
    lua_call(L, 2, 1);
    const bool is_base_part_1 = lua_toboolean(L, -1);
    lua_pop(L, 1);

    luaL_argexpected(L, is_base_part_1, 1, OBF("BasePart"));

    lua_rawcheckstack(L, 3);
    luaC_threadbarrier(L);

    lua_pushcclosure(L, gethiddenproperty, nullptr, 0);
    lua_pushvalue(L, 1);
    lua_pushstring(L, OBF("NetworkOwnerV3"));
    lua_call(L, 2, 2);

    const int peer_id = lua_tointeger(L, -2);
    lua_pop(L, 2);

    lua_pushboolean(L, peer_id > 2);
    return 1;
}

std::string execute_file(const char* cmd) {
    SECURITY_ATTRIBUTES sa;
    HANDLE read_pipe, write_pipe;
    std::string result;

    char buffer[4096];
    DWORD bytes_read;


    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0)) {
        return "";
    }


    SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);


    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = write_pipe;
    si.hStdError = write_pipe;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);


    if (!CreateProcessA(
            0,
            (LPSTR)cmd,
            0,
            0,
            true,
            CREATE_NO_WINDOW,
            0,
            0,
            &si,
            &pi))
    {
        CloseHandle(write_pipe);
        CloseHandle(read_pipe);
        return "";
    }

    CloseHandle(write_pipe);

    while (true) {
        if (!ReadFile(read_pipe, buffer, 4095, &bytes_read, NULL) || bytes_read == 0) {
            break;
        }
        buffer[bytes_read] = '\0';
        result += buffer;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(read_pipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return result;
}

int decompile(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TUSERDATA);

    lua_rawcheckstack(L, 2);
    luaC_threadbarrier(L);

    lua_pushcclosurek(L, getscriptbytecode, nullptr, 0, 0);
    lua_pushvalue(L, 1);
    lua_call(L, 1, 1);

    size_t bytecode_sz{};
    const char *bytecode = lua_tolstring(L, -1, &bytecode_sz);
    lua_pop(L, 1);

    {
        std::ofstream file_stream{};
        file_stream.open(OBF("bytecode.bin"), std::ios::out | std::ios::binary);
        //f (!output_file) { lua_pushstring(L, OBF("")); return 1; }

        file_stream.write(bytecode, bytecode_sz);
        file_stream.close();
    }


    std::filesystem::path lifter_path = globals::bin_path / OBF("luau-lifter.exe");

    if (!std::filesystem::exists(lifter_path)) {
        lua_pushstring(L, OBF(""));
        return 1;
    }

    std::string command = std::format("{} {}",
                                      lifter_path.string().c_str(),
    (std::filesystem::current_path() / OBF("bytecode.bin")).string().c_str());

    std::string output = execute_file(command.c_str());

    if (std::remove(OBF("bytecode.bin")) != 0) { lua_pushstring(L, OBF("")); return 1; }

    lua_pushlstring(L, output.c_str(), output.size());
    return 1;
}

int getcallbackvalue(lua_State *L) {
    lua_check(L, 2);
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TSTRING);

    const std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    int atom{};

    const uintptr_t instance = *static_cast<uintptr_t *>(lua_touserdata(L, 1));
    std::string property_name = lua_tostringatom(L, 2, &atom);

    std::uintptr_t prop_desc = rbx::property_descriptor::ktable[atom];
    if (!prop_desc)
        luaL_argerrorL(L, 2, OBF("property does not exist"));

    const uintptr_t class_desc = *reinterpret_cast<uintptr_t *>(instance + 0x18);
    const auto property_descriptor = rbx::property_descriptor::get_property(class_desc + 0x3B8, &prop_desc);
    if (!property_descriptor)
        luaL_argerrorL(L, 2, OBF("property does not exist"));

    const int property_type = *reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(property_descriptor) + 0x8);

    if (property_type != 4)
        luaL_argerrorL(L, 2, OBF("non callback property"));

    const std::uintptr_t callback_instance_0 = (instance + *reinterpret_cast<std::uintptr_t*>(*property_descriptor + 0x80));
    if (!*reinterpret_cast<std::uintptr_t*>(callback_instance_0 + 0x38)) {
        lua_pushnil(L);
        return 1;
    }

    const std::uintptr_t callback_instance_1 = *reinterpret_cast<uintptr_t*>(callback_instance_0 + 0x18);
    const std::uintptr_t callback_instance_2 = *reinterpret_cast<uintptr_t*>(callback_instance_1 + 0x38);
    const std::uintptr_t callback_weak_function_ref = *reinterpret_cast<uintptr_t*>(callback_instance_2 + 0x28);
    if (!callback_weak_function_ref) { lua_pushnil(L); return 1; }

    const int callback_id = *reinterpret_cast<int*>(callback_weak_function_ref + 0x14);
    lua_getref(L, callback_id);

    if (lua_isfunction(L, -1))
        return 1;

    lua_pop(L, 1);

    lua_pushnil(L);
    return 1;
}

int setcallbackvalue(lua_State *L) {
    lua_check(L, 3);
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TSTRING);
    luaL_checktype(L, 3, LUA_TFUNCTION);

    std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    int atom{};

    uintptr_t instance = *reinterpret_cast<uintptr_t *>(lua_touserdata(L, 1));
    std::string property_name = lua_tostringatom(L, 2, &atom);

    std::uintptr_t prop_desc = rbx::property_descriptor::ktable[atom];
    if (!prop_desc)
        luaL_argerrorL(L, 2, OBF("property does not exist"));

    const uintptr_t class_desc = *reinterpret_cast<uintptr_t *>(instance + 0x18);
    const auto property_descriptor = rbx::property_descriptor::get_property(class_desc + 0x3B8, &prop_desc);
    if (!property_descriptor)
        luaL_argerrorL(L, 2, OBF("property does not exist"));

    //int property_type = *reinterpret_cast<int*>(*property_descriptor + 0x8);

    //if (property_type != 4)
    //    luaL_argerrorL(L, 2, OBF("invalid property"));

    std::uintptr_t callback_instance_0 = (instance + *reinterpret_cast<std::uintptr_t*>(*property_descriptor + 0x80));
    if (!*reinterpret_cast<std::uintptr_t*>(callback_instance_0 + 0x38))
        luaL_errorL(L, OBF("no callback bound to property"));

    std::uintptr_t callback_instance_1 = *reinterpret_cast<std::uintptr_t*>(callback_instance_0 + 0x18);
    std::uintptr_t callback_instance_2 = *reinterpret_cast<std::uintptr_t*>(callback_instance_1 + 0x38);
    std::uintptr_t callback_weak_function_ref = *reinterpret_cast<std::uintptr_t*>(callback_instance_2 + 0x28);
    if (!callback_weak_function_ref) { lua_pushnil(L); return 1; }

    *reinterpret_cast<int*>(callback_weak_function_ref + 0x14) = lua_ref(L, 3);

    return 0;
}

int getfflag(lua_State* L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TSTRING);

    std::string key = lua_tostring(L, 1);

    std::string value;
    const std::uintptr_t singleton = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr)) + rbx::rvas::fast_flags::singleton);

    const auto result = rbx::fast_flags::get_fast_flag(singleton, &key, &value, 0);
    if (!result) {
        lua_pushstring(L, "");

        return 1;
    }

    lua_pushlstring(L, value.c_str(), value.length());
    return 1;
}

int setfflag(lua_State* L) {
    lua_check(L, 2);
    luaL_checktype(L, 1, LUA_TSTRING);
    luaL_checkany(L, 2);

    std::string fast_flag = lua_tostring(L, 1);

    std::string value;
    std::uintptr_t singleton = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr)) + rbx::rvas::fast_flags::singleton);

    const auto exists = rbx::fast_flags::get_fast_flag(singleton, &fast_flag, &value, 0);

    value.clear();

    if (!exists) {
        lua_pushboolean(L, false);

        return 1;
    }

    if (lua_isboolean(L, 2))
        value = lua_toboolean(L, 2) ? OBF("true") : OBF("false");
    else if (lua_isstring(L, 2))
        value = lua_tostring(L, 2);
    else if (lua_isnumber(L, 2))
        value = std::to_string(lua_tonumber(L, 2));
    else {
        luaL_argerrorL(L, 1, OBF("invalid value type"));
    }

    auto result = rbx::fast_flags::set_fast_flag(singleton, &fast_flag, &value, 0x7F, 0, 0);

    lua_pushboolean(L, result);
    return 1;
}


int saveinstance(lua_State *L) {
    rbx::standard_out::printf(1, OBF("Powered by UniversalSynSaveInstance - https://discord.gg/wx4ThpAsmw"));

    g_taskscheduler->queue_script(OBF(R"(
    local saave_instance = loadstring(game:HttpGet("https://raw.githubusercontent.com/luau/SynSaveInstance/main/saveinstance.luau"))()
    saave_instance({})
    )"));
    return 0;
}

void environment::load_scripts_lib(lua_State *L) {
    lua_pushcclosure(L, getsenv, nullptr, 0);
    lua_setglobal(L, "getsenv");
    lua_pushcclosure(L, getsenv, nullptr, 0);
    lua_setglobal(L, "getmenv");

    lua_pushcclosure(L, getcallingscript, nullptr, 0);
    lua_setglobal(L, "getcallingscript");

    lua_pushcclosure(L, getinstances, nullptr, 0);
    lua_setglobal(L, "getinstances");

    lua_pushcclosure(L, getnilinstances, nullptr, 0);
    lua_setglobal(L, "getnilinstances");

    lua_pushcclosure(L, getscripts, nullptr, 0);
    lua_setglobal(L, "getscripts");

    lua_pushcclosure(L, getrunningscripts, nullptr, 0);
    lua_setglobal(L, "getrunningscripts");

    lua_pushcclosure(L, getloadedmodules, nullptr, 0);
    lua_setglobal(L, "getloadedmodules");

    lua_pushcclosure(L, getscriptbytecode, nullptr, 0);
    lua_setglobal(L, "getscriptbytecode");

    lua_pushcclosure(L, getscriptbytecode, nullptr, 0);
    lua_setglobal(L, "dumpstring");

    lua_pushcclosure(L, getscripthash, nullptr, 0);
    lua_setglobal(L, "getscripthash");

    lua_pushcclosure(L, getscriptclosure, nullptr, 0);
    lua_setglobal(L, "getscriptclosure");
    lua_pushcclosure(L, getscriptclosure, nullptr, 0);
    lua_setglobal(L, "getscriptfunction");


    lua_pushcclosure(L, fireclickdetector, nullptr, 0);
    lua_setglobal(L, "fireclickdetector");

    lua_pushcclosure(L, firetouchinterest, nullptr, 0);
    lua_setglobal(L, "firetouchinterest");

    lua_pushcclosure(L, fireproximityprompt, nullptr, 0);
    lua_setglobal(L, "fireproximityprompt");

    lua_pushcclosure(L, decompile, nullptr, 0);
    lua_setglobal(L, "decompile");

    lua_pushcclosure(L, gethiddenproperty, nullptr, 0);
    lua_setglobal(L, "gethiddenproperty");

    lua_pushcclosure(L, sethiddenproperty, nullptr, 0);
    lua_setglobal(L, "sethiddenproperty");

    lua_pushcclosure(L, isnetworkowner, nullptr, 0);
    lua_setglobal(L, "isnetworkowner");

    lua_pushcclosure(L, isscriptable, nullptr, 0);
    lua_setglobal(L, "isscriptable");

    lua_pushcclosure(L, setscriptable, nullptr, 0);
    lua_setglobal(L, "setscriptable");

    lua_pushcclosure(L, getcallbackvalue, nullptr, 0);
    lua_setglobal(L, "getcallbackvalue");
    lua_pushcclosure(L, getcallbackvalue, nullptr, 0);
    lua_setglobal(L, "getcallbackmember");
    lua_pushcclosure(L, setcallbackvalue, nullptr, 0);
    lua_setglobal(L, "setcallbackvalue");

    lua_pushcclosure(L, getfflag, nullptr, 0);
    lua_setglobal(L, "getfflag");
    lua_pushcclosure(L, setfflag, nullptr, 0);
    lua_setglobal(L, "setfflag");

    lua_pushcclosure(L, saveinstance, nullptr, 0);
    lua_setglobal(L, "saveinstance");

}

void environment::reset_scripts_lib()
{
    environment::scriptable_map.clear();
}