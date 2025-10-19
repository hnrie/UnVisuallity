//
// Created by user on 01/05/2025.
//

#include "globals.h"
#include "lapi.h"
#include "lgc.h"
#include "lobject.h"
#include "lstate.h"
#include "ltable.h"
#include "../environment.h"
#include "src/misc/drawing_structures.h"
#include "imgui_internal.h"
#include "src/core/render/render.h"
#include "src/core/render/user_interface/user_interface.h"

inline void make_vector2(lua_State* L, ImVec2 Vec)
{
    lua_getglobal(L, OBF("Vector2"));
    lua_getfield(L, -1, OBF("new"));

    lua_pushnumber(L, Vec.x);
    lua_pushnumber(L, Vec.y);

    lua_pcall(L, 2, 1, 0);
}

inline void make_color3(lua_State* L, ImColor col)
{
    lua_getglobal(L, OBF("Color3"));
    lua_getfield(L, -1, OBF("new"));

    const auto [r, g, b, a] = col.Value;

    lua_pushnumber(L, r);
    lua_pushnumber(L, g);
    lua_pushnumber(L, b);

    lua_pcall(L, 3, 1, 0);
}



void triangle_t::draw_obj()
{
    auto drawing_list = ImGui::GetBackgroundDrawList();


    const auto point_a = this->point_a;
    const auto point_b = this->point_b;
    const auto point_c = this->point_c;

    if (!this->visible) return;

    if (this->filled) {
        drawing_list->AddTriangleFilled(point_a, point_b, point_c,
                                       ImColor(this->color.Value.x, this->color.Value.y, this->color.Value.z, this->transparency));
    }
    else {
        drawing_list->AddTriangle(point_a, point_b, point_c,
                                 ImColor(this->color.Value.x, this->color.Value.y, this->color.Value.z, this->transparency), this->thickness);
    }
}

void quad_t::draw_obj()
{
    auto drawing_list = ImGui::GetBackgroundDrawList();


    const auto point_a = this->point_a;
    const auto point_b = this->point_b;
    const auto point_c = this->point_c;
    const auto point_d = this->point_d;

    if (!this->visible) return;

    if (this->filled) {
        drawing_list->AddQuadFilled(point_a, point_b, point_c, point_d,
                                   ImColor(this->color.Value.x, this->color.Value.y, this->color.Value.z, this->transparency));
    }
    else {
        drawing_list->AddQuad(point_a, point_b, point_c, point_d,
                             ImColor(this->color.Value.x, this->color.Value.y, this->color.Value.z, this->transparency), this->thickness);
    }
}

void square_t::draw_obj()
{
    auto drawing_list = ImGui::GetBackgroundDrawList();

    if (!this->visible) return;

    ImVec2 top_left = { this->position.x, this->position.y };
    ImVec2 bottom_right = { this->position.x + this->size.x, this->position.y + this->size.y };

    ImU32 color = ImColor(this->color.Value.x, this->color.Value.y, this->color.Value.z, this->transparency);

    if (this->filled) {
        drawing_list->AddRectFilled(top_left, bottom_right, color);
    }
    else {
        drawing_list->AddRect(top_left, bottom_right, color, 0.0f, ImDrawFlags_None, this->thickness);
    }

}

void circle_t::draw_obj()
{
    auto drawing_list = ImGui::GetBackgroundDrawList();
    
    const auto pos = this->position;

    if (!this->visible) return;

    if (this->filled) {
        drawing_list->AddCircleFilled(
                { pos.x, pos.y }, this->radius, ImColor(this->color.Value.x, this->color.Value.y, this->color.Value.z, this->transparency)
                , this->num_sides);
    }
    else {
        drawing_list->AddCircle(
                { pos.x, pos.y }, this->radius, ImColor(this->color.Value.x, this->color.Value.y, this->color.Value.z, this->transparency)
                , this->num_sides, this->thickness);
    }
}

ImTextureID create_image_texture(const std::vector<uint8_t> &data, const int width,
                                              const int height) {

    const auto device = niggachain::Renderer->get_device();

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D *texture;
    D3D11_SUBRESOURCE_DATA sub_resource;
    sub_resource.pSysMem = data.data();
    sub_resource.SysMemPitch = width * 4;
    sub_resource.SysMemSlicePitch = 0;
    device->CreateTexture2D(&desc, &sub_resource, &texture);

    ID3D11ShaderResourceView *textureView;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;

    device->CreateShaderResourceView(texture, &srvDesc, &textureView);
    texture->Release();

    return reinterpret_cast<ImTextureID>(textureView);
}

