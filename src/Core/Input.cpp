/*
 * Copyright 2026 Yağız Cem Kocabıyık
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Input.h"

#include "Application.h"
#include "Events.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace sfmeditor {
    glm::vec2 Input::m_lastMousePos = {0.0f, 0.0f};

    void Input::init() {
        m_lastMousePos = getMousePosition();
    }

    bool Input::isKeyPressed(const int keycode) {
        const auto state = glfwGetKey(g_nativeWindow, keycode);
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool Input::isMouseButtonPressed(const int button) {
        const auto state = glfwGetMouseButton(g_nativeWindow, button);
        return state == GLFW_PRESS;
    }

    glm::vec2 Input::getMousePosition() {
        double x, y;
        glfwGetCursorPos(g_nativeWindow, &x, &y);
        return {static_cast<float>(x), static_cast<float>(y)};
    }

    float Input::getMouseX() {
        return getMousePosition().x;
    }

    float Input::getMouseY() {
        return getMousePosition().y;
    }

    void Input::keyCallback(GLFWwindow* window, const int key, const int scancode, const int action, const int mods) {
        const bool pressed = (action == GLFW_PRESS);
        const bool released = (action == GLFW_RELEASE);

        Events::onKey.emit(key, pressed);
    }

    void Input::mouseButtonCallback(GLFWwindow* window, const int button, const int action, const int mods) {
        const bool pressed = (action == GLFW_PRESS);

        Events::onMouseButton.emit(button, pressed);
    }

    void Input::cursorPosCallback(GLFWwindow* window, const double xPos, const double yPos) {
        const glm::vec2 pos(static_cast<float>(xPos), static_cast<float>(yPos));
        const glm::vec2 delta = pos - m_lastMousePos;
        m_lastMousePos = pos;

        Events::onMouseMove.emit(delta, pos);
    }

    void Input::scrollCallback(GLFWwindow* window, const double xOffset, const double yOffset) {
        Events::onMouseScroll.emit(static_cast<float>(yOffset));
    }
}
