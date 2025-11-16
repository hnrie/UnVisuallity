//
// Created by savage on 17.04.2025.
//

#pragma once

#include <memory>

#ifdef _WIN32
#include <Windows.h>
#else
#include <cstdint>

using HMODULE = void*;
using DWORD = unsigned long;
using LPVOID = void*;
using BOOL = int;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef WINAPI
#define WINAPI
#endif

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef CALLBACK
#define CALLBACK
#endif

#endif  // _WIN32