void image_t::draw_obj()
{

}

ImVec2 custom_calc_text_sz(const char* text, float sz, const char* text_end = NULL, bool hide_text_after_double_hash = false, float wrap_width = -1.0f)
{
    ImGuiContext& g = *GImGui;

    const char* text_display_end;
    if (hide_text_after_double_hash)
        text_display_end = ImGui::FindRenderedTextEnd(text, text_end);
    else
        text_display_end = text_end;

    ImFont* font = g.Font;
    const float font_size = sz ? sz : g.FontSize;
    if (text == text_display_end)
        return ImVec2(0.0f, font_size);
    ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, wrap_width, text, text_display_end, NULL);

    text_size.x = IM_FLOOR(text_size.x + 0.99999f);

    return text_size;
}

void text_t::draw_obj() {
    auto drawing_list = ImGui::GetBackgroundDrawList();

    if (!this->visible) return;

    auto text_sz = custom_calc_text_sz(this->str.c_str(), this->size, nullptr, false, 0.0f);



    auto pos_x = this->position.x;
    auto pos_y = this->position.y;

    if (this->center) {
        pos_x -= text_sz.x / 2.0f;
    }


    if (this->outline) {
        
        const auto Col = ImColor(this->outline_color.Value.x, this->outline_color.Value.y, this->outline_color.Value.z, this->transparency);

        ImVec2 pos_1({ pos_x, pos_y });
        ImVec2 pos_2({ pos_x, pos_y });
        ImVec2 pos_3({ pos_x, pos_y });
        ImVec2 pos_4({ pos_x, pos_y });

        pos_1.x -= 1;
        pos_1.y -= 1;

        drawing_list->AddText(renderer::sigma_font, this->size, pos_1, Col, this->str.c_str(), nullptr);
        pos_2.x -= 1;
        pos_2.y += 1;
        drawing_list->AddText(renderer::sigma_font, this->size, pos_2, Col, this->str.c_str(), nullptr);
        pos_3.x += 1;
        pos_3.y -= 1;
        drawing_list->AddText(renderer::sigma_font, this->size, pos_3, Col, this->str.c_str(), nullptr);
        pos_4.x += 1;
        pos_4.y += 1;
        drawing_list->AddText(renderer::sigma_font, this->size, pos_4, Col, this->str.c_str(), nullptr);
    }

    drawing_list->AddText(renderer::sigma_font, this->size, { pos_x, pos_y }, ImColor(this->color.Value.x, this->color.Value.y, this->color.Value.z, this->transparency), this->str.c_str(), nullptr);
}

void line_t::draw_obj() {
    auto drawing_list = ImGui::GetBackgroundDrawList();

    if (!this->visible) return;
    drawing_list->AddLine(
            { from.x, from.y }, { to.x, to.y },
            ImColor(this->color.Value.x, this->color.Value.y, this->color.Value.z, this->transparency), this->thickness
    );
}

int clear_drawing_object(lua_State* L)
{
    const auto item = reinterpret_cast<base_t*>(lua_touserdata(L, 1));
    if (const auto i = std::find(drawing_cache.begin(), drawing_cache.end(), item); i != drawing_cache.end())
    {
        drawing_cache.erase(i);

        lua_unref(L, key_map.at(item));

        key_map.erase(item);
    }

    return 0;
}

int cleardrawcache(lua_State* L)
{
    lua_check(L, 0);
    for (auto & i : drawing_cache)
    {
        lua_unref(L, key_map.at(i));

        key_map.erase(i);
    }

    drawing_cache.clear();

    return 0;
}

