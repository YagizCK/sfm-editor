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

#include <glm/vec2.hpp>

struct GLFWwindow;

namespace sfmeditor {
	class Input {
	public:
		static void init();

		static bool isKeyPressed(int keycode);

		static bool isMouseButtonPressed(int button);

		static glm::vec2 getMousePosition();
		static float getMouseX();
		static float getMouseY();

		static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		static void cursorPosCallback(GLFWwindow* window, double xPos, double yPos);
		static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);

	private:
		static glm::vec2 m_lastMousePos;
	};
}
