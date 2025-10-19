//
// Created by user on 28/04/2025.
//
#include "globals.h"
#include "lapi.h"
#include "lgc.h"
#include "lobject.h"
#include "lstate.h"
#include "../environment.h"
#include "src/rbx/engine/game.h"
#include <map>

using signal_c_function_t = void(__fastcall *)(uintptr_t);

struct rbx_signal_t {
    uint64_t _[2];
    rbx_signal_t* next;
};

struct connection_t {
    rbx_signal_t* signal;
    uintptr_t old_enabled;

    signal_c_function_t original_signal_c_func;
    bool is_c;
    bool is_once;
};

std::map<rbx_signal_t*, connection_t> connection_map = { };

template<typename T>
__forceinline static bool is_ptr_valid(T* tValue) {
    const auto ptr = reinterpret_cast<const void*>(const_cast<const T*>(tValue));
    auto buffer = MEMORY_BASIC_INFORMATION{};

    if (const auto read = VirtualQuery(ptr, &buffer, sizeof(buffer)); read != 0 && sizeof(buffer) != read) {
    }
    else if (read == 0) {
        return false;
    }

    if (buffer.RegionSize < sizeof(T)) {
        return false;
    }

    if (buffer.State & MEM_FREE == MEM_FREE) {
        return false;
    }

    auto valid_prot = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ |
                      PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

    if (buffer.Protect & valid_prot) {
        return true;
    }
    if (buffer.Protect & (PAGE_GUARD | PAGE_NOACCESS)) {
        return false;
    }

    return true;
}

int stub_ref = 0;

int stub(lua_State *L) {
    return 0;
}

void c_stub(uintptr_t) {}

int enable_connection(lua_State *L) {
    luaL_checktype(L, 1, LUA_TUSERDATA);

    const auto connection = static_cast<connection_t*>(lua_touserdata(L, 1));

    if (connection->is_c) {
        *reinterpret_cast<signal_c_function_t*>(reinterpret_cast<uintptr_t>(connection->signal) + 0x8) = connection->original_signal_c_func;
    } else {
        *reinterpret_cast<uintptr_t *>(reinterpret_cast<uintptr_t>(connection->signal) + 0x20) = connection->old_enabled;
    }

    return 0;
}

int disable_connection(lua_State *L) {
    luaL_checktype(L, 1, LUA_TUSERDATA);

    auto *connection = static_cast<connection_t*>(lua_touserdata(L, 1));

    if (connection->is_c)
    {
        *reinterpret_cast<signal_c_function_t*>(reinterpret_cast<uintptr_t>(connection->signal) + 0x8) = c_stub;
    } else
    {
        *reinterpret_cast<uintptr_t *>(reinterpret_cast<uintptr_t>(connection->signal) + 0x20) = 0;
    }

    return 0;
}

int disconnect_connection(lua_State *L) {
    luaL_checktype(L, 1, LUA_TUSERDATA);

    lua_pushvalue(L, 1);
    lua_rawgetfield(L, LUA_REGISTRYINDEX, OBF("RBXScriptConnection"));

    lua_setmetatable(L, -2);
    lua_getfield(L, -2, OBF("Disconnect"));
    lua_pushvalue(L, -2);
    lua_call(L, 1, 0);

    return 0;
}

int fire_connection(lua_State *L) {
    //lua_remove(L, 1);

    const auto nargs = lua_gettop(L);

    lua_pushvalue(L, lua_upvalueindex(1));
    if (lua_isfunction(L, -1))
    {
        lua_insert(L, 1);

       // rbx::script_context::task_defer(L);
        lua_call(L, nargs, 0);
    }

    return 0;
}

int defer_connection(lua_State *L) {
    //lua_remove(L, 1);

    const auto nargs = lua_gettop(L);

    lua_getglobal(L, OBF("task"));
    lua_getfield(L, -1, OBF("defer"));
    lua_remove(L, -2);
    {
        lua_pushvalue(L, lua_upvalueindex(1));
        if (!lua_isfunction(L, -1))
        {
            return 0;
        }

        lua_insert(L, 1); // func to index 1
        lua_insert(L, 1); // defer to index 1
        /*
        stack before:
        args... nargs
        defer   nargs + 1
        func    nargs + 2

        stack now:
        defer   1
        func    2
        args... 3 + nargs
        */
    }
    lua_call(L, nargs + 1, 0); // args + func, return.;

    return 0;
}

