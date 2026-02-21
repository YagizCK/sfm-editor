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

#include "PropertiesPanel.h"

#include "Core/Logger.h"

#include <imgui.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glad/glad.h>
#include <format>


namespace sfmeditor {
    PropertiesPanel::PropertiesPanel(SceneProperties* sceneProperties, EditorCamera* camera, SfMScene* scene,
                                     EditorSystem* editorSystem)
        : m_sceneProperties(sceneProperties), m_camera(camera), m_scene(scene), m_editorSystem(editorSystem) {
    }

    UITexture PropertiesPanel::getOrLoadImage(const std::string& filepath) {
        if (m_imageCache.contains(filepath)) {
            return m_imageCache[filepath];
        }

        UITexture tex;
        int channels;

        if (unsigned char* data = stbi_load(filepath.c_str(), &tex.width, &tex.height, &channels, 4)) {
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

    void PropertiesPanel::onRender() {
        ImGui::Begin("Properties");
        if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Total Points: %zu", m_scene->points.size());
            ImGui::Text("Total Cameras: %zu", m_scene->cameras.size());
            const ImGuiIO& io = ImGui::GetIO();
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::Text("Frame Time: %.3f ms", io.DeltaTime * 1000.0f);
        }

        if (ImGui::CollapsingHeader("Selected Point Info", ImGuiTreeNodeFlags_DefaultOpen)) {
            const auto& selectedIndices = m_editorSystem->selectedPointIndices;

            if (selectedIndices.empty()) {
                ImGui::TextDisabled("No point selected.");
            } else if (selectedIndices.size() > 1) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Multiple points selected (%zu)",
                                   selectedIndices.size());
            } else {
                unsigned int pointIdx = selectedIndices[0];

                if (pointIdx < m_scene->metadata.size()) {
                    const auto& meta = m_scene->metadata[pointIdx];

                    ImGui::Text("ID: %llu", meta.original_id);
                    ImGui::Text("Reproj. Error: %.4f px", meta.error);
                    ImGui::Separator();
                    ImGui::Text("Observed in %zu Images:", meta.observations.size());

                    ImGui::BeginChild("TrackList", ImVec2(0, 300), true);

                    for (const auto& obs : meta.observations) {
                        std::string imgName = "Unknown";
                        if (m_scene->cameras.contains(obs.image_id)) {
                            imgName = m_scene->cameras.at(obs.image_id).imageName;
                        }

                        if (ImGui::TreeNode((void*)static_cast<intptr_t>(obs.image_id), "%s (Feature: %d)",
                                            imgName.c_str(), obs.point2D_idx)) {
                            std::string fullPath = m_scene->imageBasePath + "\\" + imgName;
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

                                    if (m_scene->cameras.contains(obs.image_id)) {
                                        const auto& camPose = m_scene->cameras.at(obs.image_id);
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
            ImGui::ColorEdit3("Background", &m_sceneProperties->backgroundColor.x);
            ImGui::Checkbox("Show Grid", &m_sceneProperties->showGrid);
            ImGui::Checkbox("Show Axes", &m_sceneProperties->showAxes);
            ImGui::DragFloat("Point Size", &m_sceneProperties->pointSize, 0.1f, 0.1f, 100.0f);
        }

        if (ImGui::CollapsingHeader("Camera Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Position", &m_camera->position.x, 0.1f);

            ImGui::Separator();
            ImGui::Text("Rotation (Euler)");

            bool eulerChanged = false;
            if (ImGui::DragFloat("Pitch", &m_camera->pitch, 0.5f)) eulerChanged = true;
            if (ImGui::DragFloat("Yaw", &m_camera->yaw, 0.5f)) eulerChanged = true;
            if (ImGui::DragFloat("Roll", &m_camera->roll, 0.5f)) eulerChanged = true;

            if (eulerChanged) m_camera->setRotationFromUI();

            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            ImGui::Text("Rotation (Quaternion)");

            glm::quat currentQuat = m_camera->orientation;
            if (ImGui::DragFloat4("X Y Z W", &currentQuat.x, 0.01f, -1.0f, 1.0f)) {
                m_camera->setOrientationFromUI(currentQuat);
            }

            ImGui::Separator();

            if (ImGui::Button("Reset View to Origin")) {
                m_camera->resetView();
            }
        }

        if (ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* cameraStyleStrings[] = {"Free Look (Fly)", "Orbit (Turntable)"};
            int currentStyle = static_cast<int>(m_camera->cameraStyle);
            if (ImGui::Combo("Camera Mode", &currentStyle, cameraStyleStrings, 2)) {
                m_camera->setCameraStyle(static_cast<CameraStyle>(currentStyle));
            }

            ImGui::Separator();

            bool projectionChanged = false;

            const char* projectionTypeStrings[] = {"Perspective", "Orthographic"};
            int currentProj = static_cast<int>(m_camera->projectionMode);
            if (ImGui::Combo("Projection", &currentProj, projectionTypeStrings, 2)) {
                m_camera->projectionMode = static_cast<ProjectionMode>(currentProj);
                projectionChanged = true;
            }

            if (m_camera->projectionMode == ProjectionMode::Perspective) {
                if (ImGui::SliderFloat("FOV", &m_camera->FOV, 1.0f, 179.0f)) projectionChanged = true;
            } else {
                if (ImGui::DragFloat("Ortho Size", &m_camera->orthoSize, 0.1f, 0.1f, 1000.0f))
                    projectionChanged =
                        true;
            }

            if (projectionChanged) m_camera->updateProjection();

            ImGui::Separator();

            ImGui::DragFloat("Speed", &m_camera->movementSpeed, 0.1f, m_camera->minMovementSpeed, 500.0f);
            ImGui::SliderFloat("Sensitivity", &m_camera->mouseSensitivity, 0.001f, 0.1f);
            ImGui::SliderFloat("Scroll Sens.", &m_camera->scrollSensitivity, 0.1f, 50.0f);
        }

        ImGui::End();
    }
}
