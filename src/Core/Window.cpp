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

#include "Window.h"

#include "Input.h"
#include "Events.hpp"
#include "Application.h"
#include "Logger.h"

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


namespace sfmeditor {
    Window::Window(const WindowProps& props) {
        init(props);
    }

    Window::~Window() {
        shutdown();
    }

    void* Window::gladLoaderAdapter(const char* name) {
        return reinterpret_cast<void*>(glfwGetProcAddress(name));
    }

    void Window::init(const WindowProps& props) {
        m_data.title = props.title;
        m_data.width = props.width;
        m_data.height = props.height;

        if (!g_glfwInitialized) {
            if (!glfwInit()) {
                Logger::critical("Failed to initialize GLFW!");
                return;
            }
            g_glfwInitialized = true;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_window = glfwCreateWindow(static_cast<int>(props.width), static_cast<int>(props.height), m_data.title.c_str(),
                                    nullptr, nullptr);

        if (!m_window) {
            Logger::critical("Failed to create GLFW window!");
            return;
        }

        glfwMakeContextCurrent(m_window);

        g_nativeWindow = m_window;
        Input::init();

        glfwSwapInterval(0);

        if (!gladLoadGLLoader(&gladLoaderAdapter)) {
            Logger::critical("Failed to load glad!");
        }

        glfwSetWindowUserPointer(m_window, &m_data);

        glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, const int width, const int height) {
            WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
            data.width = width;
            data.height = height;
            glViewport(0, 0, width, height);

            Events::onWindowResize.emit(width, height);
        });

        glfwSetDropCallback(m_window, [](GLFWwindow* window, int count, const char** paths) {
            if (count > 0) {
                Events::onFileDrop.emit(paths[0]);
            }
        });

        glfwSetKeyCallback(m_window, Input::keyCallback);

        glfwSetMouseButtonCallback(m_window, Input::mouseButtonCallback);

        glfwSetCursorPosCallback(m_window, Input::cursorPosCallback);

        glfwSetScrollCallback(m_window, Input::scrollCallback);
    }

    void Window::shutdown() const {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    void Window::onUpdate() const {
        glfwPollEvents();
        glfwSwapBuffers(m_window);
    }

    bool Window::shouldClose() const {
        return glfwWindowShouldClose(m_window);
    }
}
