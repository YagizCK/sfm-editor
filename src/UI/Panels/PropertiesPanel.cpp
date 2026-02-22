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

#include <algorithm>
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

    void PropertiesPanel::renderImageWithTooltip(const UITexture& tex, const std::string& imgName, uint32_t image_id,
                                                 int point2D_idx) {
        if (tex.id == 0) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Image not found on disk!");
            ImGui::TextDisabled("Expected Path:\n%s", (m_scene->imageBasePath + "\\" + imgName).c_str());
            return;
        }

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

        const ImVec2 inlineStartPos = ImGui::GetCursorScreenPos();
        ImGui::Image(tex.id, ImVec2(drawWidth, drawHeight));

        auto drawFeatures = [&](ImDrawList* dl, ImVec2 startPos, float width, float height, float baseSize) {
            if (image_id == 0 || !m_scene->cameras.contains(image_id)) return;
            const auto& camPose = m_scene->cameras.at(image_id);

            if (point2D_idx != -1) {
                if (point2D_idx < camPose.features.size()) {
                    const glm::vec2 rawCoord = camPose.features[point2D_idx].coordinates;
                    const float normX = rawCoord.x / static_cast<float>(tex.width);
                    const float normY = rawCoord.y / static_cast<float>(tex.height);
                    const ImVec2 center(startPos.x + normX * width, startPos.y + normY * height);
                    dl->AddCircle(center, baseSize * 2.0f, IM_COL32(255, 50, 50, 255), 0, 2.0f);
                }
            } else {
                for (const auto& feat : camPose.features) {
                    const float normX = feat.coordinates.x / static_cast<float>(tex.width);
                    const float normY = feat.coordinates.y / static_cast<float>(tex.height);
                    const ImVec2 center(startPos.x + normX * width, startPos.y + normY * height);

                    dl->AddCircle(center, baseSize * 2.0f,IM_COL32(0, 255, 100, 150), 0, 2.0f);
                }
            }
        };

        drawFeatures(ImGui::GetWindowDrawList(), inlineStartPos, drawWidth, drawHeight, 1.0f);

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

            const ImVec2 ttStartPos = ImGui::GetCursorScreenPos();
            ImGui::Image(tex.id, ImVec2(ttWidth, ttHeight));

            drawFeatures(ImGui::GetWindowDrawList(), ttStartPos, ttWidth, ttHeight, 2.0f);

            ImGui::TextDisabled("Original Size: %dx%d | Shown at: %.0f%%", tex.width, tex.height, ttScale * 100.0f);
            ImGui::EndTooltip();
        } else {
            ImGui::TextDisabled("Resolution: %dx%d (Hover to Enlarge)", tex.width, tex.height);
        }
    }

    void PropertiesPanel::onRender() {
        ImGui::Begin("Properties");
        if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
            const size_t visiblePointCount = std::count_if(m_scene->points.begin(), m_scene->points.end(),
                                                           [](const auto& p) { return p.selected > -0.5f; });
            ImGui::Text("Total Points: %zu", visiblePointCount);
            ImGui::Text("Total Cameras: %zu", m_scene->cameras.size());
            const ImGuiIO& io = ImGui::GetIO();
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::Text("Frame Time: %.3f ms", io.DeltaTime * 1000.0f);
        }

        auto teleportCamera = [this](const CameraPose& cam) {
            glm::quat worldRot = cam.orientation;
            const glm::quat glCorrection = glm::angleAxis(glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            worldRot = worldRot * glCorrection;

            m_camera->teleportTo(cam.position, worldRot);

            if (cam.height > 0 && cam.focalLengthY > 0.0f) {
                const float fovY_rad = 2.0f * std::atan(static_cast<float>(cam.height) / (2.0f * cam.focalLengthY));
                m_camera->FOV = glm::degrees(fovY_rad);
            } else {
                m_camera->FOV = 60.0f;
            }

            m_camera->projectionMode = ProjectionMode::Perspective;
            m_camera->updateProjection();

            Logger::info("Teleported to camera " + cam.imageName);
        };

        static bool isIsolating = false;
        static std::vector<float> originalSelectionStates;

        auto isolateCameraFeatures = [&](const uint32_t camID, const bool isButtonActive) {
            if (isButtonActive && !isIsolating) {
                isIsolating = true;

                m_editorSystem->isolatedCameraID = camID;

                originalSelectionStates.resize(m_scene->points.size());

                for (size_t i = 0; i < m_scene->points.size(); ++i) {
                    originalSelectionStates[i] = m_scene->points[i].selected;
                    m_scene->points[i].selected = -1.0f;
                    m_editorSystem->getSelectionManager()->markAsChanged(i);
                }

                for (size_t i = 0; i < m_scene->metadata.size() && i < m_scene->points.size(); ++i) {
                    const auto& meta = m_scene->metadata[i];
                    bool seenByCamera = false;
                    for (const auto& obs : meta.observations) {
                        if (obs.image_id == camID) {
                            seenByCamera = true;
                            break;
                        }
                    }

                    if (seenByCamera) {
                        if (originalSelectionStates[i] > 0.5f) {
                            m_scene->points[i].selected = 1.0f;
                        } else if (originalSelectionStates[i] >= 0.0f && originalSelectionStates[i] <= 0.5f) {
                            m_scene->points[i].selected = 0.0f;
                        }

                        m_editorSystem->getSelectionManager()->markAsChanged(i);
                    }
                }
            } else if (!isButtonActive && isIsolating) {
                isIsolating = false;

                m_editorSystem->isolatedCameraID = 0;

                for (size_t i = 0; i < m_scene->points.size(); ++i) {
                    m_scene->points[i].selected = originalSelectionStates[i];
                    m_editorSystem->getSelectionManager()->markAsChanged(i);
                }
                originalSelectionStates.clear();
            }
        };

        if (ImGui::CollapsingHeader("Selected Point Info", ImGuiTreeNodeFlags_DefaultOpen)) {
            const auto& selectedIndices = m_editorSystem->getSelectionManager()->selectedPointIndices;

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
                            if (m_scene->cameras.contains(obs.image_id)) {
                                if (ImGui::Button("Teleport Here", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) {
                                    teleportCamera(m_scene->cameras.at(obs.image_id));
                                }

                                ImGui::Button("Hold to Isolate Features",
                                              ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
                                isolateCameraFeatures(obs.image_id, ImGui::IsItemActive());
                            }

                            const std::string fullPath = m_scene->imageBasePath + "\\" + imgName;
                            const UITexture tex = getOrLoadImage(fullPath);
                            renderImageWithTooltip(tex, imgName, obs.image_id, obs.point2D_idx);
                            ImGui::TreePop();
                        }
                    }
                    ImGui::EndChild();
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No metadata available.");
                }
            }
        }

        if (ImGui::CollapsingHeader("Selected Camera Info", ImGuiTreeNodeFlags_DefaultOpen)) {
            const auto& selCams = m_editorSystem->getSelectionManager()->selectedCameraIDs;

            if (selCams.empty()) {
                ImGui::TextDisabled("No camera selected.");
            } else if (selCams.size() > 1) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Multiple cameras selected (%zu)", selCams.size());
            } else {
                uint32_t camID = selCams[0];
                if (m_scene->cameras.contains(camID)) {
                    const auto& cam = m_scene->cameras.at(camID);

                    ImGui::Text("Camera ID: %u", cam.cameraID);
                    ImGui::Text("Image: %s", cam.imageName.c_str());

                    const char* modelNames[] = {
                        "SIMPLE_PINHOLE", "PINHOLE", "SIMPLE_RADIAL", "RADIAL", "OPENCV", "OPENCV_FISHEYE"
                    };
                    const std::string modelStr = (cam.modelId >= 0 && cam.modelId <= 5)
                                                     ? modelNames[cam.modelId]
                                                     : "UNKNOWN";

                    ImGui::TextDisabled("Model: %s", modelStr.c_str());
                    ImGui::TextDisabled("Resolution: %llu x %llu", cam.width, cam.height);
                    ImGui::TextDisabled("Focal Length: %.1f, %.1f", cam.focalLength, cam.focalLengthY);
                    ImGui::TextDisabled("Principal Pt: %.1f, %.1f", cam.principalPointX, cam.principalPointY);

                    if (cam.extraParams.size() > 4) {
                        if (ImGui::TreeNode("Distortion Params")) {
                            for (size_t i = 4; i < cam.extraParams.size(); ++i) {
                                ImGui::Text("p[%zu]: %f", i, cam.extraParams[i]);
                            }
                            ImGui::TreePop();
                        }
                    }

                    ImGui::Separator();
                    ImGui::Text("Features: %zu points", cam.features.size());

                    ImGui::Dummy(ImVec2(0.0f, 2.0f));
                    if (ImGui::Button("Teleport Here", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) {
                        teleportCamera(cam);
                    }

                    ImGui::Button("Hold to Isolate Features", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
                    isolateCameraFeatures(camID, ImGui::IsItemActive());

                    ImGui::Dummy(ImVec2(0.0f, 2.0f));

                    const std::string fullPath = m_scene->imageBasePath + "\\" + cam.imageName;
                    const UITexture tex = getOrLoadImage(fullPath);
                    renderImageWithTooltip(tex, cam.imageName, camID);
                }
            }
        }

        if (ImGui::CollapsingHeader("Scene Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ColorEdit3("Background", &m_sceneProperties->backgroundColor.x);
            ImGui::Checkbox("Show Grid", &m_sceneProperties->showGrid);
            ImGui::Checkbox("Show Axes", &m_sceneProperties->showAxes);
            ImGui::DragFloat("Point Size", &m_sceneProperties->pointSize, 0.1f, 0.1f, 100.0f);
            ImGui::DragFloat("Camera Size", &m_sceneProperties->cameraSize, 0.1f, 0.1f, 100.0f);
        }

        if (ImGui::CollapsingHeader("Transform Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Enable Snapping", &m_editorSystem->useSnap);

            if (m_editorSystem->useSnap) {
                ImGui::Indent();
                ImGui::DragFloat("Translate Snap", &m_editorSystem->snapTranslation, 0.1f, 0.1f, 100.0f, "%.2f units");
                ImGui::DragFloat("Rotate Snap", &m_editorSystem->snapRotation, 1.0f, 1.0f, 180.0f, "%.1f deg");
                ImGui::DragFloat("Scale Snap", &m_editorSystem->snapScale, 0.1f, 0.1f, 10.0f, "%.2f x");
                ImGui::Unindent();
            }
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
