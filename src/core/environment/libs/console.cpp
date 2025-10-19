//
// Created by user on 11/05/2025.
//

#include "globals.h"
#include "lapi.h"
#include "lgc.h"
#include "lobject.h"
#include "lstate.h"
#include "ltable.h"
#include "../environment.h"

int console_stub(lua_State *L) {
    return 0;
}

void environment::load_console_lib(lua_State *L) {
    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "rconsolecreate");
    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "consolecreate");

    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "rconsoleshow");
    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "consolehow");

    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "rconsolehide");
    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "consolehide");

    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "rconsoledestroy");
    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "consoledestroy");

    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "rconsolesettitle");
    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "rconsolename");
    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "consolesettitle");

    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "rconsoleprint");
    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "consoleprint");

    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "rconsolewarn");
    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "consolewarn");

    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "rconsoleinfo");
    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "consoleinfo");

    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "rconsoleerror");
    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "rconsoleerr");
    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "consoleerror");

    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "rconsoleinput");
    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "consoleinput");

    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "rconsoleclear");
    lua_pushcclosure(L, console_stub, nullptr, 0);
    lua_setglobal(L, "consoleclear");
}
