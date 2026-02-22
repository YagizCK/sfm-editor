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

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Core/Types.hpp"


namespace sfmeditor {
    enum class ProjectionMode {
        Perspective,
        Orthographic
    };

    enum class CameraStyle {
        Free,
        Orbit
    };

    class EditorCamera {
    public:
        EditorCamera();

        ProjectionMode projectionMode = ProjectionMode::Perspective;
        CameraStyle cameraStyle = CameraStyle::Free;

        glm::vec3 position = {0.0f, 0.0f, 0.0f};
        glm::quat orientation = {1.0f, 0.0f, 0.0f, 0.0f};
        float distance = 0.0f;

        glm::vec3 resetPosition = {5.0f, 5.0f, 5.0f};
        glm::vec3 focalPoint = {0.0f, 0.0f, 0.0f};
        float resetDistance = 10.0f;

        float FOV = 103.0f;
        float orthoSize = 170.0f;

        float pitch = 0.0f;
        float yaw = 0.0f;
        float roll = 0.0f;

        float mouseSensitivity = 0.005f;
        float scrollSensitivity = 2.0f;

        float movementSpeed = 5.0f;
        const float minMovementSpeed = 0.1f;

        void onUpdate(float dt, const ViewportInfo& viewportInfo);
        void onResize(float width, float height);

        const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
        const glm::mat4& getProjection() const { return m_projection; }
        glm::mat4 getViewProjection() const { return m_projection * m_viewMatrix; }

        glm::vec3 getForwardVector() const;
        glm::vec3 getRightVector() const;
        glm::vec3 getUpVector() const;

        void updateProjection();
        void updateView();

        void lookAt(const glm::vec3& target);

        void resetView();

        void setRotationFromUI();
        void setOrientationFromUI(const glm::quat& newQuat);

        void setCameraStyle(CameraStyle style);

        Ray castRay(float mouseX, float mouseY, float viewportWidth, float viewportHeight) const;

        void teleportTo(const glm::vec3& newPos, const glm::quat& newOr);

    private:
        void updateEulerAngles();

        float m_aspectRatio = 1.778f;
        float m_nearClip = 0.0001f;
        float m_farClip = 10000.0f;

        glm::mat4 m_viewMatrix = glm::mat4(1.0f);
        glm::mat4 m_projection = glm::mat4(1.0f);

        ViewportInfo m_viewportInfo;
    };
}