int mt_index(lua_State *L) {
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TSTRING);

    auto *connection = static_cast<connection_t*>(lua_touserdata(L, 1));
    const std::string key = lua_tostring(L, 2);

    // FIELDS
    if (key == OBF("Enabled")) {
        if (connection->is_c)
        {
            const auto current_signal_c_func = *reinterpret_cast<signal_c_function_t*>(reinterpret_cast<uintptr_t>(connection->signal) + 0x8);
            const auto strong = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(connection->signal));
            const auto weak = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(connection->signal) + 0x4);

            const auto has_references = (weak != 0 || strong != 0);

            lua_pushboolean(L, has_references && current_signal_c_func != c_stub);
        } else
        {
            lua_pushboolean(L, *reinterpret_cast<uintptr_t *>(reinterpret_cast<uintptr_t>(connection->signal) + 0x20) != 0);
        }

        return 1;
    } else if (key == OBF("Disabled")) {
        if (connection->is_c) {
            const auto current_signal_c_func = *reinterpret_cast<signal_c_function_t*>(reinterpret_cast<uintptr_t>(connection->signal) + 0x8);
            const auto strong = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(connection->signal));
            const auto weak = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(connection->signal) + 0x4);

            const auto has_references = (weak != 0 || strong != 0);

            lua_pushboolean(L,  current_signal_c_func == c_stub || !has_references);
        } else {
            lua_pushboolean(L, *reinterpret_cast<uintptr_t *>(reinterpret_cast<uintptr_t>(connection->signal) + 0x20) == 0);
        }
        return 1;
    } else if (key == OBF("LuaConnection")) {
        if (connection->is_c) { lua_pushboolean(L, false); return 1; }

        const uintptr_t slot_wrapper = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(connection->signal) + 0x30);
        if (!slot_wrapper) { lua_pushnil(L); return 1; }

        const uintptr_t weak_object = *reinterpret_cast<uintptr_t*>(slot_wrapper + 0x70);
        if (!weak_object) { lua_pushnil(L); return 1; }

        const lua_State* thread = *reinterpret_cast<lua_State**>(weak_object + 0x8);
        if (!thread) { lua_pushnil(L); return 1; }

        if(thread->tt != LUA_TTHREAD) { lua_pushnil(L); return 1; }

        if (thread->global != L->global) { lua_pushnil(L); return 1; }

        int function_ref = *reinterpret_cast<int*>(weak_object + 0x14);
        if (!function_ref) { lua_pushnil(L); return 1; }

        lua_rawgeti(L, LUA_REGISTRYINDEX, function_ref);


        if (lua_isfunction(L, -1)) {
            const Closure *closure = clvalue(luaA_toobject(L, -1));
            lua_pop(L, 1);

            lua_pushboolean(L, !closure->isC);
        } else {
            lua_pop(L, 1);
            lua_pushboolean(L, false);
        }

        return 1;
    } else if (key == OBF("ForeignState")) {
        if (connection->is_c) { lua_pushboolean(L, true); return 1; }

        const uintptr_t slot_wrapper = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(connection->signal) + 0x30);
        if (!slot_wrapper) { lua_pushboolean(L, true); return 1; }

        const uintptr_t weak_object = *reinterpret_cast<uintptr_t*>(slot_wrapper + 0x70);
        if (!weak_object) { lua_pushboolean(L, true); return 1; }

        const lua_State* thread = *reinterpret_cast<lua_State**>(weak_object + 0x8);
        if (!thread) { lua_pushboolean(L, true); return 1; }

        if(thread->tt != LUA_TTHREAD) { lua_pushboolean(L, true); return 1; }

        lua_pushboolean(L, thread->global != L->global);
        return 1;
    } else if (key == OBF("Type")) {
        if (connection->is_c) {
            lua_pushstring(L, OBF("C"));
        } else {
            lua_pushstring(L, OBF("Lua"));
        }
        return 1;
    } else if (key == OBF("Function")) {
        if (connection->is_c) { lua_pushnil(L); return 1; }

        const uintptr_t slot_wrapper = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(connection->signal) + 0x30);
        if (!slot_wrapper) { lua_pushnil(L); return 1; }

        const uintptr_t weak_object = *reinterpret_cast<uintptr_t*>(slot_wrapper + 0x70);
        if (!weak_object) { lua_pushnil(L); return 1; }

        const lua_State* thread = *reinterpret_cast<lua_State**>(weak_object + 0x8);
        if (!thread) { lua_pushnil(L); return 1; }

        if(thread->tt != LUA_TTHREAD) { lua_pushnil(L); return 1; }

        if (thread->global != L->global) { lua_pushnil(L); return 1; }

        const int function_ref = *reinterpret_cast<int*>(weak_object + 0x14);
        if (!function_ref) { lua_pushnil(L); return 1; }

        lua_rawgeti(L, LUA_REGISTRYINDEX, function_ref);
        return 1;
    } else if (key == OBF("Thread")) {
        if (connection->is_c) { lua_pushnil(L); return 1; }

        uintptr_t slot_wrapper = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(connection->signal) + 0x30);
        if (!slot_wrapper) { lua_pushnil(L); return 1; }

        uintptr_t weak_object = *reinterpret_cast<uintptr_t*>(slot_wrapper + 0x70);
        if (!weak_object) { lua_pushnil(L); return 1; }

        lua_State* thread = *reinterpret_cast<lua_State**>(weak_object + 0x8);
        if (!thread) { lua_pushnil(L); return 1; }

        if(thread->tt != LUA_TTHREAD) { lua_pushnil(L); return 1; }

        if (thread->global != L->global) { lua_pushnil(L); return 1; }

        lua_rawcheckstack(L, 1);
        luaC_threadbarrier(L);

        L->top->value.gc = reinterpret_cast<GCObject*>(thread);
        L->top->tt = LUA_TTHREAD;
        L->top++;
        return 1;
    }
    // METHODS
    else if (key == OBF("Enable")) {
        lua_pushcclosurek(L, enable_connection, nullptr, 0, nullptr);
        return 1;
    } else if (key == OBF("Disable")) {
        lua_pushcclosurek(L, disable_connection, nullptr, 0, nullptr);
        return 1;
    } else if (key == OBF("Disconnect")) {
        if (connection->is_c) { lua_pushcclosurek(L, stub, nullptr, 0, nullptr); } // later

        //lua_pushvalue(L, 1);
        lua_pushcclosurek(L, disconnect_connection, nullptr, 0, nullptr);
        return 1;
    } else if (key == OBF("Fire")) {
        if (connection->is_c) { lua_pushnil(L); return 1; }

        lua_getfield(L, 1, OBF("Function"));
        lua_pushcclosurek(L, fire_connection, nullptr, 1, nullptr);
        return 1;
    } else if (key == OBF("Defer")) {
        if (connection->is_c) { lua_pushnil(L); return 1; }

        lua_getfield(L, 1, OBF("Function"));
        lua_pushcclosurek(L, defer_connection, nullptr, 1, nullptr);
        return 1;
    }


    return 0;
}

