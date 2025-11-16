//
// Created by savage on 18.04.2025.
//

#pragma once
#include <memory>


/**
 * @brief Renders the in-game UI exposed to the exploit user.
 */
class user_interface {
public:
    //TextEditor text_editor;

    /**
     * @brief Draws all interface components for the current frame.
     */
    void render();
};

/**
 * @brief Global accessor for the user interface renderer.
 */
inline const auto g_user_interface = std::make_unique<user_interface>();