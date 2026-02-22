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

#include "EditorSystem.h"

#include "Input.h"
#include "KeyCodes.hpp"
#include "Logger.h"
#include "Core/Events.hpp"

#include <format>


namespace sfmeditor {
    EditorSystem::EditorSystem(EditorCamera* camera, SfMScene* scene)
        : m_camera(camera), m_scene(scene) {
        m_selectionManager = std::make_unique<SelectionManager>(this, scene);
        m_actionHistory = std::make_unique<ActionHistory>(this, m_selectionManager.get(), scene);

        setupInputCallbacks();
    }

    void EditorSystem::setupInputCallbacks() {
        Events::onMouseButton.connect([this](const int button, const int action) {
            if (!m_viewportInfo.focused || !m_viewportInfo.hovered) return;

            if (!ImGuizmo::IsUsing() && button == SFM_MOUSE_BUTTON_LEFT) {
                if (!ImGuizmo::IsOver() && action == SFM_PRESS) {
                    boxSelecting = true;
                    boxStart = Input::getVpRelativeMousePos(m_viewportInfo);
                    boxEnd = boxStart;
                } else if (action == SFM_RELEASE && boxSelecting) {
                    boxSelecting = false;
                    const glm::vec2 end = Input::getVpRelativeMousePos(m_viewportInfo);

                    if (glm::dot(glm::abs(end - boxStart), glm::abs(end - boxStart)) < m_boxSelectSqThreshold) {
                        uint32_t hitCameraID = 0;
                        float minDepth = FLT_MAX;
                        const glm::mat4 vp = m_camera->getViewProjection();

                        for (const auto& [id, cam] : m_scene->cameras) {
                            glm::vec4 clipPos = vp * glm::vec4(cam.position, 1.0f);
                            if (clipPos.w > 0.01f) {
                                glm::vec2 ndc = glm::vec2(clipPos.x, clipPos.y) / clipPos.w;
                                glm::vec2 screenPos = {
                                    (ndc.x * 0.5f + 0.5f) * m_viewportInfo.size.x,
                                    (0.5f - ndc.y * 0.5f) * m_viewportInfo.size.y
                                };
                                if (glm::distance(screenPos, end) < 25.0f && clipPos.w < minDepth) {
                                    minDepth = clipPos.w;
                                    hitCameraID = id;
                                }
                            }
                        }

                        if (hitCameraID != 0) {
                            const bool isCtrl = Input::isKeyPressed(SFM_KEY_LEFT_CONTROL);
                            if (!isCtrl) m_selectionManager->clearSelection();

                            auto& camList = m_selectionManager->selectedCameraIDs;
                            if (std::find(camList.begin(), camList.end(), hitCameraID) != camList.
                                end())
                                m_selectionManager->removeCameraFromSelection(hitCameraID);
                            else m_selectionManager->addCameraToSelection(hitCameraID);
                            updateGizmoCenter();
                        } else {
                            pendingPickedID = true;
                        }
                    } else {
                        boxEnd = end;
                        m_selectionManager->processBoxSelection(m_camera->getViewProjection(), m_viewportInfo, boxStart,
                                                                boxEnd, Input::isKeyPressed(SFM_KEY_LEFT_CONTROL));
                    }
                }
            }
        });

        Events::onKey.connect([this](const int key, const int action) {
            if (!m_viewportInfo.focused || action != SFM_PRESS) return;

            if (Input::isKeyPressed(SFM_KEY_LEFT_CONTROL) && key == SFM_KEY_A) {
                m_selectionManager->selectAll();
            }

            if (!Input::isMouseButtonPressed(SFM_MOUSE_BUTTON_RIGHT)) {
                switch (key) {
                case SFM_KEY_Q: gizmoOperation = -1;
                    break;
                case SFM_KEY_W: gizmoOperation = ImGuizmo::TRANSLATE;
                    break;
                case SFM_KEY_E: gizmoOperation = ImGuizmo::ROTATE;
                    break;
                case SFM_KEY_R: gizmoOperation = ImGuizmo::SCALE;
                    break;
                }
            }

            if (key == SFM_KEY_DELETE && m_selectionManager->hasSelection()) {
                m_actionHistory->executeDelete();
            }
        });
    }

