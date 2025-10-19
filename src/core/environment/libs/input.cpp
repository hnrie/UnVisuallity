//
// Created by user on 04/05/2025.
//

#include "globals.h"
#include "lapi.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"
#include "../environment.h"

DWORD process_id = 0;
HWND current_window = nullptr;

bool is_roblox_active() {

    HWND fore_ground = GetForegroundWindow();

    DWORD fore_ground_process_id;
    GetWindowThreadProcessId(fore_ground, &fore_ground_process_id);

    return (fore_ground_process_id == process_id);
}

int isrbxactive(lua_State* L) {
    lua_check(L, 0);
    lua_pushboolean(L, GetForegroundWindow() == current_window);
    return 1;
}

int keypress(lua_State* L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TNUMBER);
    const UINT key = lua_tointeger(L, 1);

    if (is_roblox_active())
        keybd_event(0, static_cast<BYTE>(MapVirtualKeyA(key, MAPVK_VK_TO_VSC)), KEYEVENTF_SCANCODE, 0);

    return 0;
}

int keytap(lua_State* L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TNUMBER);
    const UINT key = lua_tointeger(L, 1);

    if (!is_roblox_active())
        return 0;

    keybd_event(0, MapVirtualKeyA(key, MAPVK_VK_TO_VSC), KEYEVENTF_SCANCODE, 0);
    keybd_event(0, MapVirtualKeyA(key, MAPVK_VK_TO_VSC), KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP, 0);

    return 0;
};

int keyrelease(lua_State* L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TNUMBER);
    const UINT key = lua_tointeger(L, 1);

    if (is_roblox_active())
        keybd_event(0, static_cast<BYTE>(MapVirtualKeyA(key, MAPVK_VK_TO_VSC)), KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP, 0);

    return 0;
}

int mouse1click(lua_State* L) {
    lua_check(L, 0);
    if (is_roblox_active())
        mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

    return 0;
}

int mouse1press(lua_State* L) {
    lua_check(L, 0);
    if (is_roblox_active())
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);

    return 0;
}

int mouse1release(lua_State* L) {
    lua_check(L, 0);
    if (is_roblox_active())
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

    return 0;
}

int mouse2click(lua_State* L) {
    lua_check(L, 0);
    if (is_roblox_active())
        mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);

    return 0;
}

int mouse2press(lua_State* L) {
    lua_check(L, 0);
    if (is_roblox_active())
        mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);

    return 0;
}

int mouse2release(lua_State* L) {
    lua_check(L, 0);
    if (is_roblox_active())
        mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);

    return 0;
}

int mousemoveabs(lua_State* L) {
    lua_check(L, 2);
    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);

    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);

    if (!is_roblox_active())
        return 0;

    int width = GetSystemMetrics(SM_CXSCREEN) - 1;
    int height = GetSystemMetrics(SM_CYSCREEN) - 1;

    RECT c_rect;
    GetClientRect(GetForegroundWindow(), &c_rect);

    POINT point{ c_rect.left, c_rect.top };
    ClientToScreen(GetForegroundWindow(), &point);

    x = (x + point.x) * (65535 / width);
    y= (y + point.y) * (65535 / height);

    mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, x, y, 0, 0);
    return 0;
}

int mousemoverel(lua_State* L) {
    lua_check(L, 2);
    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);

    const int x = lua_tointeger(L, 1);
    const int y = lua_tointeger(L, 2);

    if (is_roblox_active())
        mouse_event(MOUSEEVENTF_MOVE, x, y, 0, 0);

    return 0;
}

int mousescroll(lua_State* L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TNUMBER);

    const int scroll_data = lua_tointeger(L, 1);

    if (is_roblox_active())
        mouse_event(MOUSEEVENTF_WHEEL, 0, 0, scroll_data, 0);

    return 0;
}

void environment::load_input_lib(lua_State *L) {
    current_window = FindWindowA(nullptr, OBF("Roblox"));
    process_id = GetCurrentProcessId();

    lua_pushcclosure(L, isrbxactive, nullptr, 0);
    lua_setglobal(L, "isrbxactive");

    lua_pushcclosure(L, mouse1click, nullptr, 0);
    lua_setglobal(L, "mouse1click");

    lua_pushcclosure(L, mouse1press, nullptr, 0);
    lua_setglobal(L, "mouse1press");

    lua_pushcclosure(L, mouse1release, nullptr, 0);
    lua_setglobal(L, "mouse1release");

    lua_pushcclosure(L, mouse2click, nullptr, 0);
    lua_setglobal(L, "mouse2click");

    lua_pushcclosure(L, mouse2press, nullptr, 0);
    lua_setglobal(L, "mouse2press");

    lua_pushcclosure(L, mouse2release, nullptr, 0);
    lua_setglobal(L, "mouse2release");

    lua_pushcclosure(L, mousemoveabs, nullptr, 0);
    lua_setglobal(L, "mousemoveabs");

    lua_pushcclosure(L, mousemoverel, nullptr, 0);
    lua_setglobal(L, "mousemoverel");

    lua_pushcclosure(L, mousescroll, nullptr, 0);
    lua_setglobal(L, "mousescroll");

    lua_pushcclosure(L, keypress, nullptr, 0);
    lua_setglobal(L, "keypress");

    lua_pushcclosure(L, keyrelease, nullptr, 0);
    lua_setglobal(L, "keyrelease");

    lua_pushcclosure(L, keypress, nullptr, 0);
    lua_setglobal(L, "keyclick");

    lua_pushcclosure(L, keyrelease, nullptr, 0);
    lua_setglobal(L, "keyrelease");

    lua_pushcclosure(L, isrbxactive, nullptr, 0);
    lua_setglobal(L, "isgameactive");

    lua_pushcclosure(L, isrbxactive, nullptr, 0);
    lua_setglobal(L, "iswindowactive");
}