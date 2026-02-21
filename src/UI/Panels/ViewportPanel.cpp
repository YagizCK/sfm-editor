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

#include "ViewportPanel.h"

#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>


namespace sfmeditor {
    ViewportPanel::ViewportPanel(EditorCamera* camera, EditorSystem* editorSystem)
        : m_camera(camera), m_editorSystem(editorSystem) {
    }

    void ViewportPanel::onRender() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport");

        m_viewportInfo.hovered = ImGui::IsWindowHovered();
        if (m_viewportInfo.hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::SetWindowFocus();
        }
        m_viewportInfo.focused = ImGui::IsWindowFocused();

        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        m_viewportInfo.size = {viewportSize.x, viewportSize.y};

        ImVec2 viewportPos = ImGui::GetCursorScreenPos();
        m_viewportInfo.position = {viewportPos.x, viewportPos.y};

        ImGui::Image(m_textureID, viewportSize, ImVec2(0, 1), ImVec2(1, 0));

        if (m_editorSystem->boxSelecting) {
            const glm::vec2 d = glm::abs(m_editorSystem->boxEnd - m_editorSystem->boxStart);

            if (const float distSq = glm::dot(d, d); !(distSq < 9.0f)) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();

                const ImVec2 p1 = {
                    viewportPos.x + m_editorSystem->boxStart.x, viewportPos.y + m_editorSystem->boxStart.y
                };
                const ImVec2 p2 = {viewportPos.x + m_editorSystem->boxEnd.x, viewportPos.y + m_editorSystem->boxEnd.y};

                drawList->AddRectFilled(p1, p2, IM_COL32(0, 150, 255, 50));
                drawList->AddRect(p1, p2, IM_COL32(0, 150, 255, 255), 0.0f, 0, 1.0f);
            }
        }

        if (m_editorSystem->hasSelection() && m_editorSystem->gizmoOperation != -1) {
            ImGuizmo::SetOrthographic(m_camera->projectionMode == ProjectionMode::Orthographic);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(
                viewportPos.x, viewportPos.y,
                viewportSize.x, viewportSize.y
            );

            glm::mat4 viewMatrix = m_camera->getViewMatrix();
            glm::mat4 projectionMatrix = m_camera->getProjection();
            ImGuizmo::Manipulate(
                glm::value_ptr(viewMatrix),
                glm::value_ptr(projectionMatrix),
                static_cast<ImGuizmo::OPERATION>(m_editorSystem->gizmoOperation),
                ImGuizmo::MODE::WORLD,
                glm::value_ptr(m_editorSystem->gizmoTransform)
            );
        }

        renderOverlayControls();

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void ViewportPanel::renderOverlayControls() {
        constexpr float distance = 15.0f;

        const auto overlayPos = ImVec2(m_viewportInfo.position.x + m_viewportInfo.size.x - distance,
                                       m_viewportInfo.position.y + distance);
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

            if (m_camera) {
                if (m_camera->cameraStyle == CameraStyle::Free) {
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
                } else if (m_camera->cameraStyle == CameraStyle::Orbit) {
                    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "[ ORBIT MODE ]");
                    ImGui::Separator();
                    ImGui::BulletText("Zoom In/Out: Scroll");
                    ImGui::Dummy(ImVec2(0.0f, 2.0f));
                    ImGui::TextDisabled("Distance: %.2f", m_camera->distance);
                }
            }
            ImGui::Separator();
            ImGui::Text("Gizmo Mode:");
            ImGui::Indent();

            const int op = m_editorSystem->gizmoOperation;
            ImGui::TextColored(op == -1 ? ImVec4(1, 1, 0, 1) : ImVec4(0.6f, 0.6f, 0.6f, 1), "None: Q");
            ImGui::TextColored(op == ImGuizmo::TRANSLATE ? ImVec4(1, 1, 0, 1) : ImVec4(0.6f, 0.6f, 0.6f, 1), "Move: W");
            ImGui::TextColored(op == ImGuizmo::ROTATE ? ImVec4(1, 1, 0, 1) : ImVec4(0.6f, 0.6f, 0.6f, 1), "Rotate: E");
            ImGui::TextColored(op == ImGuizmo::SCALE ? ImVec4(1, 1, 0, 1) : ImVec4(0.6f, 0.6f, 0.6f, 1), "Scale: R");
            ImGui::Unindent();
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
    }
}
