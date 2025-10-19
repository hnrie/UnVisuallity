cmake_minimum_required(VERSION 3.29)
project(Visual)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_STANDARD 23)

# disable and enable shi for performance :P
add_compile_options(
        #/MT # not supported by our mapper atm... :(
        /MP
        /cgthreads8
        /arch:AVX2
)

add_library(${PROJECT_NAME} SHARED entry.cpp
        src/core/execution/execution.cpp
        src/core/execution/execution.h
        src/rbx/engine/game.h
        src/rbx/engine/luau.h
        src/rbx/engine/hyperion.h
        src/rbx/taskscheduler/taskscheduler.cpp
        src/rbx/taskscheduler/taskscheduler.h
        src/rbx/engine/game_structures.h
        globals.h
        entry.h
        src/core/environment/environment.cpp
        src/core/environment/environment.h
        src/core/environment/libs/http.cpp
        src/core/environment/libs/misc.cpp
        src/core/environment/libs/closure.cpp
        src/core/render/render.cpp
        src/core/render/render.h
        src/core/render/user_interface/user_interface.cpp
        src/core/render/user_interface/user_interface.h
        src/core/environment/libs/debug.cpp
        src/core/environment/libs/filesystem.cpp
        src/core/environment/libs/metatables.cpp
        src/core/environment/libs/script.cpp
        src/core/environment/libs/cache.cpp
        src/core/communication/communication.cpp
        src/core/communication/communication.h
        dependencies/curl_wrapper.h
        dependencies/http_status.h
        src/core/environment/libs/crypt.cpp
        src/core/environment/libs/signals.cpp
        src/core/environment/libs/drawing.cpp
        src/misc/drawing_structures.h
        src/misc/inter.h
        src/core/environment/libs/websockets.cpp
        src/core/environment/libs/websockets.h
        src/core/environment/libs/input.cpp
        src/core/environment/libs/raknet.cpp
        src/core/environment/libs/actor.cpp
        src/core/environment/libs/console.cpp
        src/rbx/taskscheduler/yielding.cpp
        src/rbx/taskscheduler/yielding.h
        src/core/teleport_handler/teleport_handler.cpp
        src/core/teleport_handler/teleport_handler.h
        dependencies/pattern_scanner/lxzp_scanner.cpp
        dependencies/pattern_scanner/lxzp_scanner.hpp
        src/core/render/user_interface/key_interface.cpp
        src/core/render/user_interface/key_interface.hpp
)


# DEPENDENCIES
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")

## LUAU
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/dependencies/luau")
target_link_libraries(${PROJECT_NAME} PUBLIC Luau.Compiler Luau.Ast Luau.VM Luau.VM.Internals)

## IMGUI
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/dependencies/cpr")
target_link_libraries(${PROJECT_NAME} PUBLIC cpr::cpr)

## IMGUI
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/dependencies/imgui")
target_link_libraries(${PROJECT_NAME} PUBLIC Imgui)

# cryptopp
add_subdirectory("Dependencies/cryptopp")
target_link_libraries(${PROJECT_NAME} PUBLIC cryptopp::cryptopp)

# BLAKE3
add_subdirectory("Dependencies/BLAKE3")
target_link_libraries(${PROJECT_NAME} PUBLIC blake3)

# IXWebsocket
add_subdirectory("Dependencies/ixwebsocket")
target_link_libraries(${PROJECT_NAME} PUBLIC ixwebsocket::ixwebsocket)


# LZ4
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lz4")
target_link_libraries(${PROJECT_NAME} PUBLIC lz4)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/zstd")
target_link_libraries(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/zstd/lib/libzstd_static.lib")

## LIBSODIUM
add_definitions(-DSODIUM_STATIC)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/libsodium/include")
target_link_libraries(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/libsodium/libsodium.lib")