bool is_c_signal(uintptr_t signal) {
    bool is_c = false;
    auto* slot_wrapper = reinterpret_cast<uintptr_t*>(signal + 0x30);

    if (!is_ptr_valid(slot_wrapper)) {
        is_c = true;
        return is_c;
    }

    auto* weak_object = reinterpret_cast<uintptr_t*>(*slot_wrapper + 0x70);

    if (!is_ptr_valid(weak_object)) {
        is_c = true;
    } else {
        auto* object_field = reinterpret_cast<uintptr_t*>(*weak_object + 0x8);
        is_c = !is_ptr_valid(object_field);
    }

    return is_c;
}

int getconnections(lua_State *L) {
    luaL_checktype(L, 1, LUA_TUSERDATA);

    if (strcmp(luaL_typename(L, 1), OBF("RBXScriptSignal")) != 0)
        luaL_typeerrorL(L, 1, OBF("RBXScriptSignal"));

    const auto script_signal = *static_cast<uintptr_t *>(lua_touserdata(L, 1));
    const auto script_signal_descriptor = *reinterpret_cast<uintptr_t *>(script_signal);

    lua_getfield(L, 1, OBF("Connect"));
    lua_pushvalue(L, 1);
    lua_pushcfunction(L, [](lua_State*) -> int { return 0; }, nullptr);
    lua_pcall(L, 2, 1, 0);

    const rbx_signal_t *signal = *static_cast<rbx_signal_t **>(lua_touserdata(L, -1));
    rbx_signal_t *next = signal->next;

    lua_getfield(L, -1, OBF("Disconnect"));
    lua_insert(L, -2);
    lua_pcall(L, 1, 0, 0);

    lua_newtable(L);


    int i = 1;
    while (next) {
        if (!connection_map.contains(next)) {
            const bool is_c = is_c_signal(reinterpret_cast<uintptr_t>(next));
            uintptr_t old_enabled;

            if (!is_c)
                old_enabled = *reinterpret_cast<uintptr_t *>(reinterpret_cast<uintptr_t>(next) + 0x20);
            else
                old_enabled = 0;


            connection_t new_connection{};
            new_connection.is_c = is_c;
            new_connection.old_enabled = old_enabled;
            new_connection.signal = next;
            if (new_connection.is_c){ new_connection.original_signal_c_func = *reinterpret_cast<signal_c_function_t*>(reinterpret_cast<uintptr_t>(next) + 0x8); }


            connection_map[next] = new_connection;
        }
        auto *connection = static_cast<connection_t*>(lua_newuserdata(L, sizeof(connection_t)));
        *connection = connection_map[next];

        lua_newtable(L);

        // Metamethods of our Metatable
        lua_pushstring(L, OBF("ConnectionObject"));
        lua_setfield(L, -2, OBF("__type"));

        lua_pushcclosurek(L, mt_index, nullptr, 0, nullptr);
        lua_setfield(L, -2, OBF("__index"));

        lua_setmetatable(L, -2);
        lua_rawseti(L, -2, i++);

        next = next->next;
    }

    return 1;
}

