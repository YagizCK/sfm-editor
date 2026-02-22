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

#pragma once

#include "Types.hpp"

#include <vector>
#include <utility>


namespace sfmeditor {
    class EditorSystem;

    enum class ActionType {
        Transform, Delete
    };

    struct PointState {
        unsigned int index;
        glm::vec3 position;
        float selected;
    };

    struct EditorAction {
        ActionType type;
        std::vector<PointState> oldStates;
        std::vector<PointState> newStates;

        std::vector<std::pair<uint32_t, CameraPose>> oldCamStates;
        std::vector<std::pair<uint32_t, CameraPose>> newCamStates;
    };

    class SelectionManager;

    class ActionHistory {
    public:
        ActionHistory(EditorSystem* editorSystem, SelectionManager* selectionManager, SfMScene* scene);

        void recordTransformAction(const std::vector<PointState>& oldPoints,
                                   const std::vector<PointState>& newPoints,
                                   const std::vector<std::pair<uint32_t, CameraPose>>& oldCams,
                                   const std::vector<std::pair<uint32_t, CameraPose>>& newCams);
        void executeDelete();
        void undo();
        void redo();

    private:
        EditorSystem* m_editorSystem;
        SelectionManager* m_selectionManager;
        SfMScene* m_scene;

        std::vector<EditorAction> m_undoStack;
        std::vector<EditorAction> m_redoStack;
    };
}