int drawing_new(lua_State* L)
{
    lua_check(L, 1);
    luaL_checktype(L, 1 ,LUA_TSTRING);

    const auto type = luaL_checkstring(L, 1);

    base_t* drawing_object = nullptr;

    if (!std::strcmp(type, OBF("Text")))
    {
        drawing_object = new (lua_newuserdata(L, sizeof(text_t))) text_t;
    }
    else if (!std::strcmp(type, OBF("Circle")))
    {
        drawing_object = new (lua_newuserdata(L, sizeof(circle_t))) circle_t;
    }
    else if (!std::strcmp(type, OBF("Square")))
    {
        drawing_object = new (lua_newuserdata(L, sizeof(square_t))) square_t;
    }
    else if (!std::strcmp(type, OBF("Line")))
    {
        drawing_object = new (lua_newuserdata(L, sizeof(line_t))) line_t;
    }
    else if (!std::strcmp(type, OBF("Image")))
    {
        drawing_object = new (lua_newuserdata(L, sizeof(image_t))) image_t;
    }
    else if (!std::strcmp(type, OBF("Quad")))
    {
        drawing_object = new (lua_newuserdata(L, sizeof(quad_t))) quad_t;
    }
    else if (!std::strcmp(type, OBF("Triangle")))
    {
        drawing_object = new (lua_newuserdata(L, sizeof(triangle_t))) triangle_t;
    }
    else
    {
        luaL_argerror(L, 1, OBF("invalid drawing type"));
    }

    luaL_getmetatable(L, OBF("DrawingObject"));
    lua_setmetatable(L, -2);


    key_map.insert({ drawing_object, lua_ref(L, -1) });
    drawing_cache.push_back(drawing_object);

    return 1;
}

int __index(lua_State* L)
{
    return static_cast<base_t*>(lua_touserdata(L, 1))->__index(L);
}

int __newindex(lua_State* L)
{
    return static_cast<base_t*>(lua_touserdata(L, 1))->__newindex(L);
}

int isrenderobj(lua_State* L)
{
    lua_check(L, 1);
    luaL_argexpected(L, lua_isuserdata(L, 1) || lua_istable(L, 1), 1, OBF("userdata or table"));//luaL_checktype(L, 1, LUA_TUSERDATA);

    const auto ud = static_cast<base_t*>(lua_touserdata(L, 1));

    lua_pushboolean(L, key_map.contains(ud));

    return 1;
}

int getrenderproperty(lua_State* L)
{
    lua_check(L, 2);
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TSTRING);

    lua_getfield(L, 1, luaL_checkstring(L, 2));

    return lua_gettop(L) - 2;
}

int setrenderproperty(lua_State* L)
{
    lua_check(L, 3);
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TSTRING);
    luaL_checkany(L, 3);

    lua_pushvalue(L, 3);
    lua_setfield(L, 1, luaL_checkstring(L, 2));

    return 0;
}

int circle_t::__index(lua_State* L)
{
    const auto drawing_object = static_cast<circle_t*>(lua_touserdata(L, 1));

    const auto type = luaL_checkstring(L, 2);

    if (!std::strcmp(type, OBF("Position")))
    {
        make_vector2(L, drawing_object->position);
        return 1;
    }
    else if (!std::strcmp(type, OBF("Radius")))
    {
        lua_pushnumber(L, drawing_object->radius);
        return 1;
    }
    else if (!std::strcmp(type, OBF("NumSides")))
    {
        lua_pushnumber(L, drawing_object->num_sides);
        return 1;
    }
    else if (!std::strcmp(type, OBF("Thickness")))
    {
        lua_pushnumber(L, drawing_object->thickness);
        return 1;
    }
    else if (!std::strcmp(type, OBF("Filled")))
    {
        lua_pushboolean(L, drawing_object->filled);
        return 1;
    }

    return base_t::__index(L);
}

int circle_t::__newindex(lua_State* L)
{
    const auto drawing_object = static_cast<circle_t*>(lua_touserdata(L, 1));

    const auto type = luaL_checkstring(L, 2);

    if (!std::strcmp(type, OBF("Position")))
    {
        drawing_object->position = *reinterpret_cast<const ImVec2*>(lua_topointer(L, 3));
        return 0;
    }
    else if (!std::strcmp(type, OBF("Radius")))
    {
        drawing_object->radius = luaL_checknumber(L, 3);
        return 0;
    }
    else if (!std::strcmp(type, OBF("Thickness")))
    {
        drawing_object->thickness = luaL_checknumber(L, 3);

        return 0;
    }
    else if (!std::strcmp(type, OBF("NumSides")))
    {
        drawing_object->num_sides = luaL_checknumber(L, 3);
        return 0;
    }
    else if (!std::strcmp(type, OBF("Filled")))
    {
        drawing_object->filled = luaL_checkboolean(L, 3);
        return 0;
    }

    return base_t::__newindex(L);
}

