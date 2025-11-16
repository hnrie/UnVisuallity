//
// Created by user on 28/05/2025.
//

#pragma once
#include <memory>
//#include "TextEditor.h"

/**
 * @brief Handles rendering of the key validation interface.
 */
class key_interface {
public:
    /**
     * @brief Draws the key interface widgets for the current frame.
     */
    void render();
};

/**
 * @brief Global accessor for the key interface renderer.
 */
inline const auto g_key_interface = std::make_unique<key_interface>();