cmake_minimum_required(VERSION 3.28)

project(Imgui LANGUAGES CXX)

set(PROJECT Imgui)
set(CMAKE_CXX_STANDARD 23)

file(GLOB IMGUI_SOURCES
        *.cpp
        *.h
        *.hpp
        misc/cpp/*.cpp
        misc/cpp/*.h
        misc/cpp/*.hpp
)

if(WIN32)
    file(GLOB IMGUI_BACKENDS
            backends/imgui_impl_dx*.cpp
            backends/imgui_impl_dx*.h
            backends/imgui_impl_win32.cpp
            backends/imgui_impl_win32.h
    )
    list(APPEND IMGUI_SOURCES ${IMGUI_BACKENDS})
endif()


add_library(${PROJECT} STATIC
        ${IMGUI_SOURCES}
)

string(TOLOWER ${CMAKE_BUILD_TYPE} build_type)

if(WIN32)
    target_link_libraries(${PROJECT} PUBLIC
            dxgi.lib
            d3d11.lib
    )
endif()

target_include_directories(${PROJECT} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