int firesignal(lua_State *L) {
    luaL_checktype(L, 1, LUA_TUSERDATA);

    if (strcmp(luaL_typename(L, 1), OBF("RBXScriptSignal")) != 0)
        luaL_typeerrorL(L, 1, OBF("RBXScriptSignal"));

    const auto nargs = lua_gettop(L) - 1;
    getconnections(L);

    lua_remove(L, 1);


    lua_pushnil(L);
    while (lua_next(L, -2)) {
        lua_getfield(L, -1, OBF("Fire"));

        for (auto i = 1; i <= nargs; i++)
            lua_pushvalue(L, i);

        lua_call(L, nargs, 0);


        lua_pop(L, 1);
    }
    //lua_pop(L, 2);

    return 0;
}


int replicatesignal(lua_State *L) {
    luaL_checktype(L, 1, LUA_TUSERDATA);

    if (strcmp(luaL_typename(L, 1), OBF("RBXScriptSignal")) != 0)
    {
        luaL_typeerror(L, 1, OBF("RBXScriptSignal"));
    }

    const auto event_instance = *static_cast<uintptr_t *>(lua_touserdata(L, 1));
    const auto desc = *reinterpret_cast<uintptr_t*>(event_instance);

    const auto nargs = lua_gettop(L);

    auto signature = *reinterpret_cast<uintptr_t*>(desc + 0x48);
    std::vector<rbx::rbx_type_t*> types = { };

    uintptr_t start = signature + 0x8;
    while (start)
    {
        if (!is_ptr_valid<uintptr_t>(reinterpret_cast<uintptr_t*>(start)))
            break;

        std::uintptr_t type = *reinterpret_cast<uintptr_t*>(start);
        if (!type || !is_ptr_valid<uintptr_t >(reinterpret_cast<uintptr_t*>(type)) )
            break;

        types.push_back(reinterpret_cast<rbx::rbx_type_t*>(type));

        start += 0x70;
    }

    int total_args = nargs - 1;

    if (total_args != types.size())
        luaL_error(L, OBF("expected %d arguments, got %d arguments"), types.size(), total_args);


    std::vector<rbx::variant_t> arg_values = {};


        for (int i = 2; i < lua_gettop(L) + 1; i++) {
            rbx::variant_t value = {};
            rbx::lua_arguments::get(L, i, &value, true, 0);

            if (value._type != types[i - 2]) {
                int arg_type = lua_type(L, i);

                try {


                    if (arg_type == LUA_TNUMBER || arg_type == LUA_TBOOLEAN) {
                       // rbx::standard_out::printf(1, "type name: %s", types[i - 2]->name.c_str());
                        if (types[i - 2]->name == OBF("float")) {
                            rbx::lua_arguments::variant_cast_float(&value);
                        }
                        else if (strcmp(types[i - 2]->name.c_str(), OBF("int")) == 0) {
                            //rbx::standard_out::printf(1, "wow");
                            rbx::lua_arguments::variant_cast_int(&value);
                        }
                        else if (types[i - 2]->name == OBF("int64")) {
                            rbx::lua_arguments::variant_cast_int64(&value);
                        }
                    }


                }
                catch (std::exception& ex) { }
            }


            arg_values.push_back(value);
        }




    rbx::remoteevent_invocation_targetoptions_t options{};
    options.target = nullptr;
    options.isExcludeTarget = rbx::only_target;


    const auto owning_instance = *reinterpret_cast<uintptr_t*>(event_instance + 0x18);
    const auto owning_instance_vftable = *reinterpret_cast<uintptr_t*>(owning_instance);

    const auto raise_event_invocation = *reinterpret_cast<void(**)(uintptr_t*, uintptr_t*, std::vector<rbx::variant_t>*,
                                                             rbx::remoteevent_invocation_targetoptions_t*)>(owning_instance_vftable + 0x18);
    raise_event_invocation(reinterpret_cast<uintptr_t*>(owning_instance), reinterpret_cast<uintptr_t*>(desc), &arg_values, &options);


    return 0;
}

void environment::load_signals_lib(lua_State *L) {
    lua_pushcclosure(L, getconnections, nullptr, 0);
    lua_setglobal(L, "getconnections");
    lua_pushcclosure(L, firesignal, nullptr, 0);
    lua_setglobal(L, "firesignal");
    lua_pushcclosure(L, replicatesignal, nullptr, 0);
    lua_setglobal(L, "replicatesignal");

    lua_pushcclosure(L, stub, nullptr, 0);
    stub_ref = lua_ref(L, -1);
    lua_pop(L, 1);
}

void environment::reset_signals_lib()
{
    connection_map.clear();
}