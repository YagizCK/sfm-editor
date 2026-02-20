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


namespace sfmeditor {
    EditorSystem::EditorSystem(EditorCamera* camera, LineRenderer* lineRenderer, std::vector<Point>* points)
        : m_camera(camera), m_lineRenderer(lineRenderer), m_points(points) {
        Events::onMouseButton.connect([this](const int button, const int action) {
            if (!m_viewportInfo.focused || !m_viewportInfo.hovered) return;

            if (!ImGuizmo::IsUsing() && button == SFM_MOUSE_BUTTON_LEFT) {
                if (!ImGuizmo::IsOver() && action == SFM_PRESS) {
                    if (!Input::isKeyPressed(SFM_KEY_LEFT_CONTROL)) {
                        clearSelection();
                    }

                    pendingSelection = true;

                    boxSelecting = true;
                    boxStart = Input::getVpRelativeMousePos(m_viewportInfo);
                    boxEnd = boxStart;
                } else if (action == SFM_RELEASE) {
                    boxSelecting = false;
                    boxEnd = Input::getVpRelativeMousePos(m_viewportInfo);

                    const glm::vec2 d = glm::abs(boxEnd - boxStart);

                    if (const float distSq = glm::dot(d, d); distSq < 9.0f) {
                        if (const int closestIdx = getClosestPointIdx(36.0f); closestIdx != -1) {
                            if (Input::isKeyPressed(SFM_KEY_LEFT_CONTROL) && (*m_points)[closestIdx].selected > 0.5f) {
                                (*m_points)[closestIdx].selected = 0.0f;
                                changedIndices.push_back(closestIdx);
                            } else if ((*m_points)[closestIdx].selected < 0.5f) {
                                selectedPointIndices.push_back(static_cast<unsigned int>(closestIdx));
                                (*m_points)[closestIdx].selected = 1.0f;
                                changedIndices.push_back(closestIdx);
                            }
                        }
                    } else {
                        const float minX = std::min(boxStart.x, boxEnd.x);
                        const float maxX = std::max(boxStart.x, boxEnd.x);
                        const float minY = std::min(boxStart.y, boxEnd.y);
                        const float maxY = std::max(boxStart.y, boxEnd.y);

                        const glm::mat4 vp = m_camera->getViewProjection();
                        const bool isCtrlPressed = Input::isKeyPressed(SFM_KEY_LEFT_CONTROL);

                        for (int i = 0; i < std::ssize(*m_points); ++i) {
                            glm::vec4 clipPos = vp * glm::vec4((*m_points)[i].position, 1.0f);

                            if (clipPos.w <= 0.0f) continue;

                            const glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;

                            const float screenX = (ndc.x + 1.0f) * 0.5f * m_viewportInfo.size.x;
                            const float screenY = (1.0f - ndc.y) * 0.5f * m_viewportInfo.size.y;

                            if (screenX >= minX && screenX <= maxX && screenY >= minY && screenY <= maxY) {
                                if (isCtrlPressed && (*m_points)[i].selected > 0.5f) {
                                    (*m_points)[i].selected = 0.0f;
                                    changedIndices.push_back(i);
                                } else if ((*m_points)[i].selected < 0.5f) {
                                    selectedPointIndices.push_back(i);
                                    (*m_points)[i].selected = 1.0f;
                                    changedIndices.push_back(i);
                                }
                            }
                        }
                    }

                    if (hasSelection()) {
                        Logger::info(std::to_string(selectedPointIndices.size()) + " points selected.");

                        glm::vec3 center(0.0f);
                        for (const unsigned int idx : selectedPointIndices) {
                            center += (*m_points)[idx].position;
                        }
                        center /= static_cast<float>(selectedPointIndices.size());

                        gizmoTransform = glm::translate(glm::mat4(1.0f), center);
                        gizmoLastTransform = gizmoTransform;
                    }

                    pendingSelection = true;
                }
            }
        });

        Events::onKey.connect([this](const int key, const int action) {
            if (!m_viewportInfo.focused) return;
            if (action != SFM_PRESS) return;

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

            if (key == SFM_KEY_DELETE && hasSelection()) {
                pendingDeletion = true;
            }
        });
    }

    void EditorSystem::onUpdate(const ViewportInfo& viewportInfo) {
        m_viewportInfo = viewportInfo;

        if (boxSelecting) boxEnd = Input::getVpRelativeMousePos(m_viewportInfo);
    }

    bool EditorSystem::hasSelection() const {
        return !selectedPointIndices.empty();
    }

    void EditorSystem::clearSelection() {
        for (const unsigned int idx : selectedPointIndices) {
            if (m_points->size() > idx) {
                (*m_points)[idx].selected = 0.0f;
                changedIndices.push_back(idx);
            }
        }
        selectedPointIndices.clear();
    }

    void EditorSystem::resetState() {
        boxSelecting = false;
        pendingSelection = false;
        pendingDeletion = false;
        selectedPointIndices.clear();
        changedIndices.clear();
    }

    bool EditorSystem::projectToViewport(const glm::vec3& worldPos, glm::vec2& outScreenPos) const {
        const glm::vec4 clip = m_camera->getViewProjection() * glm::vec4(worldPos, 1.0f);

        if (clip.w <= 0.0f) return false;

        const glm::vec3 ndc = glm::vec3(clip) / clip.w;

        if (ndc.x < -1 || ndc.x > 1 || ndc.y < -1 || ndc.y > 1) return false;

        outScreenPos.x = (ndc.x * 0.5f + 0.5f) * m_viewportInfo.size.x;
        outScreenPos.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * m_viewportInfo.size.y;

        return true;
    }

    int EditorSystem::getClosestPointIdx(const float maxDistanceSq) {
        int closestIdx = -1;
        float closestDist = maxDistanceSq;

        for (size_t i = 0; i < m_points->size(); ++i) {
            glm::vec2 screenPos;
            if (!projectToViewport((*m_points)[i].position, screenPos)) continue;

            const glm::vec2 d = screenPos - Input::getVpRelativeMousePos(m_viewportInfo);
            if (const float distSq = glm::dot(d, d); distSq < closestDist) {
                closestDist = distSq;
                closestIdx = static_cast<int>(i);
            }
        }

        return closestIdx;
    }
}
