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

#include "UIManager.h"

#include "Core/Logger.h"
#include "Core/Window.h"
#include "Renderer/EditorCamera.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <iostream>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>


namespace sfmeditor {
    UIManager::UIManager(Window* window) : m_windowRef(window) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        ImGui_ImplGlfw_InitForOpenGL(window->getNativeWindow(), true);
        ImGui_ImplOpenGL3_Init("#version 460");
    }

    UIManager::~UIManager() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void UIManager::beginFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        renderDockspace();
    }

    void UIManager::endFrame() const {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(static_cast<float>(m_windowRef->getWidth()),
                                static_cast<float>(m_windowRef->getHeight()));

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void UIManager::renderMainMenuBar(const std::function<void()>& onImport, const std::function<void()>& onSave,
                                      const std::function<void()>& onExit) {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Import Map...", "Ctrl+O")) { if (onImport) onImport(); }
                if (ImGui::MenuItem("Save", "Ctrl+S")) { if (onSave) onSave(); }
                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) { if (onSave) onSave(); }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) { if (onExit) onExit(); }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit")) {
                ImGui::MenuItem("Undo", "Ctrl+Z", false, false);
                ImGui::MenuItem("Redo", "Ctrl+Y", false, false);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Reset Layout")) {
                    m_resetLayout = true;
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

    void UIManager::renderViewport(const uint32_t textureID, ViewportInfo& viewportInfo, const EditorCamera* camera,
                                   EditorSystem* editorSystem) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        ImGui::Begin("Viewport");

        viewportInfo.hovered = ImGui::IsWindowHovered();
        if (viewportInfo.hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::SetWindowFocus();
        }
        viewportInfo.focused = ImGui::IsWindowFocused();

        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        viewportInfo.size = {viewportSize.x, viewportSize.y};

        ImVec2 viewportPos = ImGui::GetCursorScreenPos();
        viewportInfo.position = {viewportPos.x, viewportPos.y};

        ImGui::Image(textureID, viewportSize, ImVec2(0, 1), ImVec2(1, 0));

        if (editorSystem->boxSelecting) {
            const glm::vec2 d = glm::abs(editorSystem->boxEnd - editorSystem->boxStart);

            if (const float distSq = glm::dot(d, d); !(distSq < 9.0f)) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();

                const ImVec2 p1 = {viewportPos.x + editorSystem->boxStart.x, viewportPos.y + editorSystem->boxStart.y};
                const ImVec2 p2 = {viewportPos.x + editorSystem->boxEnd.x, viewportPos.y + editorSystem->boxEnd.y};

                drawList->AddRectFilled(p1, p2, IM_COL32(0, 150, 255, 50));
                drawList->AddRect(p1, p2, IM_COL32(0, 150, 255, 255), 0.0f, 0, 1.0f);
            }
        }

        if (editorSystem->hasSelection() && editorSystem->gizmoOperation != -1) {
            ImGuizmo::SetOrthographic(camera->projectionMode == ProjectionMode::Orthographic);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(
                viewportPos.x, viewportPos.y,
                viewportSize.x, viewportSize.y
            );

            glm::mat4 viewMatrix = camera->getViewMatrix();
            glm::mat4 projectionMatrix = camera->getProjection();
            ImGuizmo::Manipulate(
                glm::value_ptr(viewMatrix),
                glm::value_ptr(projectionMatrix),
                static_cast<ImGuizmo::OPERATION>(editorSystem->gizmoOperation),
                ImGuizmo::MODE::WORLD,
                glm::value_ptr(editorSystem->gizmoTransform)
            );
        }

        constexpr float distance = 15.0f;

        const auto overlayPos = ImVec2(viewportPos.x + viewportSize.x - distance, viewportPos.y + distance);
        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::SetNextWindowPos(overlayPos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoMouseInputs;

        if (ImGui::Begin("##OverlayControls", nullptr, flags)) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "CONTROLS");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 2.0f));

            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Selection:");
            ImGui::BulletText("Single: Left Click");
            ImGui::BulletText("Box Select: Click & Drag");
            ImGui::BulletText("Multi/Toggle: Hold Ctrl");
            ImGui::BulletText("Delete: Del");

            ImGui::Dummy(ImVec2(0.0f, 5.0f));

            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Camera:");
            ImGui::BulletText("Rotate: Hold Right Click");
            ImGui::BulletText("Pan: Hold Middle Click");
            ImGui::BulletText("Reset View: F");

            ImGui::Dummy(ImVec2(0.0f, 5.0f));

            if (camera) {
                if (camera->cameraStyle == CameraStyle::Free) {
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[ FREE FLY MODE ]");
                    ImGui::Separator();
                    ImGui::Text("While Holding Right Click:");
                    ImGui::Indent();
                    ImGui::Text("Move: W, A, S, D");
                    ImGui::Text("Up/Down: E, Q");
                    ImGui::Text("Boost: Hold Shift");
                    ImGui::Text("Slow: Hold Alt");
                    ImGui::Text("Adjust Speed: Scroll");
                    ImGui::Unindent();
                } else if (camera->cameraStyle == CameraStyle::Orbit) {
                    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "[ ORBIT MODE ]");
                    ImGui::Separator();
                    ImGui::BulletText("Zoom In/Out: Scroll");
                    ImGui::Dummy(ImVec2(0.0f, 2.0f));
                    ImGui::TextDisabled("Distance: %.2f", camera->distance);
                }
            }
            ImGui::Separator();
            ImGui::Text("Gizmo Mode:");
            ImGui::Indent();

            const int op = editorSystem->gizmoOperation;
            ImGui::TextColored(op == -1 ? ImVec4(1, 1, 0, 1) : ImVec4(0.6f, 0.6f, 0.6f, 1), "None: Q");
            ImGui::TextColored(op == ImGuizmo::TRANSLATE ? ImVec4(1, 1, 0, 1) : ImVec4(0.6f, 0.6f, 0.6f, 1), "Move: W");
            ImGui::TextColored(op == ImGuizmo::ROTATE ? ImVec4(1, 1, 0, 1) : ImVec4(0.6f, 0.6f, 0.6f, 1), "Rotate: E");
            ImGui::TextColored(op == ImGuizmo::SCALE ? ImVec4(1, 1, 0, 1) : ImVec4(0.6f, 0.6f, 0.6f, 1), "Scale: R");
            ImGui::Unindent();
        }
        ImGui::End();
        ImGui::PopStyleVar(2);

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void UIManager::renderInfoPanel(const std::unique_ptr<SceneProperties>& sceneProperties,
                                    const std::unique_ptr<EditorCamera>& camera,
                                    const SfMScene& scene,
                                    EditorSystem* editorSystem) {
        ImGui::Begin("Properties");
        if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Total Points: %zu", scene.points.size());
            ImGui::Text("Total Cameras: %zu", scene.cameras.size());
            const ImGuiIO& io = ImGui::GetIO();
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::Text("Frame Time: %.3f ms", io.DeltaTime * 1000.0f);
        }

        if (ImGui::CollapsingHeader("Selected Point Info", ImGuiTreeNodeFlags_DefaultOpen)) {
            const auto& selectedIndices = editorSystem->selectedPointIndices;

            if (selectedIndices.empty()) {
                ImGui::TextDisabled("No point selected.");
            } else if (selectedIndices.size() > 1) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Multiple points selected (%zu)",
                                   selectedIndices.size());
            } else {
                unsigned int pointIdx = selectedIndices[0];

                if (pointIdx < scene.metadata.size()) {
                    const auto& meta = scene.metadata[pointIdx];

                    ImGui::Text("ID: %llu", meta.original_id);
                    ImGui::Text("Reproj. Error: %.4f px", meta.error);
                    ImGui::Separator();
                    ImGui::Text("Observed in %zu Images:", meta.observations.size());

                    ImGui::BeginChild("TrackList", ImVec2(0, 300), true);

                    for (const auto& obs : meta.observations) {
                        std::string imgName = "Unknown";
                        if (scene.cameras.contains(obs.image_id)) {
                            imgName = scene.cameras.at(obs.image_id).imageName;
                        }

                        if (ImGui::TreeNode((void*)static_cast<intptr_t>(obs.image_id), "%s (Feature: %d)",
                                            imgName.c_str(), obs.point2D_idx)) {
                            std::string fullPath = scene.imageBasePath + "\\" + imgName;
                            UITexture tex = getOrLoadImage(fullPath);

                            if (tex.id != 0) {
                                const float availWidth = ImGui::GetContentRegionAvail().x;

                                float scale = availWidth / static_cast<float>(tex.width);
                                float drawHeight = static_cast<float>(tex.height) * scale;
                                float drawWidth = availWidth;

                                if (constexpr float maxPanelHeight = 200.0f; drawHeight > maxPanelHeight) {
                                    scale = maxPanelHeight / static_cast<float>(tex.height);
                                    drawHeight = maxPanelHeight;
                                    drawWidth = static_cast<float>(tex.width) * scale;
                                }

                                const float cursorPosX = ImGui::GetCursorPosX() + (availWidth - drawWidth) * 0.5f;
                                ImGui::SetCursorPosX(cursorPosX);

                                ImGui::Image(tex.id, ImVec2(drawWidth, drawHeight));

                                if (ImGui::IsItemHovered()) {
                                    ImGui::BeginTooltip();

                                    const ImVec2 displaySize = ImGui::GetMainViewport()->WorkSize;

                                    const float maxTooltipWidth = displaySize.x * 0.8f;
                                    const float maxTooltipHeight = displaySize.y * 0.8f;

                                    float ttScale = 1.0f;

                                    if (static_cast<float>(tex.width) > maxTooltipWidth) {
                                        ttScale = maxTooltipWidth / static_cast<float>(tex.width);
                                    }

                                    if ((static_cast<float>(tex.height) * ttScale) > maxTooltipHeight) {
                                        ttScale = maxTooltipHeight / static_cast<float>(tex.height);
                                    }

                                    const float ttWidth = static_cast<float>(tex.width) * ttScale;
                                    const float ttHeight = static_cast<float>(tex.height) * ttScale;

                                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", imgName.c_str());

                                    const ImVec2 imageStartPos = ImGui::GetCursorScreenPos();

                                    ImGui::Image(tex.id, ImVec2(ttWidth, ttHeight));

                                    if (scene.cameras.contains(obs.image_id)) {
                                        const auto& camPose = scene.cameras.at(obs.image_id);
                                        if (obs.point2D_idx < camPose.features.size()) {
                                            const glm::vec2 rawCoord = camPose.features[obs.point2D_idx].coordinates;

                                            const float normX = rawCoord.x / static_cast<float>(tex.width);
                                            const float normY = rawCoord.y / static_cast<float>(tex.height);

                                            ImVec2 centerPos;
                                            centerPos.x = imageStartPos.x + normX * ttWidth;
                                            centerPos.y = imageStartPos.y + normY * ttHeight;

                                            ImDrawList* drawList = ImGui::GetWindowDrawList();

                                            constexpr ImU32 crossColor = IM_COL32(255, 50, 50, 255);

                                            drawList->AddCircle(centerPos, 5.0f, crossColor, 0, 4.0f);
                                        }
                                    }

                                    ImGui::TextDisabled("Original Size: %dx%d | Shown at: %.0f%%", tex.width,
                                                        tex.height, ttScale * 100.0f);

                                    ImGui::EndTooltip();
                                } else {
                                    ImGui::TextDisabled("Resolution: %dx%d (Hover to Enlarge)", tex.width, tex.height);
                                }
                            } else {
                                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Image not found on disk!");
                                ImGui::TextDisabled("Expected Path:\n%s", fullPath.c_str());
                            }

                            ImGui::TreePop();
                        }
                    }
                    ImGui::EndChild();
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No metadata available.");
                }
            }
        }

        if (ImGui::CollapsingHeader("Scene Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ColorEdit3("Background", &sceneProperties->backgroundColor.x);
            ImGui::Checkbox("Show Grid", &sceneProperties->showGrid);
            ImGui::Checkbox("Show Axes", &sceneProperties->showAxes);
            ImGui::DragFloat("Point Size", &sceneProperties->pointSize, 0.1f, 0.1f, 100.0f);
        }

        if (ImGui::CollapsingHeader("Camera Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Position", &camera->position.x, 0.1f);

            ImGui::Separator();
            ImGui::Text("Rotation (Euler)");

            bool eulerChanged = false;
            if (ImGui::DragFloat("Pitch", &camera->pitch, 0.5f)) eulerChanged = true;
            if (ImGui::DragFloat("Yaw", &camera->yaw, 0.5f)) eulerChanged = true;
            if (ImGui::DragFloat("Roll", &camera->roll, 0.5f)) eulerChanged = true;

            if (eulerChanged) camera->setRotationFromUI();

            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            ImGui::Text("Rotation (Quaternion)");

            glm::quat currentQuat = camera->orientation;
            if (ImGui::DragFloat4("X Y Z W", &currentQuat.x, 0.01f, -1.0f, 1.0f)) {
                camera->setOrientationFromUI(currentQuat);
            }

            ImGui::Separator();

            if (ImGui::Button("Reset View to Origin")) {
                camera->resetView();
            }
        }

        if (ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* cameraStyleStrings[] = {"Free Look (Fly)", "Orbit (Turntable)"};
            int currentStyle = static_cast<int>(camera->cameraStyle);
            if (ImGui::Combo("Camera Mode", &currentStyle, cameraStyleStrings, 2)) {
                camera->setCameraStyle(static_cast<CameraStyle>(currentStyle));
            }

            ImGui::Separator();

            bool projectionChanged = false;

            const char* projectionTypeStrings[] = {"Perspective", "Orthographic"};
            int currentProj = static_cast<int>(camera->projectionMode);
            if (ImGui::Combo("Projection", &currentProj, projectionTypeStrings, 2)) {
                camera->projectionMode = static_cast<ProjectionMode>(currentProj);
                projectionChanged = true;
            }

            if (camera->projectionMode == ProjectionMode::Perspective) {
                if (ImGui::SliderFloat("FOV", &camera->FOV, 1.0f, 179.0f)) projectionChanged = true;
            } else {
                if (ImGui::DragFloat("Ortho Size", &camera->orthoSize, 0.1f, 0.1f, 1000.0f))
                    projectionChanged =
                        true;
            }

            if (projectionChanged) camera->updateProjection();

            ImGui::Separator();

            ImGui::DragFloat("Speed", &camera->movementSpeed, 0.1f, camera->minMovementSpeed, 500.0f);
            ImGui::SliderFloat("Sensitivity", &camera->mouseSensitivity, 0.001f, 0.1f);
            ImGui::SliderFloat("Scroll Sens.", &camera->scrollSensitivity, 0.1f, 50.0f);
        }

        ImGui::End();
    }

    void UIManager::renderConsole() {
        ImGui::Begin("Logs");

        if (ImGui::Button("Clear")) {
            Logger::clear();
        }
        ImGui::SameLine();

        ImGui::Separator();

        ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        for (const auto& [level, message, timestamp] : Logger::getLogs()) {
            ImGui::TextDisabled("[%s]", timestamp.c_str());
            ImGui::SameLine();

            ImVec4 color;
            switch (level) {
            case LogLevel::Info: color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                break;
            case LogLevel::Warning: color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                break;
            case LogLevel::Error: color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                break;
            case LogLevel::Critical: color = ImVec4(1.0f, 0.0f, 1.0f, 1.0f);
                break;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(message.c_str());
            ImGui::PopStyleColor();
        }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }

    void UIManager::renderDockspace() {
        static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("SFM DockSpace", nullptr, windowFlags);
        ImGui::PopStyleVar(3);

        const ImGuiIO& io = ImGui::GetIO();
        const ImGuiID dockspaceID = ImGui::GetID("SFMDockSpace");

        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), dockspaceFlags);
        }

        if (m_resetLayout) {
            m_resetLayout = false;

            ImGui::DockBuilderRemoveNode(dockspaceID);
            ImGui::DockBuilderAddNode(dockspaceID, dockspaceFlags | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceID, viewport->Size);

            ImGuiID dockMainID = dockspaceID;

            const ImGuiID dockRightID = ImGui::DockBuilderSplitNode(dockMainID, ImGuiDir_Right, 0.25f, nullptr,
                                                                    &dockMainID);

            const ImGuiID dockBottomID = ImGui::DockBuilderSplitNode(dockMainID, ImGuiDir_Down, 0.25f, nullptr,
                                                                     &dockMainID);

            ImGui::DockBuilderDockWindow("Viewport", dockMainID);
            ImGui::DockBuilderDockWindow("Properties", dockRightID);
            ImGui::DockBuilderDockWindow("Logs", dockBottomID);

            ImGui::DockBuilderFinish(dockspaceID);
        }

        ImGui::End();
    }

    UITexture UIManager::getOrLoadImage(const std::string& filepath) {
        if (m_imageCache.contains(filepath)) {
            return m_imageCache[filepath];
        }

        UITexture tex;
        int channels;
        unsigned char* data = stbi_load(filepath.c_str(), &tex.width, &tex.height, &channels, 4);

        if (data) {
            glGenTextures(1, &tex.id);
            glBindTexture(GL_TEXTURE_2D, tex.id);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            stbi_image_free(data);
            Logger::info("Loaded image to UI: " + filepath);
        } else {
            Logger::error("Failed to load image: " + filepath);
        }

        m_imageCache[filepath] = tex;
        return tex;
    }
}
