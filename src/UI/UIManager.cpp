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
            ImGui::DockBuilderDockWindow("Console Log", dockBottomID);

            ImGui::DockBuilderFinish(dockspaceID);
        }

        ImGui::End();
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

    void UIManager::renderViewport(const uint32_t textureID, glm::vec2& outSize, bool& outHovered,
                                   bool& outFocused, const EditorCamera* camera) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        ImGui::Begin("Viewport");

        outHovered = ImGui::IsWindowHovered();
        if (outHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::SetWindowFocus();
        }
        outFocused = ImGui::IsWindowFocused();

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        outSize = {viewportPanelSize.x, viewportPanelSize.y};

        ImGui::Image(textureID, viewportPanelSize, ImVec2(0, 1), ImVec2(1, 0));

        if (camera && camera->gizmoOperation != -1) {
            ImGuizmo::SetOrthographic(camera->projectionMode == ProjectionMode::Orthographic);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(),
                              ImGui::GetWindowHeight());

            glm::mat4 viewMatrix = camera->getViewMatrix();
            glm::mat4 projectionMatrix = camera->getProjection();

            static glm::mat4 testTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f));

            ImGuizmo::Manipulate(
                glm::value_ptr(viewMatrix),
                glm::value_ptr(projectionMatrix),
                static_cast<ImGuizmo::OPERATION>(camera->gizmoOperation),
                ImGuizmo::MODE::WORLD,
                glm::value_ptr(testTransform)
            );
        }

        constexpr float distance = 15.0f;
        const ImVec2 viewportPos = ImGui::GetWindowPos();
        const ImVec2 viewportSize = ImGui::GetWindowSize();

        const auto overlayPos = ImVec2(viewportPos.x + viewportSize.x - distance, viewportPos.y + distance + 25.0f);
        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::SetNextWindowPos(overlayPos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoMove;

        if (ImGui::Begin("##OverlayControls", nullptr, flags)) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "CONTROLS");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 2.0f));

            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Global:");
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

                    ImGui::Dummy(ImVec2(0.0f, 2.0f));
                    ImGui::Text("Zoom (Move): Scroll (No Click)");
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
            ImGui::Text("None: Q");
            ImGui::Text("Move: W");
            ImGui::Text("Rotate: E");
            ImGui::Text("Scale: R");
            ImGui::Unindent();
        }
        ImGui::End();
        ImGui::PopStyleVar(2);

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void UIManager::renderInfoPanel(const std::unique_ptr<SceneProperties>& sceneProperties,
                                    const std::unique_ptr<EditorCamera>& camera, const int pointCount) {
        ImGui::Begin("Properties");

        if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Total Points: %d", pointCount);
            const ImGuiIO& io = ImGui::GetIO();
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::Text("Frame Time: %.3f ms", io.DeltaTime * 1000.0f);
        }

        if (ImGui::CollapsingHeader("Scene Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ColorEdit3("Background", &sceneProperties->backgroundColor.x);
            ImGui::Checkbox("Show Grid", &sceneProperties->showGrid);
            ImGui::Checkbox("Show Axes", &sceneProperties->showAxes);
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
        ImGui::Begin("Console Log");

        if (ImGui::Button("Clear")) {
            Logger::clear();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Logs: %d", static_cast<int>(Logger::getLogs().size()));

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
}
