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

#include "SelectionManager.h"

#include "EditorSystem.h"
#include "Logger.h"

#include <algorithm>


namespace sfmeditor {
    SelectionManager::SelectionManager(EditorSystem* editorSystem, SfMScene* scene)
        : m_editorSystem(editorSystem), m_scene(scene) {
    }

    bool SelectionManager::hasSelection() const {
        return !selectedPointIndices.empty() || !selectedImageIDs.empty();
    }

    void SelectionManager::clearSelection(const bool modifyScenePoints) {
        if (modifyScenePoints) {
            for (const unsigned int idx : selectedPointIndices) {
                m_scene->points[idx].selected = 0.0f;
                markAsChanged(idx);
            }
        }
        selectedPointIndices.clear();
        selectedImageIDs.clear();
    }

    void SelectionManager::selectAll(const bool selectPoints, const bool selectCameras) {
        if (selectPoints) {
            for (size_t i = 0; i < m_scene->points.size(); ++i) {
                if (m_scene->points[i].selected < -0.5f) continue;
                m_scene->points[i].selected = 1.0f;
                addPointToSelection(static_cast<unsigned int>(i));
                markAsChanged(static_cast<unsigned int>(i));
            }
        }

        if (selectCameras) {
            for (const auto& [id, img] : m_scene->images) {
                addImageToSelection(id);
            }
        }

        m_editorSystem->updateGizmoCenter();
    }

    void SelectionManager::resetState() {
        m_editorSystem->boxSelecting = false;
        m_editorSystem->pendingPickedID = false;
        selectedPointIndices.clear();
        selectedImageIDs.clear();
        changedIndices.clear();
    }

    void SelectionManager::processPickedID(const int pickedID, const bool isCtrlPressed) {
        if (!isCtrlPressed) clearSelection();

        if (pickedID < 0 || pickedID >= m_scene->points.size() || m_scene->points[pickedID].selected < -0.5f) {
            return;
        }

        if (isCtrlPressed && m_scene->points[pickedID].selected > 0.5f) {
            m_scene->points[pickedID].selected = 0.0f;
            removePointFromSelection(pickedID);
        } else if (m_scene->points[pickedID].selected < 0.5f) {
            m_scene->points[pickedID].selected = 1.0f;
            addPointToSelection(pickedID);
        }
        markAsChanged(pickedID);
        m_editorSystem->updateGizmoCenter();
    }

