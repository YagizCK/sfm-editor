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
#include <filesystem>


namespace sfmeditor {
    GLFWwindow* g_nativeWindow = nullptr;

    Application::Application() {
        Logger::init();

        Events::onKey.connect([this](const int key, const int action) {
            if (action != SFM_PRESS) return;

            if (Input::isKeyPressed(SFM_KEY_LEFT_CONTROL)) {
                if (key == SFM_KEY_O) {
                    onImportColmapModel();
                    return;
                }
                if (key == SFM_KEY_S) {
                    onSaveColmapModel(true);
                    return;
                }
                if (key == SFM_KEY_Z) {
                    m_editorSystem->getActionHistory()->undo();
                    return;
                }
                if (key == SFM_KEY_Y || (Input::isKeyPressed(SFM_KEY_LEFT_SHIFT) && key == SFM_KEY_Z)) {
                    m_editorSystem->getActionHistory()->redo();
                }
            }
        });

        m_window = std::make_unique<Window>(WindowProps("SFM Editor", 1600, 900));
        g_nativeWindow = m_window->getNativeWindow();

        m_sceneProperties = std::make_unique<SceneProperties>();
        m_renderer = std::make_unique<SceneRenderer>();
        m_framebuffer = std::make_unique<Framebuffer>(1600, 900);
        m_grid = std::make_unique<SceneGrid>();
        m_lineRenderer = std::make_unique<LineRenderer>();
        m_camera = std::make_unique<EditorCamera>();
        m_editorSystem = std::make_unique<EditorSystem>(m_camera.get(), &m_scene);
        m_uiManager = std::make_unique<UIManager>(m_window.get());
        m_uiManager->initPanels(m_sceneProperties.get(), m_camera.get(), &m_scene, m_editorSystem.get());

        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_DEPTH_TEST);

