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

#include "EditorCamera.h"

#include "Core/Input.h"
#include "UI/UIManager.h"
#include "Core/KeyCodes.hpp"
#include "Core/Application.h"
#include "Core/Events.hpp"
#include "Core/Logger.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <algorithm>


namespace sfmeditor {
    EditorCamera::EditorCamera() {
        position = resetPosition;
        distance = resetDistance;
        lookAt(focalPoint);
        updateView();
        updateProjection();
        updateEulerAngles();

        Events::onKey.connect([this](const int key, const int action) {
            if (!m_viewportInfo.focused) return;
            if (action != SFM_PRESS) return;

            if (key == SFM_KEY_F) {
                resetView();
            }
        });

        Events::onMouseScroll.connect([this](const float yOffset) {
            if (!m_viewportInfo.focused || !m_viewportInfo.hovered) return;

            if (yOffset != 0.0f) {
                if (cameraStyle == CameraStyle::Free) {
                    if (Input::isMouseButtonPressed(SFM_MOUSE_BUTTON_RIGHT)) {
                        movementSpeed = std::max(movementSpeed * (1.0f + 0.1f * yOffset), minMovementSpeed);
                    } else {
                        position += getForwardVector() * (yOffset * scrollSensitivity);
                    }
                } else if (cameraStyle == CameraStyle::Orbit) {
                    distance -= yOffset * distance * 0.1f * scrollSensitivity;
                    if (distance < 0.1f) {
                        focalPoint += getForwardVector();
                        distance = 0.1f;
                    }
                    updateView();
                }
            }
        });

        Events::onMouseMove.connect([this](const glm::vec2 delta, const glm::vec2 pos) {
            if (!m_viewportInfo.focused || !m_viewportInfo.hovered) return;

            if (Input::isMouseButtonPressed(SFM_MOUSE_BUTTON_RIGHT)) {
                const float yawDelta = delta.x * mouseSensitivity;
                const float pitchDelta = delta.y * mouseSensitivity;

                const glm::quat qPitch = glm::angleAxis(pitchDelta * -1.0f, glm::vec3(1.0f, 0.0f, 0.0f));
                const glm::quat qYaw = glm::angleAxis(yawDelta * -1.0f, glm::vec3(0.0f, 1.0f, 0.0f));

                orientation = qYaw * orientation * qPitch;
                orientation = glm::normalize(orientation);

                updateView();
                updateEulerAngles();
            } else if (Input::isMouseButtonPressed(SFM_MOUSE_BUTTON_MIDDLE)) {
                const float speedMultiplier = (cameraStyle == CameraStyle::Orbit) ? distance : movementSpeed;
                const float panSpeed = speedMultiplier * 0.002f;

                const glm::vec3 right = getRightVector();
                const glm::vec3 up = getUpVector();

                const glm::vec3 translation = -(right * delta.x * panSpeed) + (up * delta.y * panSpeed);

                position += translation;
                focalPoint += translation;

                updateView();
            }
        });
    }

