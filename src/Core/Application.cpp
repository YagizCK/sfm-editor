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

#include "Application.h"

#include "IO/FileDialog.h"
#include "IO/ModelLoader.h"
#include "IO/SceneExporter.h"
#include "Logger.h"
#include "Input.h"
#include "Events.hpp"
#include "KeyCodes.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <random>
#include <glm/gtc/type_ptr.hpp>
#include <format>
#include <imgui.h>
#include <ImGuizmo.h>


namespace sfmeditor {
    GLFWwindow* g_nativeWindow = nullptr;

    Application::Application() {
        Logger::init();

        Events::onKey.connect([this](const int key, const int action) {
            if (m_running && action == SFM_PRESS && key == SFM_KEY_ESCAPE) {
                m_running = false;
            }
        });

        m_window = std::make_unique<Window>(WindowProps("SFM Editor", 1600, 900));
        g_nativeWindow = m_window->getNativeWindow();

        m_uiManager = std::make_unique<UIManager>(m_window.get());
        m_sceneProperties = std::make_unique<SceneProperties>();
        m_renderer = std::make_unique<SceneRenderer>();
        m_framebuffer = std::make_unique<Framebuffer>(1600, 900);
        m_grid = std::make_unique<SceneGrid>();
        m_lineRenderer = std::make_unique<LineRenderer>();
        m_camera = std::make_unique<EditorCamera>();
        m_editorSystem = std::make_unique<EditorSystem>(m_camera.get(), m_lineRenderer.get(), &m_points);

        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_DEPTH_TEST);

        Events::onFileDrop.connect([this](const std::string& path) {
            this->loadMap(path);
        });
    }

    Application::~Application() {
    }

    void Application::run() {
        ViewportInfo viewportInfo;

        while (m_running && !m_window->shouldClose()) {
            const float time = static_cast<float>(glfwGetTime());
            m_deltaTime = time - m_lastFrameTime;
            m_lastFrameTime = time;

            if ((viewportInfo.size.x != m_lastViewportSize.x || viewportInfo.size.y != m_lastViewportSize.y) &&
                viewportInfo.size.x > 0.0f && viewportInfo.size.y > 0.0f) {
                m_lastViewportSize = viewportInfo.size;
                m_framebuffer->resize(static_cast<uint32_t>(viewportInfo.size.x),
                                      static_cast<uint32_t>(viewportInfo.size.y));
                m_camera->onResize(viewportInfo.size.x, viewportInfo.size.y);
            }

            // GPU Sync
            m_renderer->updateBuffers(m_points, m_editorSystem.get());

            // System Updates
            m_lineRenderer->onUpdate(m_deltaTime);
            viewportInfo.hovered = viewportInfo.hovered && !ImGuizmo::IsUsing();
            m_editorSystem->onUpdate(viewportInfo);
            m_camera->onUpdate(m_deltaTime, viewportInfo);

            // Render Pass
            m_framebuffer->bind();

            if (m_editorSystem->pendingPickedID) {
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                m_renderer->renderPickingPass(m_points, m_sceneProperties.get(), m_camera.get());


                const bool isCtrl = Input::isKeyPressed(SFM_KEY_LEFT_CONTROL);

                const glm::vec2 mousePos = m_editorSystem->boxEnd;

                const int pickedID = SceneRenderer::readPointID(
                    static_cast<int>(mousePos.x),
                    static_cast<int>(mousePos.y),
                    static_cast<int>(viewportInfo.size.y)
                );

                m_editorSystem->processPickedID(pickedID, isCtrl);
                m_editorSystem->pendingPickedID = false;
            }

            glClearColor(
                m_sceneProperties->backgroundColor.r,
                m_sceneProperties->backgroundColor.g,
                m_sceneProperties->backgroundColor.b,
                1.0f
            );
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            m_grid->draw(m_sceneProperties, m_camera);
            m_lineRenderer->draw(m_camera);
            m_renderer->render(m_points, m_sceneProperties.get(), m_camera.get());

            m_framebuffer->unbind();

            // UI Pass
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            m_uiManager->beginFrame();
            m_uiManager->renderMainMenuBar(
                [this]() { onImportMap(); },
                [this]() { onSaveMap(); },
                [this]() { onExit(); }
            );
            m_uiManager->renderViewport(m_framebuffer->getTextureID(), viewportInfo, m_camera.get(),
                                        m_editorSystem.get());
            m_uiManager->renderInfoPanel(m_sceneProperties, m_camera, static_cast<int>(m_points.size()));
            m_uiManager->renderConsole();
            m_uiManager->endFrame();

            m_window->onUpdate();
        }
    }

    void Application::onImportMap() {
        const auto filter =
            "All Supported\0*.bin;*.txt;*.ply;*.obj;*.xyz\0"
            "COLMAP Binary (*.bin)\0*.bin\0"
            "COLMAP Text (*.txt)\0*.txt\0"
            "Stanford PLY (*.ply)\0*.ply\0"
            "Wavefront OBJ (*.obj)\0*.obj\0"
            "XYZ Points (*.xyz)\0*.xyz\0";

        if (const std::string filepath = FileDialog::openFile(filter); !filepath.empty()) {
            loadMap(filepath);
        }
    }

    void Application::loadMap(const std::string& filepath) {
        Logger::info("Loading map from: " + filepath);

        const auto newPoints = ModelLoader::load(filepath);

        if (newPoints.empty()) {
            Logger::warn("File loaded but contained no points or format error.");
            return;
        }

        m_points = newPoints;
        m_editorSystem->resetState();
        m_renderer->initBuffers(m_points);
        Logger::info("Successfully loaded " + std::to_string(m_points.size()) + " points.");
    }

    void Application::onSaveMap() const {
        const auto filter =
            "COLMAP Binary (*.bin)\0*.bin\0"
            "Stanford PLY (*.ply)\0*.ply\0"
            "Wavefront OBJ (*.obj)\0*.obj\0"
            "XYZ Points (*.xyz)\0*.xyz\0";

        if (const std::string filepath = FileDialog::saveFile(filter); !filepath.empty()) {
            if (SceneExporter::exportFile(filepath, m_points)) Logger::info("Map saved successfully.");
            else Logger::error("Failed to save map!");
        }
    }

    void Application::onExit() {
        if (m_running) {
            Logger::info("Shutting down application...");
            m_running = false;
        }
    }
}
