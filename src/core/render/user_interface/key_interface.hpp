//
// Created by user on 28/05/2025.
//

#pragma once
#include <memory>
//#include "TextEditor.h"

class key_interface {
public:
    void render();
};

inline const auto g_key_interface = std::make_unique<key_interface>();