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


namespace sfmeditor {
    class EditorSystem;

    class SelectionManager {
    public:
        SelectionManager(EditorSystem* editorSystem, SfMScene* scene);

        bool hasSelection() const;
        void clearSelection(bool modifyScenePoints = true);
        void selectAll();
        void resetState();

        void processPickedID(int pickedID, bool isCtrlPressed);
        void processBoxSelection(const glm::mat4& vpMatrix, const ViewportInfo& vpInfo, const glm::vec2& boxStart,
                                 const glm::vec2& boxEnd, bool isCtrlPressed);

        void addPointToSelection(unsigned int idx);
        void removePointFromSelection(unsigned int idx);
        void addCameraToSelection(uint32_t id);
        void removeCameraFromSelection(uint32_t id);
        void markAsChanged(unsigned int idx);

        std::vector<unsigned int> selectedPointIndices;
        std::vector<uint32_t> selectedCameraIDs;
        std::vector<unsigned int> changedIndices;

    private:
        EditorSystem* m_editorSystem;
        SfMScene* m_scene;
    };
}
