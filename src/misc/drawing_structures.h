//
// Created by user on 01/05/2025.
//

#pragma once

//
// Created by user on 02/04/2025.
//
//
// Created by user on 15/02/2025.
//
#include <ImGui/imgui.h>
#include <cstdint>
#include <lstate.h>
#include <string>
#include <vector>
#include <unordered_map>

struct color_t
{
    float r, g, b;

    operator ImVec4() { return { r, g, b, 1.f }; }

    operator ImVec4() const { return { r, g, b, 1.f }; }

    operator ImColor() const { return { r, g, b, 1.f }; }
};

enum class font_t {
    ui,
    system,
    plex,
    mono_space
};

struct base_t {
    int z_index{ 1 };
    bool visible{ false };
    double transparency{ 1.0 };
    ImColor color{ 0.f, 0.f, 0.f, 0.f };

    virtual int __index(lua_State* state);
    virtual int __newindex(lua_State* state);

    virtual ~base_t() = default;
    virtual void draw_obj() = 0;
};

struct line_t : public base_t {
    double thickness{ 1.0 };
    ImVec2 from{ 0.0, 0.0 };
    ImVec2 to{ 0.0, 0.0 };

    int __index(lua_State* L) override;
    int __newindex(lua_State* L) override;

    virtual ~line_t() = default;
    void draw_obj() override;
};

struct text_t : public base_t {
    std::string str{ "Text" };
    double size{ 12 };
    bool center{ false };
    bool outline{ false };
    ImColor outline_color{ 0.f, 0.f, 0.f, 0.f };
    double outline_opacity{ 0.0 };
    ImVec2 position{ 0.0, 0.0 };
    ImVec2 text_bounds{ 0, 0 }; // Read Only
    font_t font{ font_t::system };

    int __index(lua_State* L) override;
    int __newindex(lua_State* L) override;

    virtual ~text_t() = default;
    void draw_obj() override;
};

struct image_t : public base_t {
    std::string data{};
    ImVec2 size{ 0.0, 0.0 };
    ImVec2 position{ 0.0, 0.0 };
    double rounding{ 0 };

    int __index(lua_State* L) override;
    int __newindex(lua_State* L) override;

    virtual ~image_t() = default;
    void draw_obj() override;
};

struct circle_t : public base_t {
    double thickness{ 1.0 };
    double num_sides{ 360.0 };
    double radius{ 2.0 };
    ImVec2 position{ 0.0, 0.0 };
    bool filled{ false };

    int __index(lua_State* L) override;
    int __newindex(lua_State* L) override;

    virtual ~circle_t() = default;
    void draw_obj() override;
};

struct square_t : public base_t {
    double thickness{ 1.0 };
    ImVec2 size{ 100,100 };
    ImVec2 position{ 0.0, 0.0 };
    bool filled{ false };

    int __index(lua_State* L) override;
    int __newindex(lua_State* L) override;

    virtual ~square_t() = default;
    void draw_obj() override;
};

struct quad_t : public base_t {
    double thickness{ 1.0 };
    ImVec2 point_a{ 0.0, 0.0 };
    ImVec2 point_b{ 0.0, 0.0 };
    ImVec2 point_c{ 0.0, 0.0 };
    ImVec2 point_d{ 0.0, 0.0 };
    bool filled{ false };

    int __index(lua_State* L) override;
    int __newindex(lua_State* L) override;

    virtual ~quad_t() = default;
    void draw_obj() override;
};

struct triangle_t : public base_t {
    double thickness{ 1.0 };
    ImVec2 point_a{ 0.0, 0.0 };
    ImVec2 point_b{ 0.0, 0.0 };
    ImVec2 point_c{ 0.0, 0.0 };
    bool filled{ false };

    int __index(lua_State* L) override;
    int __newindex(lua_State* L) override;

    virtual ~triangle_t() = default;
    void draw_obj() override;
};

inline std::vector< base_t*> drawing_cache;
inline std::unordered_map< base_t*, int > key_map;