    void EditorSystem::onUpdate(const ViewportInfo& viewportInfo) {
        m_viewportInfo = viewportInfo;

        const bool isUsingGizmo = ImGuizmo::IsUsing();

        if (isUsingGizmo && !m_wasUsingGizmo) {
            m_dragStartStates.clear();
            m_dragStartCamStates.clear();
            for (const unsigned int idx : m_selectionManager->selectedPointIndices) {
                m_dragStartStates.push_back({idx, m_scene->points[idx].position, m_scene->points[idx].selected});
            }
            for (const unsigned int id : m_selectionManager->selectedCameraIDs) {
                if (m_scene->cameras.contains(id)) m_dragStartCamStates.push_back({id, m_scene->cameras.at(id)});
            }
        } else if (!isUsingGizmo && m_wasUsingGizmo) {
            std::vector<PointState> newPointStates;
            std::vector<std::pair<uint32_t, CameraPose>> newCamStates;

            for (const unsigned int idx : m_selectionManager->selectedPointIndices) {
                newPointStates.push_back({idx, m_scene->points[idx].position, m_scene->points[idx].selected});
            }
            for (const unsigned int id : m_selectionManager->selectedCameraIDs) {
                if (m_scene->cameras.contains(id)) newCamStates.push_back({id, m_scene->cameras.at(id)});
            }

            m_actionHistory->recordTransformAction(m_dragStartStates, newPointStates, m_dragStartCamStates,
                                                   newCamStates);
        }
        m_wasUsingGizmo = isUsingGizmo;

        if (m_selectionManager->hasSelection() && gizmoOperation != -1 && isUsingGizmo) {
            bool isMatrixChanged = false;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    if (std::abs(gizmoTransform[i][j] - gizmoLastTransform[i][j]) > 1e-5f) {
                        isMatrixChanged = true;
                        break;
                    }
                }
                if (isMatrixChanged) break;
            }

            if (isMatrixChanged) {
                const glm::mat4 deltaTransform = gizmoTransform * glm::inverse(gizmoLastTransform);
                const glm::quat deltaRot = glm::quat_cast(deltaTransform);

                for (const unsigned int idx : m_selectionManager->selectedPointIndices) {
                    m_scene->points[idx].position = glm::vec3(
                        deltaTransform * glm::vec4(m_scene->points[idx].position, 1.0f));
                    m_selectionManager->markAsChanged(idx);
                }

                for (const uint32_t camID : m_selectionManager->selectedCameraIDs) {
                    if (m_scene->cameras.contains(camID)) {
                        auto& cam = m_scene->cameras.at(camID);
                        cam.position = glm::vec3(deltaTransform * glm::vec4(cam.position, 1.0f));
                        cam.orientation = glm::normalize(deltaRot * cam.orientation);
                    }
                }

                gizmoLastTransform = gizmoTransform;
            }
        }

        if (boxSelecting) {
            const glm::vec2 end = Input::getVpRelativeMousePos(m_viewportInfo);
            if (glm::dot(glm::abs(end - boxStart), glm::abs(end - boxStart)) >= m_boxSelectSqThreshold) {
                boxEnd = end;
            }
        }
    }

    void EditorSystem::updateGizmoCenter() {
        if (m_selectionManager->hasSelection()) {
            glm::vec3 center(0.0f);
            float count = 0;
            for (const unsigned int idx : m_selectionManager->selectedPointIndices) {
                if (m_scene->points[idx].selected > -0.5f) {
                    center += m_scene->points[idx].position;
                    count++;
                }
            }
            for (const uint32_t id : m_selectionManager->selectedCameraIDs) {
                if (m_scene->cameras.contains(id)) {
                    center += m_scene->cameras.at(id).position;
                    count++;
                }
            }
            if (count > 0) {
                center /= count;
            } else {
                center = glm::vec3(0.0f);
            }

            gizmoTransform = glm::translate(glm::mat4(1.0f), center);
            gizmoLastTransform = gizmoTransform;
        }
    }

    void EditorSystem::getSnapValues(float* snapArray) const {
        if (gizmoOperation == ImGuizmo::TRANSLATE) {
            snapArray[0] = snapTranslation;
            snapArray[1] = snapTranslation;
            snapArray[2] = snapTranslation;
        } else if (gizmoOperation == ImGuizmo::ROTATE) {
            snapArray[0] = snapRotation;
            snapArray[1] = snapRotation;
            snapArray[2] = snapRotation;
        } else if (gizmoOperation == ImGuizmo::SCALE) {
            snapArray[0] = snapScale;
            snapArray[1] = snapScale;
            snapArray[2] = snapScale;
        }
    }
}