int square_t::__index(lua_State* L)
{
    const auto drawing_object = static_cast<square_t*>(lua_touserdata(L, 1));

    const auto type = luaL_checkstring(L, 2);

    if (!std::strcmp(type, OBF("Position")))
    {
        make_vector2(L, drawing_object->position);
        return 1;
    }
    else if (!std::strcmp(type, OBF("Size")))
    {
        make_vector2(L, drawing_object->size);
        return 1;
    }
    else if (!std::strcmp(type, OBF("Thickness")))
    {
        lua_pushnumber(L, drawing_object->thickness);
        return 1;
    }
    else if (!std::strcmp(type, OBF("Filled")))
    {
        lua_pushboolean(L, drawing_object->filled);
        return 1;
    }

    return base_t::__index(L);
}

int square_t::__newindex(lua_State* L)
{
    const auto drawing_object = static_cast<square_t*>(lua_touserdata(L, 1));

    const auto type = luaL_checkstring(L, 2);

    if (!std::strcmp(type, OBF("Position")))
    {
        drawing_object->position = *reinterpret_cast<const ImVec2*>(lua_topointer(L, 3));

        return 0;
    }
    else if (!std::strcmp(type, OBF("Size")))
    {
        drawing_object->size = *reinterpret_cast<const ImVec2*>(lua_topointer(L, 3));

        return 0;
    }
    else if (!std::strcmp(type, OBF("Thickness")))
    {
        drawing_object->thickness = luaL_checknumber(L, 3);

        return 0;
    }
    else if (!std::strcmp(type, OBF("Filled")))
    {
        drawing_object->filled = luaL_checkboolean(L, 3);

        return 0;
    }

    return base_t::__newindex(L);
}

int image_t::__index(lua_State* L)
{
    const auto drawing_object = static_cast<image_t*>(lua_touserdata(L, 1));

    const auto type = luaL_checkstring(L, 2);

    if (!std::strcmp(type, OBF("Data"))) {
        lua_pushstring(L, drawing_object->data.data());
        return 1;
    } else if (!std::strcmp(type, OBF("Size")))
    {
        make_vector2(L, drawing_object->size);
        return 1;
    } else if (!std::strcmp(type, OBF("Position")))
    {
        make_vector2(L, drawing_object->position);
        return 1;
    }
    else if (!std::strcmp(type, OBF("Rounding")))
    {
        lua_pushnumber(L, drawing_object->rounding);
        return 1;
    }

    return base_t::__index(L);
}

int image_t::__newindex(lua_State* L)
{
    const auto drawing_object = static_cast<image_t*>(lua_touserdata(L, 1));

    const auto type = luaL_checkstring(L, 2);

    if (!std::strcmp(type, OBF("Data"))) {
        drawing_object->data = luaL_checkstring(L, 3);
        return 0;
    } else if (!std::strcmp(type, OBF("Size")))
    {
        drawing_object->size = *reinterpret_cast<const ImVec2*>(lua_topointer(L, 3));

        return 0;
    } else if (!std::strcmp(type, OBF("Position")))
    {
        drawing_object->position = *reinterpret_cast<const ImVec2*>(lua_topointer(L, 3));

        return 0;
    }
    else if (!std::strcmp(type, OBF("Rounding")))
    {
        drawing_object->rounding = luaL_checknumber(L, 3);

        return 0;
    }

    return base_t::__newindex(L);
}

int quad_t::__index(lua_State* L)
{
    const auto drawing_object = static_cast<quad_t*>(lua_touserdata(L, 1));

    const auto type = luaL_checkstring(L, 2);

    if (!std::strcmp(type, OBF("PointA")))
    {
        make_vector2(L, drawing_object->point_a);
        return 1;
    }
    else if (!std::strcmp(type, OBF("PointB")))
    {
        make_vector2(L, drawing_object->point_b);
        return 1;
    }
    else if (!std::strcmp(type, OBF("PointC")))
    {
        make_vector2(L, drawing_object->point_c);
        return 1;
    }
    else if (!std::strcmp(type, OBF("PointD")))
    {
        make_vector2(L, drawing_object->point_d);

        return 1;
    }
    else if (!std::strcmp(type, OBF("Thickness")))
    {
        lua_pushnumber(L, drawing_object->thickness);

        return 1;
    }
    else if (!std::strcmp(type, OBF("Filled")))
    {
        lua_pushboolean(L, drawing_object->filled);

        return 1;
    }

    return base_t::__index(L);
}

