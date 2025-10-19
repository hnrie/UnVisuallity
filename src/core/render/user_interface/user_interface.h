//
// Created by savage on 18.04.2025.
//

#pragma once
#include <memory>


class user_interface {
public:
    //TextEditor text_editor;

    void render();
};

inline const auto g_user_interface = std::make_unique<user_interface>();