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

#include "Renderer/EditorCamera.h"
#include "ActionHistory.h"
#include "SelectionManager.h"

#include <imgui.h>
#include <ImGuizmo.h>
#include <memory>


namespace sfmeditor {
    class EditorSystem {
    public:
        EditorSystem(EditorCamera* camera, SfMScene* scene);

        void onUpdate(const ViewportInfo& viewportInfo);

        void updateGizmoCenter();

        SelectionManager* getSelectionManager() const { return m_selectionManager.get(); }
        ActionHistory* getActionHistory() const { return m_actionHistory.get(); }

        int gizmoOperation = ImGuizmo::TRANSLATE;
        bool useSnap = false;
        float snapTranslation = 1.0f;
        float snapRotation = 15.0f;
        float snapScale = 0.5f;
        void getSnapValues(float* snapArray) const;

        glm::mat4 gizmoTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f));
        glm::mat4 gizmoLastTransform = glm::mat4(1.0f);

        glm::vec2 boxStart = {0.0f, 0.0f};
        glm::vec2 boxEnd = {0.0f, 0.0f};
        bool boxSelecting = false;
        bool pendingPickedID = false;

        uint32_t isolatedCameraID = 0;

    private:
        void setupInputCallbacks();

        EditorCamera* m_camera;
        ViewportInfo m_viewportInfo;
        SfMScene* m_scene;

        std::unique_ptr<SelectionManager> m_selectionManager;
        std::unique_ptr<ActionHistory> m_actionHistory;

        const float m_boxSelectSqThreshold = 100.0f;
        bool m_wasUsingGizmo = false;

        std::vector<PointState> m_dragStartStates;
        std::vector<std::pair<uint32_t, CameraPose>> m_dragStartCamStates;
    };
}
