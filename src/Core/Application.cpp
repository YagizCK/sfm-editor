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

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <random>
#include <glm/gtc/type_ptr.hpp>
#include <format>
#include <ImGuizmo.h>

#include "KeyCodes.hpp"


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
        m_pointShader = std::make_unique<Shader>("assets/shaders/basic.vert", "assets/shaders/basic.frag");
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
        if (m_VAO)
            glDeleteVertexArrays(1, &m_VAO);
        if (m_VBO)
            glDeleteBuffers(1, &m_VBO);
    }

    void Application::run() {
        ViewportInfo viewportInfo;

        while (m_running && !m_window->shouldClose()) {
            const float time = static_cast<float>(glfwGetTime());
            m_deltaTime = time - m_lastFrameTime;
            m_lastFrameTime = time;

            if ((m_viewportSize.x != m_lastViewportSize.x || m_viewportSize.y != m_lastViewportSize.y) &&
                m_viewportSize.x > 0.0f && m_viewportSize.y > 0.0f) {
                m_lastViewportSize = m_viewportSize;
                m_framebuffer->resize(static_cast<uint32_t>(m_viewportSize.x), static_cast<uint32_t>(m_viewportSize.y));
                m_camera->onResize(m_viewportSize.x, m_viewportSize.y);
            }

            glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

            if (m_editorSystem->pendingSelection) {
                m_editorSystem->pendingSelection = false;

                glBufferSubData(GL_ARRAY_BUFFER, 0, m_points.size() * sizeof(Point), m_points.data());
            }

            if (m_editorSystem->hasSelection()) {
                const auto& selected = m_editorSystem->selectedPointIndices;
                const unsigned int selectionCount = static_cast<unsigned int>(selected.size());

                const auto deltaPos = glm::vec3(m_editorSystem->gizmoTransform[3]) - m_editorSystem->gizmoStartPosition;
                if (glm::dot(deltaPos, deltaPos) > 0.0f) {
                    for (const unsigned int idx : selected) {
                        const glm::vec3 newPos = m_points[idx].position + deltaPos;
                        m_points[idx].position = newPos;
                        glBufferSubData(GL_ARRAY_BUFFER, idx * sizeof(Point), sizeof(glm::vec3), &newPos);
                    }
                    m_editorSystem->gizmoStartPosition = glm::vec3(m_editorSystem->gizmoTransform[3]);
                }

                if (m_editorSystem->pendingDeletion) {
                    m_editorSystem->pendingDeletion = false;

                    std::erase_if(m_points, [](const Point& p) {
                        return p.selected > 0.5f;
                    });

                    glBufferData(GL_ARRAY_BUFFER, m_points.size() * sizeof(Point), m_points.data(), GL_STATIC_DRAW);

                    m_editorSystem->selectedPointIndices.clear();

                    Logger::info(std::to_string(selectionCount) + " points deleted.");
                }
            }

            glBindBuffer(GL_ARRAY_BUFFER, 0);

            m_lineRenderer->onUpdate(m_deltaTime);

            viewportInfo.hovered = viewportInfo.hovered && !ImGuizmo::IsUsing();

            m_editorSystem->onUpdate(viewportInfo);

            m_camera->onUpdate(m_deltaTime, viewportInfo);

            // Render Pass
            m_framebuffer->bind();
            glClearColor(
                m_sceneProperties->backgroundColor.r,
                m_sceneProperties->backgroundColor.g,
                m_sceneProperties->backgroundColor.b,
                1.0f
            );
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderScene();
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

    void Application::renderScene() const {
        m_grid->draw(m_sceneProperties, m_camera);

        m_lineRenderer->draw(m_camera);

        if (m_points.empty()) return;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_pointShader->bind();
        m_pointShader->setFloat("u_PointSize", m_sceneProperties->pointSize);
        m_pointShader->setMat4("u_ViewProjection", m_camera->getViewProjection());
        glBindVertexArray(m_VAO);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_points.size()));
        m_pointShader->unbind();
        glDisable(GL_BLEND);
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

        if (m_VAO) {
            glDeleteVertexArrays(1, &m_VAO);
            m_VAO = 0;
        }
        if (m_VBO) {
            glDeleteBuffers(1, &m_VBO);
            m_VBO = 0;
        }

        glCreateVertexArrays(1, &m_VAO);
        glCreateBuffers(1, &m_VBO);

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        glBufferData(GL_ARRAY_BUFFER, m_points.size() * sizeof(Point), m_points.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), nullptr);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point),
                              reinterpret_cast<const void*>(offsetof(Point, color)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Point),
                              reinterpret_cast<const void*>(offsetof(Point, selected)));

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
