//
// Created by reveny on 21/04/2025.
//
#include "globals.h"
#include "lapi.h"
#include "lgc.h"
#include "lobject.h"
#include "lstate.h"
#include "../environment.h"
#include "src/rbx/engine/game.h"
#include "lmem.h"
#include "lfunc.h"

int cloneref(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TUSERDATA);

    std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    const auto instance = *static_cast<void **>(lua_touserdata(L, 1));

    lua_pushlightuserdata(L, rbx::lua_bridge::v_push_instance);

    lua_rawget(L, LUA_REGISTRYINDEX);

    lua_pushlightuserdata(L, instance);
    lua_rawget(L, -2);

    lua_pushlightuserdata(L, instance);
    lua_pushnil(L);
    lua_rawset(L, -4);

    rbx::lua_bridge::v_push_instance(L, lua_touserdata(L, 1));
    lua_pushlightuserdata(L, instance);
    lua_pushvalue(L, -3);
    lua_rawset(L, -5);
    return 1;
}

int compareinstances(lua_State *L) {
    lua_check(L, 2);
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TUSERDATA);

    std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    std::string ud2_t = luaL_typename(L, 2);
    if (ud2_t != OBF("Instance"))
        luaL_typeerrorL(L, 2, OBF("Instance"));

    lua_pushboolean(L, *static_cast<void **>(lua_touserdata(L, 1)) == *static_cast<void **>(lua_touserdata(L, 2)));
    return 1;
}

int cache_replace(lua_State *L) {
    lua_check(L, 2);
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TUSERDATA);

    std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    std::string ud2_t = luaL_typename(L, 2);
    if (ud2_t != OBF("Instance"))
        luaL_typeerrorL(L, 2, OBF("Instance"));

    const auto instance = *static_cast<void **>(lua_touserdata(L, 1));


    lua_rawcheckstack(L, 4);
    lua_pushlightuserdata(L, rbx::lua_bridge::v_push_instance);
    lua_gettable(L, LUA_REGISTRYINDEX);

    const auto previous = lua_gettop(L); {
        lua_pushvalue(L, -1);
        lua_pushlightuserdata(L, instance);
        lua_gettable(L, -2);

        if (lua_type(L, -1) == LUA_TNIL)
            luaL_argerror(L, 1, OBF("instance is not cached"));
    }
    lua_settop(L, previous);

    lua_pushlightuserdata(L, instance);
    lua_pushvalue(L, 2);
    lua_settable(L, -3);

    return 0;
}

int cache_invalidate(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TUSERDATA);

    std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    const auto instance = *static_cast<void **>(lua_touserdata(L, 1));

    lua_rawcheckstack(L, 4);
    lua_pushlightuserdata(L, rbx::lua_bridge::v_push_instance);
    lua_gettable(L, LUA_REGISTRYINDEX);

    const auto previous = lua_gettop(L); {
        lua_pushvalue(L, -1);
        lua_pushlightuserdata(L, instance);
        lua_gettable(L, -2);

        if (lua_type(L, -1) == LUA_TNIL)
            luaL_argerror(L, 1, OBF("instance is not cached"));
    }
    lua_settop(L, previous);

    lua_pushlightuserdata(L, instance);
    lua_pushnil(L);
    lua_settable(L, -3);

    return 0;
}

int cache_iscached(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TUSERDATA);

    std::string ud_t = luaL_typename(L, 1);
    if (ud_t != OBF("Instance"))
        luaL_typeerrorL(L, 1, OBF("Instance"));

    const auto instance = *static_cast<void **>(lua_touserdata(L, 1));

    lua_rawcheckstack(L, 3);
    lua_pushlightuserdata(L, rbx::lua_bridge::v_push_instance);
    lua_gettable(L, LUA_REGISTRYINDEX);

    lua_pushlightuserdata(L, instance);
    lua_gettable(L, -2);

    lua_pushboolean(L, lua_type(L, -1) != LUA_TNIL);

    return 1;
}

void environment::load_cache_lib(lua_State *L) {
    static const luaL_Reg cache[] = {
            {"replace", cache_replace},
            {"invalidate", cache_invalidate},
            {"iscached", cache_iscached},
            {nullptr, nullptr}
    };

    static const luaL_Reg instance[] = {
            {"cloneref", cloneref},
            {"compareinstances", compareinstances},
            {nullptr, nullptr}
    };

    lua_newtable(L);

    lua_pushcclosure(L, cache_replace, nullptr, 0);
    lua_setfield(L, -2, "replace");
    lua_pushcclosure(L, cache_invalidate, nullptr, 0);
    lua_setfield(L, -2, "invalidate");
    lua_pushcclosure(L, cache_iscached, nullptr, 0);
    lua_setfield(L, -2, "iscached");

    lua_setglobal(L, "cache");

    lua_pushcclosure(L, cloneref, nullptr, 0);
    lua_setglobal(L, "cloneref");
    lua_pushcclosure(L, cloneref, nullptr, 0);
    lua_setglobal(L, "clonereference");
    lua_pushcclosure(L, compareinstances, nullptr, 0);
    lua_setglobal(L, "compareinstances");

    const int previous_top = lua_gettop(L);

    lua_getglobal(L, OBF("cloneref"));
    lua_getglobal(L, OBF("game"));
    lua_call(L, 1, 1); // cloneref'd game

    lua_getfield(L, -1, OBF("GetService"));
    lua_pushvalue(L, -2);
    lua_pushstring(L, OBF("CoreGui"));
    lua_call(L, 2, 1);
    lua_remove(L, 1);
    lua_getglobal(L, OBF("cloneref"));
    lua_pushvalue(L, 1);
    lua_call(L, 1, 1);

    lua_rawsetfield(L, LUA_REGISTRYINDEX, OBF("hidden_ui_container"));
    lua_settop(L, previous_top);
}