    void SelectionManager::processBoxSelection(const glm::mat4& vpMatrix, const ViewportInfo& vpInfo,
                                               const glm::vec2& boxStart, const glm::vec2& boxEnd,
                                               const bool isCtrlPressed,
                                               const bool allowPointSelection, const bool allowCameraSelection) {
        if (!isCtrlPressed) clearSelection();

        const float ndcMinX = (std::min(boxStart.x, boxEnd.x) / vpInfo.size.x) * 2.0f - 1.0f;
        const float ndcMaxX = (std::max(boxStart.x, boxEnd.x) / vpInfo.size.x) * 2.0f - 1.0f;
        const float ndcMinY = 1.0f - (std::max(boxStart.y, boxEnd.y) / vpInfo.size.y) * 2.0f;
        const float ndcMaxY = 1.0f - (std::min(boxStart.y, boxEnd.y) / vpInfo.size.y) * 2.0f;

        const auto rowX = glm::vec4(vpMatrix[0][0], vpMatrix[1][0], vpMatrix[2][0], vpMatrix[3][0]);
        const auto rowY = glm::vec4(vpMatrix[0][1], vpMatrix[1][1], vpMatrix[2][1], vpMatrix[3][1]);
        const auto rowW = glm::vec4(vpMatrix[0][3], vpMatrix[1][3], vpMatrix[2][3], vpMatrix[3][3]);

        if (allowPointSelection) {
            for (unsigned int i = 0; i < m_scene->points.size(); ++i) {
                if (m_scene->points[i].selected < -0.5f) continue;

                const float w = glm::dot(rowW, glm::vec4(m_scene->points[i].position, 1.0f));
                if (w <= 0.0f) continue;

                const float x = glm::dot(rowX, glm::vec4(m_scene->points[i].position, 1.0f)) / w;
                const float y = glm::dot(rowY, glm::vec4(m_scene->points[i].position, 1.0f)) / w;

                if (x >= ndcMinX && x <= ndcMaxX && y >= ndcMinY && y <= ndcMaxY) {
                    if (isCtrlPressed && m_scene->points[i].selected > 0.5f) {
                        m_scene->points[i].selected = 0.0f;
                    } else if (m_scene->points[i].selected < 0.5f) {
                        m_scene->points[i].selected = 1.0f;
                    }
                    markAsChanged(i);
                }
            }


            selectedPointIndices.clear();
            for (unsigned int i = 0; i < m_scene->points.size(); ++i) {
                if (m_scene->points[i].selected > 0.5f) addPointToSelection(i);
            }
        }

        if (allowCameraSelection) {
            for (const auto& [id, cam] : m_scene->images) {
                const float w = glm::dot(rowW, glm::vec4(cam.position, 1.0f));
                if (w <= 0.0f) continue;

                const float x = glm::dot(rowX, glm::vec4(cam.position, 1.0f)) / w;
                const float y = glm::dot(rowY, glm::vec4(cam.position, 1.0f)) / w;

                if (x >= ndcMinX && x <= ndcMaxX && y >= ndcMinY && y <= ndcMaxY) {
                    if (isCtrlPressed) {
                        auto it = std::find(selectedImageIDs.begin(), selectedImageIDs.end(), id);
                        if (it != selectedImageIDs.end()) removeImageFromSelection(id);
                        else addImageToSelection(id);
                    } else {
                        addImageToSelection(id);
                    }
                }
            }
        }

        m_editorSystem->updateGizmoCenter();
    }

    void SelectionManager::addPointToSelection(const unsigned int idx) {
        if (std::find(selectedPointIndices.begin(), selectedPointIndices.end(), idx) == selectedPointIndices.end()) {
            selectedPointIndices.push_back(idx);
        }
    }

    void SelectionManager::removePointFromSelection(const unsigned int idx) {
        std::erase(selectedPointIndices, idx);
    }

    void SelectionManager::addImageToSelection(const uint32_t id) {
        if (std::find(selectedImageIDs.begin(), selectedImageIDs.end(), id) == selectedImageIDs.end()) {
            selectedImageIDs.push_back(id);
        }
    }

    void SelectionManager::removeImageFromSelection(const uint32_t id) {
        std::erase(selectedImageIDs, id);
    }

    void SelectionManager::markAsChanged(const unsigned int idx) {
        changedIndices.push_back(idx);
    }

    void SelectionManager::selectPointsByError(const double minError) {
        clearSelection();
        for (size_t i = 0; i < m_scene->points.size(); ++i) {
            if (m_scene->points[i].selected < -0.5f) continue;

            if (i < m_scene->metadata.size() && m_scene->metadata[i].error > minError) {
                m_scene->points[i].selected = 1.0f;
                addPointToSelection(static_cast<unsigned int>(i));
                markAsChanged(static_cast<unsigned int>(i));
            }
        }
        m_editorSystem->updateGizmoCenter();
    }

    void SelectionManager::selectPointsByTrackLength(const size_t maxTrackLength) {
        clearSelection();
        for (size_t i = 0; i < m_scene->points.size(); ++i) {
            if (m_scene->points[i].selected < -0.5f) continue;

            if (i < m_scene->metadata.size() && m_scene->metadata[i].observations.size() <= maxTrackLength) {
                m_scene->points[i].selected = 1.0f;
                addPointToSelection(static_cast<unsigned int>(i));
                markAsChanged(static_cast<unsigned int>(i));
            }
        }
        m_editorSystem->updateGizmoCenter();
    }
}
