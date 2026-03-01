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
#include <glm/gtc/type_ptr.hpp>


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

        auto drawFeatures = [&](ImDrawList* dl, const ImVec2 startPos, const float width, const float height,
                                const float baseSize) {
            if (image_id == 0 || !m_scene->images.contains(image_id)) return;
            const auto& imgPose = m_scene->images.at(image_id);

            if (point2D_idx != -1) {
                if (point2D_idx < imgPose.features.size()) {
                    const glm::vec2 rawCoord = imgPose.features[point2D_idx].coordinates;
                    const float normX = rawCoord.x / static_cast<float>(tex.width);
                    const float normY = rawCoord.y / static_cast<float>(tex.height);
                    const ImVec2 center(startPos.x + normX * width, startPos.y + normY * height);
                    dl->AddCircle(center, baseSize * 2.0f, IM_COL32(255, 50, 50, 255), 0, 2.0f);
                }
            } else {
                for (const auto& feat : imgPose.features) {
                    if (feat.point3D_id == static_cast<uint64_t>(-1)) {
                        continue;
                    }

                    const float normX = feat.coordinates.x / static_cast<float>(tex.width);
                    const float normY = feat.coordinates.y / static_cast<float>(tex.height);
                    const ImVec2 center(startPos.x + normX * width, startPos.y + normY * height);

                    dl->AddCircle(center, baseSize * 2.0f, IM_COL32(0, 255, 100, 150), 0, 2.0f);
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

        auto teleportCamera = [this](const CameraPose& imgPose) {
            glm::quat worldRot = imgPose.orientation;
            const glm::quat glCorrection = glm::angleAxis(glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            worldRot = worldRot * glCorrection;

            m_camera->teleportTo(imgPose.position, worldRot);

            if (m_scene->cameras.contains(imgPose.cameraID)) {
                const auto& camLens = m_scene->cameras.at(imgPose.cameraID);

                if (camLens.height > 0 && camLens.focalLengthY > 0.0f) {
                    const float fovY_rad = 2.0f * std::atan(
                        static_cast<float>(camLens.height) / (2.0f * camLens.focalLengthY));
                    m_camera->FOV = glm::degrees(fovY_rad);
                } else {
                    m_camera->FOV = 60.0f;
                }

                m_camera->lensModel = camLens.modelId;
                for (float& distParam : m_camera->distParams) distParam = 0.0f;

                const int offset = (camLens.modelId == 0 || camLens.modelId == 2 || camLens.modelId == 3) ? 3 : 4;

                for (size_t i = offset; i < camLens.extraParams.size() && (i - offset) < 8; ++i) {
                    m_camera->distParams[i - offset] = static_cast<float>(camLens.extraParams[i]);
                }

                if (camLens.width > 0 && camLens.height > 0) {
                    m_camera->principalPoint.x = camLens.principalPointX / static_cast<float>(camLens.width);
                    m_camera->principalPoint.y = camLens.principalPointY / static_cast<float>(camLens.height);
                }
            } else {
                m_camera->FOV = 60.0f;
                m_camera->lensModel = 0;
                m_camera->principalPoint = {0.5f, 0.5f};
            }

            m_camera->projectionMode = ProjectionMode::Perspective;
            m_camera->updateProjection();

            Logger::info("Teleported to image " + imgPose.imageName);
        };

        static bool isIsolating = false;
        static std::vector<float> originalSelectionStates;

        auto isolateImageFeatures = [&](const uint32_t camID, const bool isButtonActive) {
            if (isButtonActive && !isIsolating) {
                isIsolating = true;

                m_editorSystem->isolatedImageID = camID;

                originalSelectionStates.resize(m_scene->points.size());

                for (size_t i = 0; i < m_scene->points.size(); ++i) {
                    originalSelectionStates[i] = m_scene->points[i].selected;
                    m_scene->points[i].selected = -1.0f;
                    m_editorSystem->getSelectionManager()->markAsChanged(static_cast<unsigned int>(i));
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

                        m_editorSystem->getSelectionManager()->markAsChanged(static_cast<unsigned int>(i));
                    }
                }
            } else if (!isButtonActive && isIsolating) {
                isIsolating = false;

                m_editorSystem->isolatedImageID = 0;

                for (size_t i = 0; i < m_scene->points.size(); ++i) {
                    m_scene->points[i].selected = originalSelectionStates[i];
                    m_editorSystem->getSelectionManager()->markAsChanged(static_cast<unsigned int>(i));
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
                        if (m_scene->images.contains(obs.image_id)) {
                            imgName = m_scene->images.at(obs.image_id).imageName;
                        }

                        if (ImGui::TreeNode((void*)static_cast<intptr_t>(obs.image_id), "%s (Feature: %d)",
                                            imgName.c_str(), obs.point2D_idx)) {
                            if (m_scene->images.contains(obs.image_id)) {
                                if (ImGui::Button("Teleport Here", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) {
                                    teleportCamera(m_scene->images.at(obs.image_id));
                                }

                                ImGui::Button("Hold to Isolate Features",
                                              ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
                                isolateImageFeatures(obs.image_id, ImGui::IsItemActive());
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

        if (ImGui::CollapsingHeader("Selected Image Info", ImGuiTreeNodeFlags_DefaultOpen)) {
            const auto& selCams = m_editorSystem->getSelectionManager()->selectedImageIDs;

            if (selCams.empty()) {
                ImGui::TextDisabled("No image selected.");
            } else if (selCams.size() > 1) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Multiple images selected (%zu)", selCams.size());
            } else {
                uint32_t imageID = selCams[0];
                if (m_scene->images.contains(imageID)) {
                    const auto& img = m_scene->images.at(imageID);

                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Image: %s", img.imageName.c_str());
                    ImGui::Text("Image ID: %u", img.imageID);
                    ImGui::Text("Sensor / Camera ID: %u", img.cameraID);

                    if (m_scene->cameras.contains(img.cameraID)) {
                        auto& cam = m_scene->cameras.at(img.cameraID);

                        const char* modelNames[] = {
                            "SIMPLE_PINHOLE", "PINHOLE", "SIMPLE_RADIAL", "RADIAL", "OPENCV", "OPENCV_FISHEYE",
                            "FULL_OPENCV"
                        };
                        const std::string modelStr = (cam.modelId >= 0 && cam.modelId <= 6)
                                                         ? modelNames[cam.modelId]
                                                         : "UNKNOWN";

                        ImGui::TextDisabled("Model: %s", modelStr.c_str());
                        ImGui::TextDisabled("Resolution: %llu x %llu", cam.width, cam.height);

                        ImGui::Separator();
                        if (ImGui::TreeNodeEx("Sensor Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
                            const int offset = (cam.modelId == 0 || cam.modelId == 2 || cam.modelId == 3) ? 3 : 4;

                            for (size_t i = 0; i < offset && i < cam.extraParams.size(); ++i) {
                                const float val = static_cast<float>(cam.extraParams[i]);
                                std::string label = (i == 0)
                                                        ? "f / fx"
                                                        : (i == 1 && offset == 4)
                                                        ? "fy"
                                                        : (i == offset - 2)
                                                        ? "cx"
                                                        : "cy";

                                ImGui::Text("%s: %.3f", label.c_str(), val);
                            }

                            if (cam.extraParams.size() > offset) {
                                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Distortion Parameters");
                                for (size_t i = offset; i < cam.extraParams.size(); ++i) {
                                    const float val = static_cast<float>(cam.extraParams[i]);
                                    ImGui::Text("param[%zu]: %f", i - offset, val);
                                }
                            }
                            ImGui::TreePop();
                        }
                    }

                    ImGui::Separator();
                    const size_t numTriangulated = std::count_if(
                        img.features.begin(),
                        img.features.end(),
                        [](const auto& f) { return f.point3D_id != -1; }
                    );
                    ImGui::Text("Features: %zu points (%zu triangulated)",
                                img.features.size(),
                                numTriangulated);

                    ImGui::Dummy(ImVec2(0.0f, 2.0f));
                    if (ImGui::Button("Teleport Here", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) {
                        teleportCamera(img);
                    }

                    ImGui::Button("Hold to Isolate Features", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
                    isolateImageFeatures(imageID, ImGui::IsItemActive());

                    ImGui::Dummy(ImVec2(0.0f, 2.0f));

                    const std::string fullPath = m_scene->imageBasePath + "\\" + img.imageName;
                    const UITexture tex = getOrLoadImage(fullPath);
                    renderImageWithTooltip(tex, img.imageName, imageID);
                }
            }
        }

        if (ImGui::CollapsingHeader("Scene Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ColorEdit3("Background", &m_sceneProperties->backgroundColor.x);
            ImGui::Checkbox("Show Grid", &m_sceneProperties->showGrid);
            ImGui::Checkbox("Show Axes", &m_sceneProperties->showAxes);
            ImGui::Checkbox("Show Points", &m_sceneProperties->showPoints);
            ImGui::Checkbox("Show Cameras", &m_sceneProperties->showCameras);
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

            ImGui::Separator();
            ImGui::Text("Editor Lens Distortion");

            const char* lensModels[] = {
                "Pinhole", "Pinhole", "Simple Radial", "Radial", "OpenCV", "OpenCV Fisheye", "Full OpenCV"
            };
            int currentLens = m_camera->lensModel;
            if (ImGui::Combo("Lens Model", &currentLens, lensModels, 7)) {
                m_camera->lensModel = currentLens;
            }
            if (m_camera->lensModel > 1) {
                if (m_camera->lensModel == 6) {
                    ImGui::DragFloat4("k1, k2, p1, p2", &m_camera->distParams[0], 0.001f, -2.0f, 2.0f);
                    ImGui::DragFloat4("k3, k4, k5, k6", &m_camera->distParams[4], 0.001f, -2.0f, 2.0f);
                } else if (m_camera->lensModel == 5) {
                    ImGui::DragFloat4("k1, k2, k3, k4", &m_camera->distParams[0], 0.001f, -2.0f, 2.0f);
                } else {
                    ImGui::DragFloat4("k1, k2, p1, p2", &m_camera->distParams[0], 0.001f, -2.0f, 2.0f);
                }

                ImGui::DragFloat2("Principal Offset", glm::value_ptr(m_camera->principalPoint), 0.01f, 0.0f, 1.0f);
            }
        }

        ImGui::End();
    }
}
