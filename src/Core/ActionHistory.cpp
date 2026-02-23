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

#include "ActionHistory.h"

#include "EditorSystem.h"
#include "SelectionManager.h"
#include "Logger.h"

#include <format>


namespace sfmeditor {
    ActionHistory::ActionHistory(EditorSystem* editorSystem, SelectionManager* selectionManager, SfMScene* scene)
        : m_editorSystem(editorSystem), m_selectionManager(selectionManager), m_scene(scene) {
    }

    void ActionHistory::recordTransformAction(const std::vector<PointState>& oldPoints,
                                              const std::vector<PointState>& newPoints,
                                              const std::vector<std::pair<uint32_t, CameraPose>>& oldImages,
                                              const std::vector<std::pair<uint32_t, CameraPose>>& newImages) {
        EditorAction action;
        action.type = ActionType::Transform;
        action.oldStates = oldPoints;
        action.newStates = newPoints;
        action.oldImages = oldImages;
        action.newImages = newImages;

        m_undoStack.push_back(action);
        m_redoStack.clear();
    }

    void ActionHistory::executeDelete() {
        EditorAction action;
        action.type = ActionType::Delete;

        for (const unsigned int idx : m_selectionManager->selectedPointIndices) {
            action.oldStates.push_back({idx, m_scene->points[idx].position, m_scene->points[idx].selected});
            m_scene->points[idx].selected = -1.0f;
            m_selectionManager->markAsChanged(idx);
            action.newStates.push_back({idx, m_scene->points[idx].position, -1.0f});
        }

        for (const uint32_t imageID : m_selectionManager->selectedImageIDs) {
            if (m_scene->images.contains(imageID)) {
                action.oldImages.push_back({imageID, m_scene->images.at(imageID)});
                m_scene->images.erase(imageID);
            }
        }

        m_undoStack.push_back(action);
        m_redoStack.clear();
        m_selectionManager->clearSelection(false);
        Logger::info(std::format("Deleted {} points and {} images.", action.oldStates.size(), action.oldImages.size()));
    }

    void ActionHistory::undo() {
        if (m_undoStack.empty()) return;

        EditorAction action = m_undoStack.back();
        m_undoStack.pop_back();

        for (const auto& state : action.oldStates) {
            m_scene->points[state.index].position = state.position;
            m_scene->points[state.index].selected = state.selected;
            m_selectionManager->markAsChanged(state.index);

            if (state.selected > 0.5f) m_selectionManager->addPointToSelection(state.index);
            else m_selectionManager->removePointFromSelection(state.index);
        }

        if (action.type == ActionType::Delete) {
            for (const auto& [imageID, oldImg] : action.oldImages) {
                m_scene->images[imageID] = oldImg;
                m_selectionManager->addImageToSelection(imageID);
            }
        } else {
            for (const auto& [imageID, oldImg] : action.oldImages) {
                m_scene->images[imageID] = oldImg;
            }
        }

        m_editorSystem->updateGizmoCenter();
        m_redoStack.push_back(action);
        Logger::info("Undo action performed.");
    }

    void ActionHistory::redo() {
        if (m_redoStack.empty()) return;

        const EditorAction action = m_redoStack.back();
        m_redoStack.pop_back();

        for (const auto& state : action.newStates) {
            m_scene->points[state.index].position = state.position;
            m_scene->points[state.index].selected = state.selected;
            m_selectionManager->markAsChanged(state.index);

            if (state.selected > 0.5f) m_selectionManager->addPointToSelection(state.index);
            else m_selectionManager->removePointFromSelection(state.index);
        }

        if (action.type == ActionType::Delete) {
            for (const auto& [imageID, oldImg] : action.oldImages) {
                m_scene->images.erase(imageID);
                m_selectionManager->removeImageFromSelection(imageID);
            }
        } else {
            for (const auto& [imageID, newImg] : action.newImages) {
                m_scene->images[imageID] = newImg;
            }
        }

        m_editorSystem->updateGizmoCenter();
        m_undoStack.push_back(action);
        Logger::info("Redo action performed.");
    }
}