        Events::onFileDrop.connect([this](const std::string& path) {
            this->loadMap(path);
        });
    }

    Application::~Application() {
    }

    void Application::run() {
        while (m_running && !m_window->shouldClose()) {
            const float time = static_cast<float>(glfwGetTime());
            m_deltaTime = time - m_lastFrameTime;
            m_lastFrameTime = time;

            ViewportInfo& viewportInfo = m_uiManager->getViewportPanel()->getViewportInfo();

            if ((viewportInfo.size.x != m_lastViewportSize.x || viewportInfo.size.y != m_lastViewportSize.y) &&
                viewportInfo.size.x > 0.0f && viewportInfo.size.y > 0.0f) {
                m_lastViewportSize = viewportInfo.size;
                m_framebuffer->resize(static_cast<uint32_t>(viewportInfo.size.x),
                                      static_cast<uint32_t>(viewportInfo.size.y));
                m_camera->onResize(viewportInfo.size.x, viewportInfo.size.y);
            }

            // GPU Sync
            m_renderer->updateBuffers(m_scene.points, m_editorSystem.get());

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

                m_renderer->renderPickingPass(m_scene.points, m_sceneProperties.get(), m_camera.get());

                const bool isCtrl = Input::isKeyPressed(SFM_KEY_LEFT_CONTROL);

                const glm::vec2 mousePos = m_editorSystem->boxEnd;

                const int pickedID = SceneRenderer::readPointID(
                    static_cast<int>(mousePos.x),
                    static_cast<int>(mousePos.y),
                    static_cast<int>(viewportInfo.size.y)
                );

                m_editorSystem->getSelectionManager()->processPickedID(pickedID, isCtrl);
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
            m_lineRenderer->clear();

            const float camSize = m_sceneProperties->cameraSize;

            for (const auto& [image_id, cam] : m_scene.cameras) {
                if (m_editorSystem->isolatedCameraID != 0) {
                    continue;
                }

                bool isSelected = std::find(m_editorSystem->getSelectionManager()->selectedCameraIDs.begin(),
                                            m_editorSystem->getSelectionManager()->selectedCameraIDs.end(),
                                            image_id) != m_editorSystem->getSelectionManager()->selectedCameraIDs.end();

                glm::vec3 camColor = isSelected ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.5f, 0.0f);

                glm::mat4 rotationMatrix = glm::mat4_cast(cam.orientation);
                glm::mat4 model = glm::translate(glm::mat4(1.0f), cam.position) * rotationMatrix;

                auto center = glm::vec3(model * glm::vec4(0, 0, 0, 1));

                float aspectRatio = 1.0f;
                if (cam.height > 0 && cam.width > 0) {
                    aspectRatio = static_cast<float>(cam.width) / static_cast<float>(cam.height);
                }

                float w = camSize * aspectRatio;
                float h = camSize;
                float z = camSize * 2.0f;

                auto tl = glm::vec3(model * glm::vec4(-w, -h, z, 1.0f));
                auto tr = glm::vec3(model * glm::vec4(w, -h, z, 1.0f));
                auto bl = glm::vec3(model * glm::vec4(-w, h, z, 1.0f));
                auto br = glm::vec3(model * glm::vec4(w, h, z, 1.0f));

                m_lineRenderer->addLine(center, tl, camColor, 0.0f);
                m_lineRenderer->addLine(center, tr, camColor, 0.0f);
                m_lineRenderer->addLine(center, bl, camColor, 0.0f);
                m_lineRenderer->addLine(center, br, camColor, 0.0f);

                m_lineRenderer->addLine(tl, tr, camColor, 0.0f);
                m_lineRenderer->addLine(tr, br, camColor, 0.0f);
                m_lineRenderer->addLine(br, bl, camColor, 0.0f);
                m_lineRenderer->addLine(bl, tl, camColor, 0.0f);
            }

            m_lineRenderer->draw(m_camera);
            m_renderer->render(m_scene.points, m_sceneProperties.get(), m_camera.get());

            m_framebuffer->unbind();

            // UI Pass
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            m_uiManager->beginFrame();
            m_uiManager->renderMainMenuBar(
                [this]() { onImportMap(); },
                [this]() { onImportColmapModel(); },
                [this]() { onSaveMap(); },
                [this](const bool isBinary) { onSaveColmapModel(isBinary); },
                [this]() { onExit(); },
                [this]() { m_editorSystem->getActionHistory()->undo(); },
                [this]() { m_editorSystem->getActionHistory()->redo(); }
            );
            m_uiManager->getViewportPanel()->setTextureID(m_framebuffer->getTextureID());
            m_uiManager->renderPanels();
            m_uiManager->endFrame();

            m_window->onUpdate();
        }
    }

    void Application::onImportColmapModel() {
        std::string folderPath = FileDialog::pickFolder();
        if (!folderPath.empty()) {
            const std::filesystem::path binPath = std::filesystem::path(folderPath) / "points3D.bin";
            const std::filesystem::path txtPath = std::filesystem::path(folderPath) / "points3D.txt";

            if (std::filesystem::exists(binPath)) {
                loadMap(binPath.string());
            } else if (std::filesystem::exists(txtPath)) {
                loadMap(txtPath.string());
            } else {
                Logger::error("Invalid Model: No points3D.bin or points3D.txt found in " + folderPath);
            }
        }
    }

    void Application::onSaveColmapModel(const bool isBinary) {
        std::string folderPath = FileDialog::pickFolder();
        if (!folderPath.empty()) {
            Logger::info("Exporting model to: " + folderPath);

            const std::filesystem::path targetFile = std::filesystem::path(folderPath) / (isBinary
                    ? "points3D.bin"
                    : "points3D.txt");

            if (SceneExporter::exportFile(targetFile.string(), m_scene)) {
                Logger::info("Model exported successfully.");
                m_currentFilePath = targetFile.string();
            } else {
                Logger::error("Failed to export model!");
            }
        }
    }

    void Application::onImportMap() {
        const auto filter =
            "Point Cloud Files\0*.ply;*.obj;*.xyz\0"
            "Stanford PLY (*.ply)\0*.ply\0"
            "Wavefront OBJ (*.obj)\0*.obj\0"
            "XYZ Points (*.xyz)\0*.xyz\0";

        if (const std::string filepath = FileDialog::openFile(filter); !filepath.empty()) {
            loadMap(filepath);
        }
    }

    void Application::onSaveMap() {
        const auto filter =
            "Stanford PLY (*.ply)\0*.ply\0"
            "Wavefront OBJ (*.obj)\0*.obj\0"
            "XYZ Points (*.xyz)\0*.xyz\0";

        int filterIndex = 0;
        std::string filepath = FileDialog::saveFile(filter, &filterIndex);

        if (!filepath.empty()) {
            std::filesystem::path path(filepath);

            if (!path.has_extension()) {
                if (filterIndex == 1) {
                    filepath += ".ply";
                } else if (filterIndex == 2) {
                    filepath += ".obj";
                } else if (filterIndex == 3) {
                    filepath += ".xyz";
                }
            }

            Logger::info("Saving map as: " + filepath);

            if (SceneExporter::exportFile(filepath, m_scene)) {
                Logger::info("Map saved successfully.");
                m_currentFilePath = filepath;
            } else {
                Logger::error("Failed to save map!");
            }
        }
    }

    void Application::loadMap(const std::string& filepath) {
        Logger::info("Loading map from: " + filepath);

        const SfMScene newScene = ModelLoader::load(filepath);

        if (newScene.points.empty()) {
            Logger::warn("File loaded but contained no points or format error.");
            return;
        }

        m_scene = newScene;
        m_editorSystem->getSelectionManager()->resetState();
        m_renderer->initBuffers(m_scene.points);

        m_currentFilePath = filepath;

        Logger::info(std::format("Successfully loaded {} points and {} cameras.", m_scene.points.size(),
                                 m_scene.cameras.size()));
    }

    void Application::onExit() {
        if (m_running) {
            Logger::info("Shutting down application...");
            m_running = false;
        }
    }
}
