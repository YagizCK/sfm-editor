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

#pragma once

#include "UI/UIManager.h"
#include "Renderer/Framebuffer.h"
#include "Renderer/EditorCamera.h"
#include "Renderer/Shader.h"
#include "Renderer/SceneGrid.h"
#include "Renderer/LineRenderer.h"
#include "Types.hpp"
#include "Window.h"

#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace sfmeditor {
    extern GLFWwindow* g_nativeWindow;

    class Application {
    public:
        Application();
        ~Application();
        Application(const Application&) = default;
        Application& operator=(const Application&) = default;
        Application(Application&&) = default;
        Application& operator=(Application&&) = default;

        void run();

        float getDeltaTime() const { return m_deltaTime; }

    private:
        void renderScene() const;
        void onImportMap();
        void loadMap(const std::string& filepath);
        void onSaveMap() const;
        void onExit();

        float m_lastFrameTime = 0.0f;
        float m_deltaTime = 0.0f;

        std::unique_ptr<Window> m_window;
        std::unique_ptr<UIManager> m_uiManager;
        std::unique_ptr<SceneProperties> m_sceneProperties;
        std::unique_ptr<Shader> m_pointShader;
        std::unique_ptr<Framebuffer> m_framebuffer;
        std::unique_ptr<EditorCamera> m_camera;
        std::unique_ptr<SceneGrid> m_grid;
        std::unique_ptr<LineRenderer> m_lineRenderer;

        bool m_running = true;

        std::vector<Point> m_points;
        uint32_t m_VAO = 0, m_VBO = 0;

        glm::vec2 m_viewportSize = {0.0f, 0.0f};
        glm::vec2 m_lastViewportSize = {0.0f, 0.0f};
    };
}