    void EditorCamera::onUpdate(const float dt, const ViewportInfo& viewportInfo) {
        m_viewportInfo = viewportInfo;

        if (m_viewportInfo.focused && m_viewportInfo.hovered && Input::isMouseButtonPressed(SFM_MOUSE_BUTTON_RIGHT)) {
            if (glfwGetInputMode(g_nativeWindow, GLFW_CURSOR) != GLFW_CURSOR_DISABLED) {
                glfwSetInputMode(g_nativeWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }

            if (cameraStyle == CameraStyle::Free) {
                float velocity = movementSpeed * dt;
                if (Input::isKeyPressed(SFM_KEY_LEFT_SHIFT)) velocity *= 3.0f;
                if (Input::isKeyPressed(SFM_KEY_LEFT_ALT)) velocity *= 0.1f;

                const glm::vec3 forward = getForwardVector();
                const glm::vec3 right = getRightVector();
                const glm::vec3 up = getUpVector();

                if (Input::isKeyPressed(SFM_KEY_W)) position += forward * velocity;
                if (Input::isKeyPressed(SFM_KEY_S)) position -= forward * velocity;
                if (Input::isKeyPressed(SFM_KEY_A)) position -= right * velocity;
                if (Input::isKeyPressed(SFM_KEY_D)) position += right * velocity;

                if (Input::isKeyPressed(SFM_KEY_E)) position += up * velocity;
                if (Input::isKeyPressed(SFM_KEY_Q)) position -= up * velocity;

                focalPoint = position + (forward * distance);
            }
        } else {
            if (glfwGetInputMode(g_nativeWindow, GLFW_CURSOR) != GLFW_CURSOR_NORMAL) {
                glfwSetInputMode(g_nativeWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }

        updateView();
    }

    void EditorCamera::onResize(const float width, const float height) {
        if (height > 0) {
            m_aspectRatio = width / height;
            updateProjection();
        }
    }

    glm::vec3 EditorCamera::getForwardVector() const {
        return glm::rotate(orientation, glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 EditorCamera::getRightVector() const {
        return glm::rotate(orientation, glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 EditorCamera::getUpVector() const {
        return glm::rotate(orientation, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void EditorCamera::updateProjection() {
        if (projectionMode == ProjectionMode::Perspective) {
            m_projection = glm::perspective(glm::radians(FOV), m_aspectRatio, m_nearClip, m_farClip);
        } else {
            const float orthoLeft = -orthoSize * m_aspectRatio * 0.5f;
            const float orthoRight = orthoSize * m_aspectRatio * 0.5f;
            const float orthoBottom = -orthoSize * 0.5f;
            const float orthoTop = orthoSize * 0.5f;
            m_projection = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_nearClip, m_farClip);
        }
    }

    void EditorCamera::updateView() {
        orientation = glm::normalize(orientation);

        if (cameraStyle == CameraStyle::Orbit) {
            position = focalPoint - (getForwardVector() * distance);
        }

        const glm::mat4 translate = glm::translate(glm::mat4(1.0f), position);
        const glm::mat4 rotate = glm::toMat4(orientation);
        m_viewMatrix = glm::inverse(translate * rotate);
    }

    void EditorCamera::lookAt(const glm::vec3& target) {
        focalPoint = target;
        const glm::vec3 direction = glm::normalize(target - position);
        orientation = glm::quatLookAt(direction, glm::vec3(0.0f, 1.0f, 0.0f));
        updateEulerAngles();
    }

    void EditorCamera::resetView() {
        focalPoint = glm::vec3(0.0f);
        distance = resetDistance;
        position = focalPoint - (getForwardVector() * distance);
        lookAt(focalPoint);
        Logger::info("Camera Focused to Origin");
    }

    void EditorCamera::setRotationFromUI() {
        const auto eulerRadians = glm::vec3(
            glm::radians(pitch),
            glm::radians(yaw),
            glm::radians(roll)
        );
        orientation = glm::quat(eulerRadians);
        if (cameraStyle == CameraStyle::Orbit) {
            position = focalPoint - (getForwardVector() * distance);
        }
        updateView();
    }

    void EditorCamera::setOrientationFromUI(const glm::quat& newQuat) {
        orientation = glm::normalize(newQuat);
        updateEulerAngles();
        if (cameraStyle == CameraStyle::Orbit) {
            position = focalPoint - (getForwardVector() * distance);
        }
        updateView();
    }

    void EditorCamera::setCameraStyle(const CameraStyle style) {
        cameraStyle = style;
        if (cameraStyle == CameraStyle::Orbit) {
            distance = glm::distance(position, focalPoint);
        }
    }

    Ray EditorCamera::castRay(const float mouseX, const float mouseY, const float viewportWidth,
                              const float viewportHeight) const {
        const float x = (2.0f * mouseX) / viewportWidth - 1.0f;
        const float y = 1.0f - (2.0f * mouseY) / viewportHeight;

        const auto rayClip = glm::vec4(x, y, -1.0f, 1.0f);

        glm::vec4 rayEye = glm::inverse(m_projection) * rayClip;
        rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

        auto rayWorld = glm::vec3(glm::inverse(m_viewMatrix) * rayEye);
        rayWorld = glm::normalize(rayWorld);

        return Ray{position, rayWorld};
    }

    void EditorCamera::updateEulerAngles() {
        const glm::vec3 euler = glm::eulerAngles(orientation);
        pitch = glm::degrees(euler.x);
        yaw = glm::degrees(euler.y);
        roll = glm::degrees(euler.z);
    }
}