int quad_t::__newindex(lua_State* L)
{
    const auto drawing_object = static_cast<quad_t*>(lua_touserdata(L, 1));

    const auto type = luaL_checkstring(L, 2);

    if (!std::strcmp(type, OBF("Filled")))
    {
        drawing_object->filled = luaL_checkboolean(L, 3);

        return 0;
    }
    else if (!std::strcmp(type, OBF("PointA")))
    {
        drawing_object->point_a = *reinterpret_cast<const ImVec2*>(lua_topointer(L, 3));

        return 0;
    }
    else if (!std::strcmp(type, OBF("PointB")))
    {
        drawing_object->point_b = *reinterpret_cast<const ImVec2*>(lua_topointer(L, 3));

        return 0;
    }
    else if (!std::strcmp(type, OBF("PointC")))
    {
        drawing_object->point_c = *reinterpret_cast<const ImVec2*>(lua_topointer(L, 3));

        return 0;
    }
    else if (!std::strcmp(type, OBF("PointD")))
    {
        drawing_object->point_d = *reinterpret_cast<const ImVec2*>(lua_topointer(L, 3));

        return 0;
    }
    else if (!std::strcmp(type, OBF("Thickness")))
    {
        drawing_object->thickness = luaL_checknumber(L, 3);

        return 0;
    }

    return base_t::__newindex(L);
}

int triangle_t::__index(lua_State* L)
{
    const auto drawing_object = static_cast<triangle_t*>(lua_touserdata(L, 1));

    const auto type = luaL_checkstring(L, 2);

    if (!std::strcmp(type, OBF("PointA")))
    {
        make_vector2(L, drawing_object->point_a);

        return 1;
    }
    else if (!std::strcmp(type, OBF("PointB")))
    {
        make_vector2(L, drawing_object->point_b);

        return 1;
    }
    else if (!std::strcmp(type, OBF("PointC")))
    {
        make_vector2(L, drawing_object->point_c);

        return 1;
    }
    else if (!std::strcmp(type, OBF("Thickness")))
    {
        lua_pushnumber(L, drawing_object->thickness);

        return 1;
    }
    else if (!std::strcmp(type, OBF("Filled")))
    {
        lua_pushboolean(L, drawing_object->filled);

        return 1;
    }

    return base_t::__index(L);
}

int triangle_t::__newindex(lua_State* L)
{
    const auto drawing_object = static_cast<triangle_t*>(lua_touserdata(L, 1));

    const auto type = luaL_checkstring(L, 2);

    if (!std::strcmp(type, OBF("Filled")))
    {
        drawing_object->filled = luaL_checkboolean(L, 3);

        return 0;
    }
    else if (!std::strcmp(type, OBF("PointA")))
    {
        drawing_object->point_a = *reinterpret_cast<const ImVec2*>(lua_topointer(L, 3));

        return 0;
    }
    else if (!std::strcmp(type, OBF("PointB")))
    {
        drawing_object->point_b = *reinterpret_cast<const ImVec2*>(lua_topointer(L, 3));

        return 0;
    }
    else if (!std::strcmp(type, OBF("PointC")))
    {
        drawing_object->point_c = *reinterpret_cast<const ImVec2*>(lua_topointer(L, 3));

        return 0;
    }
    else if (!std::strcmp(type, OBF("Thickness")))
    {
        drawing_object->thickness = luaL_checknumber(L, 3);

        return 0;
    }

    return base_t::__newindex(L);
}

