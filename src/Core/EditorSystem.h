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
#include "Renderer/LineRenderer.h"


namespace sfmeditor {
    class EditorSystem {
    public:
        EditorSystem(EditorCamera* camera, LineRenderer* lineRenderer, std::vector<Point>* points);

        void onUpdate(const ViewportInfo& viewportInfo);

        bool hasSelection() const;

        void clearSelection();

        void resetState();

        int gizmoOperation = ImGuizmo::TRANSLATE;

        glm::mat4 gizmoTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f));

        glm::vec3 gizmoStartPosition = {0.0f, 0.0f, 0.0f};

        std::vector<unsigned int> selectedPointIndices;

        glm::vec2 boxStart = {0.0f, 0.0f};
        glm::vec2 boxEnd = {0.0f, 0.0f};
        bool boxSelecting = false;

        std::vector<unsigned int> changedIndices;

        bool pendingSelection = false;
        bool pendingDeletion = false;

    private:
        bool projectToViewport(const glm::vec3& worldPos, glm::vec2& outScreenPos) const;

        int getClosestPointIdx(float maxDistanceSq);

        EditorCamera* m_camera;
        LineRenderer* m_lineRenderer;

        ViewportInfo m_viewportInfo;

        std::vector<Point>* m_points;
    };
}
