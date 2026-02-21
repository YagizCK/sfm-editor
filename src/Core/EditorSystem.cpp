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

#include <format>

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
                    boxSelecting = true;
                    boxStart = Input::getVpRelativeMousePos(m_viewportInfo);
                    boxEnd = boxStart;
                } else if (action == SFM_RELEASE) {
                    boxSelecting = false;

                    const glm::vec2 end = Input::getVpRelativeMousePos(m_viewportInfo);
                    const glm::vec2 d = glm::abs(end - boxStart);

                    if (const float distSq = glm::dot(d, d); distSq < m_boxSelectSqThreshold) {
                        pendingPickedID = true;
                    } else {
                        boxEnd = end;
                        processBoxSelection(m_camera->getViewProjection(), Input::isKeyPressed(SFM_KEY_LEFT_CONTROL));
                    }
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

        if (boxSelecting) {
            const glm::vec2 end = Input::getVpRelativeMousePos(m_viewportInfo);
            const glm::vec2 d = glm::abs(end - boxStart);

            if (const float distSq = glm::dot(d, d); distSq >= m_boxSelectSqThreshold) {
                boxEnd = end;
            }
        }
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

        pendingSelection = true;
    }

    void EditorSystem::resetState() {
        boxSelecting = false;
        pendingPickedID = false;
        pendingSelection = false;
        pendingDeletion = false;
        selectedPointIndices.clear();
        changedIndices.clear();
    }

    void EditorSystem::processPickedID(const int pickedID, const bool isCtrlPressed) {
        if (!isCtrlPressed) clearSelection();

        if (pickedID < 0 || pickedID >= m_points->size()) {
            return;
        }

        if (isCtrlPressed && (*m_points)[pickedID].selected > 0.5f) {
            (*m_points)[pickedID].selected = 0.0f;
            changedIndices.push_back(pickedID);
            std::erase(selectedPointIndices, static_cast<unsigned int>(pickedID));
        } else if ((*m_points)[pickedID].selected < 0.5f) {
            selectedPointIndices.push_back(pickedID);
            (*m_points)[pickedID].selected = 1.0f;
            changedIndices.push_back(pickedID);
        }

        pendingSelection = true;

        updateGizmoCenter();
    }

    void EditorSystem::processBoxSelection(const glm::mat4& vpMatrix, const bool isCtrlPressed) {
        if (!isCtrlPressed) clearSelection();

        const float vpW = m_viewportInfo.size.x;
        const float vpH = m_viewportInfo.size.y;

        const float ndcMinX = (std::min(boxStart.x, boxEnd.x) / vpW) * 2.0f - 1.0f;
        const float ndcMaxX = (std::max(boxStart.x, boxEnd.x) / vpW) * 2.0f - 1.0f;

        const float ndcMinY = 1.0f - (std::max(boxStart.y, boxEnd.y) / vpH) * 2.0f;
        const float ndcMaxY = 1.0f - (std::min(boxStart.y, boxEnd.y) / vpH) * 2.0f;

        const auto rowX = glm::vec4(vpMatrix[0][0], vpMatrix[1][0], vpMatrix[2][0], vpMatrix[3][0]);
        const auto rowY = glm::vec4(vpMatrix[0][1], vpMatrix[1][1], vpMatrix[2][1], vpMatrix[3][1]);
        const auto rowW = glm::vec4(vpMatrix[0][3], vpMatrix[1][3], vpMatrix[2][3], vpMatrix[3][3]);

        const unsigned int pointCount = static_cast<unsigned int>(m_points->size());

        for (unsigned int i = 0; i < pointCount; ++i) {
            const glm::vec3& p = (*m_points)[i].position;

            const float w = rowW.x * p.x + rowW.y * p.y + rowW.z * p.z + rowW.w;
            if (w <= 0.0f) continue;

            const float x = rowX.x * p.x + rowX.y * p.y + rowX.z * p.z + rowX.w;
            if (x < ndcMinX * w || x > ndcMaxX * w) continue;

            const float y = rowY.x * p.x + rowY.y * p.y + rowY.z * p.z + rowY.w;
            if (y < ndcMinY * w || y > ndcMaxY * w) continue;

            if (isCtrlPressed && (*m_points)[i].selected > 0.5f) {
                (*m_points)[i].selected = 0.0f;
                changedIndices.push_back(i);
            } else if ((*m_points)[i].selected < 0.5f) {
                selectedPointIndices.push_back(i);
                (*m_points)[i].selected = 1.0f;
                changedIndices.push_back(i);
            }
        }

        std::erase_if(selectedPointIndices, [&](const unsigned int idx) {
            return (*m_points)[idx].selected < 0.5f;
        });

        pendingSelection = true;

        updateGizmoCenter();
    }

    void EditorSystem::updateGizmoCenter() {
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
    }
}