int text_t::__index(lua_State* L)
{
    const auto drawing_object = static_cast<text_t*>(lua_touserdata(L, 1));

    const auto type = luaL_checkstring(L, 2);

    if (!std::strcmp(type, OBF("Text")))
    {
        lua_pushstring(L, drawing_object->str.c_str());

        return 1;
    }
    else if (!std::strcmp(type, OBF("OutlineColor")))
    {
        make_color3(L, drawing_object->outline_color);

        return 1;
    }
    else if (!std::strcmp(type, OBF("Center")))
    {
        lua_pushboolean(L, drawing_object->center);

        return 1;
    }
    else if (!std::strcmp(type, OBF("Outline")))
    {
        lua_pushboolean(L, drawing_object->outline);

        return 1;
    }
    else if (!std::strcmp(type, OBF("Position")))
    {
        make_vector2(L, drawing_object->position);

        return 1;
    }
    else if (!std::strcmp(type, OBF("Size")))
    {
        lua_pushnumber(L, drawing_object->size);

        return 1;
    }
    else if (!std::strcmp(type, OBF("TextBounds")))
    {
        make_vector2(L, custom_calc_text_sz(this->str.c_str(), this->size, nullptr, false, 0.0f));

        return 1;
    }
    else if (!std::strcmp(type, OBF("OutlineOpacity")))
    {
        lua_pushnumber(L, drawing_object->outline_opacity);

        return 1;
    }
    else if (!std::strcmp(type, OBF("Font")))
    {
        const char* font_type = nullptr;

        switch(drawing_object->font) {
            case font_t::ui:
                font_type = OBF("UI");
                break;
            case font_t::system:
                font_type = OBF("System");
                break;
            case font_t::plex:
                font_type = OBF("Plex");
                break;
            case font_t::mono_space:
                font_type = OBF("Monospace");
                break;
            default:
                font_type = OBF("System");
                break;
        }

        lua_getglobal(L, OBF("Drawing"));
        lua_getfield(L, -1, OBF("Fonts"));
        lua_remove(L, -2);
        lua_getfield(L, -1, font_type);
        lua_remove(L, -2);

        return 1;
    }

    return base_t::__index(L);
}

int text_t::__newindex(lua_State* L)
{
    const auto drawing_object = static_cast<text_t*>(lua_touserdata(L, 1));

    const auto type = luaL_checkstring(L, 2);

    if (!std::strcmp(type, OBF("Text")))
    {
        drawing_object->str = luaL_checkstring(L, 3);

        return 0;
    }
    else if (!std::strcmp(type, OBF("OutlineColor")))
    {
        drawing_object->outline_color = *reinterpret_cast<const color_t*>(lua_topointer(L, 3));

        return 0;
    }
    else if (!std::strcmp(type, OBF("Center")))
    {
        drawing_object->center = luaL_checkboolean(L, 3);

        return 0;
    }
    else if (!std::strcmp(type, OBF("Outline")))
    {
        drawing_object->outline = luaL_checkboolean(L, 3);

        return 0;
    }
    else if (!std::strcmp(type, OBF("Position")))
    {
        drawing_object->position = *reinterpret_cast<const ImVec2*>(lua_topointer(L, 3));

        return 0;
    }
    else if (!std::strcmp(type, OBF("Size")))
    {
        drawing_object->size = luaL_checknumber(L, 3);

        return 0;
    }
    else if (!std::strcmp(type, OBF("OutlineOpacity")))
    {
        drawing_object->outline_opacity = luaL_checknumber(L, 3);

        return 0;
    }
    else if (!std::strcmp(type, OBF("Font")))
    {
        int new_font = luaL_checknumber(L, 3);

        switch(new_font) {
            case 0:
                drawing_object->font = font_t::ui;
                break;
            case 1:
                drawing_object->font = font_t::system;
                break;
            case 2:
                drawing_object->font = font_t::plex;
                break;
            case 3:
                drawing_object->font = font_t::mono_space;
                break;
            default:
                break;
        }

        return 0;
    }

    return base_t::__newindex(L);
}

int line_t::__index(lua_State* L)
{
    const auto drawing_object = static_cast<line_t*>(lua_touserdata(L, 1));

    const auto type = luaL_checkstring(L, 2);

    if (!std::strcmp(type, OBF("From")))
    {
        make_vector2(L, drawing_object->from);

        return 1;
    }
    else if (!std::strcmp(type, OBF("To")))
    {
        make_vector2(L, drawing_object->to);

        return 1;
    }
    else if (!std::strcmp(type, OBF("Thickness")))
    {
        lua_pushnumber(L, drawing_object->thickness);

        return 1;
    }

    return base_t::__index(L);
}

