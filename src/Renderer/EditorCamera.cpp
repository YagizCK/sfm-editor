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
		m_position = m_resetPosition;
		m_distance = m_resetDistance;
		lookAt(m_focalPoint);
		updateView();
		updateProjection();
		updateEulerAngles();

		Events::onKey.connect([this](const int key, const bool pressed) {
			if (key == SFM_KEY_F && pressed) {
				resetView();
			}
		});

		Events::onMouseScroll.connect([this](const float yOffset) {
			if (yOffset != 0.0f) {
				if (m_cameraStyle == CameraStyle::Free) {
					if (Input::isMouseButtonPressed(SFM_MOUSE_BUTTON_RIGHT)) {
						m_movementSpeed = std::max(m_movementSpeed * (1.0f + 0.1f * yOffset), m_minMovementSpeed);
					} else {
						m_position += getForwardVector() * (yOffset * m_scrollSensitivity);
					}
				} else if (m_cameraStyle == CameraStyle::Orbit) {
					m_distance -= yOffset * m_distance * 0.1f * m_scrollSensitivity;
					if (m_distance < 0.1f) {
						m_focalPoint += getForwardVector();
						m_distance = 0.1f;
					}
					updateView();
				}
			}
		});

		Events::onMouseMove.connect([this](const glm::vec2 delta, const glm::vec2 pos) {
			if (Input::isMouseButtonPressed(SFM_MOUSE_BUTTON_RIGHT)) {
				const float yawDelta = delta.x * m_mouseSensitivity;
				const float pitchDelta = delta.y * m_mouseSensitivity;

				const glm::quat qPitch = glm::angleAxis(pitchDelta * -1.0f, glm::vec3(1.0f, 0.0f, 0.0f));
				const glm::quat qYaw = glm::angleAxis(yawDelta * -1.0f, glm::vec3(0.0f, 1.0f, 0.0f));

				m_orientation = qYaw * m_orientation * qPitch;
				m_orientation = glm::normalize(m_orientation);

				updateView();
				updateEulerAngles();
			} else if (Input::isMouseButtonPressed(SFM_MOUSE_BUTTON_MIDDLE)) {
				const float speedMultiplier = (m_cameraStyle == CameraStyle::Orbit) ? m_distance : m_movementSpeed;
				const float panSpeed = speedMultiplier * 0.002f;

				const glm::vec3 right = getRightVector();
				const glm::vec3 up = getUpVector();

				const glm::vec3 translation = -(right * delta.x * panSpeed) + (up * delta.y * panSpeed);

				m_position += translation;
				m_focalPoint += translation;

				updateView();
			}
		});
	}

	void EditorCamera::onUpdate(const float dt, const bool allowInput) {
		if (!Input::isMouseButtonPressed(SFM_MOUSE_BUTTON_RIGHT)) {
			glfwSetInputMode(g_nativeWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		if (allowInput && Input::isMouseButtonPressed(SFM_MOUSE_BUTTON_RIGHT)) {
			glfwSetInputMode(g_nativeWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			if (m_cameraStyle == CameraStyle::Free) {
				float velocity = m_movementSpeed * dt;
				if (Input::isKeyPressed(SFM_KEY_LEFT_SHIFT)) velocity *= 3.0f;
				if (Input::isKeyPressed(SFM_KEY_LEFT_ALT)) velocity *= 0.1f;

				const glm::vec3 forward = getForwardVector();
				const glm::vec3 right = getRightVector();
				const glm::vec3 up = getUpVector();

				if (Input::isKeyPressed(SFM_KEY_W)) m_position += forward * velocity;
				if (Input::isKeyPressed(SFM_KEY_S)) m_position -= forward * velocity;
				if (Input::isKeyPressed(SFM_KEY_A)) m_position -= right * velocity;
				if (Input::isKeyPressed(SFM_KEY_D)) m_position += right * velocity;

				if (Input::isKeyPressed(SFM_KEY_E)) m_position += up * velocity;
				if (Input::isKeyPressed(SFM_KEY_Q)) m_position -= up * velocity;

				m_focalPoint = m_position + (forward * m_distance);
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
		return glm::rotate(m_orientation, glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 EditorCamera::getRightVector() const {
		return glm::rotate(m_orientation, glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 EditorCamera::getUpVector() const {
		return glm::rotate(m_orientation, glm::vec3(0.0f, 1.0f, 0.0f));
	}

	void EditorCamera::updateProjection() {
		if (m_projectionMode == ProjectionMode::Perspective) {
			m_projection = glm::perspective(glm::radians(m_FOV), m_aspectRatio, m_nearClip, m_farClip);
		} else {
			const float orthoLeft = -m_orthoSize * m_aspectRatio * 0.5f;
			const float orthoRight = m_orthoSize * m_aspectRatio * 0.5f;
			const float orthoBottom = -m_orthoSize * 0.5f;
			const float orthoTop = m_orthoSize * 0.5f;
			m_projection = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_nearClip, m_farClip);
		}
	}

	void EditorCamera::updateView() {
		m_orientation = glm::normalize(m_orientation);

		if (m_cameraStyle == CameraStyle::Orbit) {
			m_position = m_focalPoint - (getForwardVector() * m_distance);
		}

		const glm::mat4 translate = glm::translate(glm::mat4(1.0f), m_position);
		const glm::mat4 rotate = glm::toMat4(m_orientation);
		m_viewMatrix = glm::inverse(translate * rotate);
	}

	void EditorCamera::lookAt(const glm::vec3& target) {
		m_focalPoint = target;
		const glm::vec3 direction = glm::normalize(target - m_position);
		m_orientation = glm::quatLookAt(direction, glm::vec3(0.0f, 1.0f, 0.0f));
		updateEulerAngles();
	}

	void EditorCamera::resetView() {
		m_focalPoint = glm::vec3(0.0f);
		m_distance = m_resetDistance;
		m_position = m_focalPoint - (getForwardVector() * m_distance);
		lookAt(m_focalPoint);
		Logger::info("Camera Focused to Origin");
	}

	void EditorCamera::setRotationFromUI() {
		const auto eulerRadians = glm::vec3(
			glm::radians(m_pitch),
			glm::radians(m_yaw),
			glm::radians(m_roll)
		);
		m_orientation = glm::quat(eulerRadians);
		if (m_cameraStyle == CameraStyle::Orbit) {
			m_position = m_focalPoint - (getForwardVector() * m_distance);
		}
		updateView();
	}

	void EditorCamera::setOrientationFromUI(const glm::quat& newQuat) {
		m_orientation = glm::normalize(newQuat);
		updateEulerAngles();
		if (m_cameraStyle == CameraStyle::Orbit) {
			m_position = m_focalPoint - (getForwardVector() * m_distance);
		}
		updateView();
	}

	void EditorCamera::setCameraStyle(const CameraStyle style) {
		m_cameraStyle = style;
		if (m_cameraStyle == CameraStyle::Orbit) {
			m_distance = glm::distance(m_position, m_focalPoint);
		}
	}

	void EditorCamera::updateEulerAngles() {
		const glm::vec3 euler = glm::eulerAngles(m_orientation);
		m_pitch = glm::degrees(euler.x);
		m_yaw = glm::degrees(euler.y);
		m_roll = glm::degrees(euler.z);
	}
}