int line_t::__newindex(lua_State* L)
{
    const auto drawing_object = static_cast<line_t*>(lua_touserdata(L, 1));

    const auto type = luaL_checkstring(L, 2);

    if (!std::strcmp(type, OBF("From")))
    {
        drawing_object->from = *reinterpret_cast<const ImVec2*>(lua_topointer(L, 3));

        return 0;
    }
    else if (!std::strcmp(type, OBF("To")))
    {
        drawing_object->to = *reinterpret_cast<const ImVec2*>(lua_topointer(L, 3));

        return 0;
    }
    else if (!std::strcmp(type, OBF("Thickness")))
    {
        drawing_object->thickness = luaL_checknumber(L, 3);

        return 0;
    }

    return base_t::__newindex(L);
}

int base_t::__index(lua_State* L)
{
    const auto drawing_object = static_cast<base_t*>(lua_touserdata(L, 1));

    const auto type = luaL_checkstring(L, 2);

    if (!std::strcmp(type, OBF("ZIndex")))
    {
        lua_pushinteger(L, drawing_object->z_index);
    }
    else if (!std::strcmp(type, OBF("Visible")))
    {
        lua_pushboolean(L, drawing_object->visible);
    }
    else if (!std::strcmp(type, OBF("Transparency")))
    {
        lua_pushnumber(L, drawing_object->transparency);
    }
    else if (!std::strcmp(type, OBF("Color")))
    {
        make_color3(L, drawing_object->color);
    }
    else if (!std::strcmp(type, OBF("__OBJECT_EXISTS")))
    {
        if (std::find(drawing_cache.begin(), drawing_cache.end(), drawing_object) != drawing_cache.end()) {
            lua_pushboolean(L, true);
        }
        else {
            lua_pushboolean(L, false);
        }
    }
    else if (!std::strcmp(type, OBF("Destroy")) || !std::strcmp(type, OBF("Remove")))
    {
        lua_pushcclosurek(L, clear_drawing_object, nullptr, 0, nullptr);
    }
    else
        return 0;

    return 1;
}

int base_t::__newindex(lua_State *L)
{
    const auto drawing_object = static_cast<base_t*>(lua_touserdata(L, 1));

    const auto type = luaL_checkstring(L, 2);

    if (!std::strcmp(type, OBF("ZIndex")))
    {
        drawing_object->z_index = luaL_checkinteger(L, 3);
    }
    else if (!std::strcmp(type, OBF("Visible")))
    {
        drawing_object->visible = luaL_checkboolean(L, 3);
    }
    else if (!std::strcmp(type, OBF("Transparency")))
    {
        drawing_object->transparency = luaL_checknumber(L, 3);
    }
    else if (!std::strcmp(type, OBF("Color")))
    {
        drawing_object->color = *reinterpret_cast<const color_t*>(lua_topointer(L, 3));
    }

    return 0;
}

void environment::load_drawing_lib(lua_State *L) {
    luaL_newmetatable(L, OBF("DrawingObject"));
    lua_pushcclosurek(L, __index, nullptr, 0, nullptr);
    lua_setfield(L, -2, OBF("__index"));
    lua_pushcclosurek(L, __newindex, nullptr, 0, nullptr);
    lua_setfield(L, -2, OBF("__newindex"));

    lua_newtable(L);

    lua_newtable(L);
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, OBF("UI"));

    lua_pushinteger(L, 1);
    lua_setfield(L, -2, OBF("System"));

    lua_pushinteger(L, 2);
    lua_setfield(L, -2, OBF("Plex"));

    lua_pushinteger(L, 3);
    lua_setfield(L, -2, OBF("Monospace"));

    lua_setfield(L, -2, OBF("Fonts"));

    lua_pushcclosurek(L, drawing_new, nullptr, 0, nullptr);
    lua_setfield(L, -2, OBF("new"));

    lua_setglobal(L, OBF("Drawing"));

    lua_pushcclosurek(L, isrenderobj, nullptr, 0, nullptr);
    lua_setglobal(L, OBF("isrenderobj"));
    lua_pushcclosurek(L, getrenderproperty, nullptr, 0, nullptr);
    lua_setglobal(L, OBF("getrenderproperty"));
    lua_pushcclosurek(L, setrenderproperty, nullptr, 0, nullptr);
    lua_setglobal(L, OBF("setrenderproperty"));
    lua_pushcclosurek(L, cleardrawcache, nullptr, 0, nullptr);
    lua_setglobal(L, OBF("cleardrawcache"));
}

void environment::reset_drawing_lib()
{
    key_map.clear();
    drawing_cache.clear();
}