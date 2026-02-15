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

#include "Core/Types.hpp"

#include <functional>
#include <cstdint>
#include <memory>
#include <glm/vec2.hpp>


namespace sfmeditor {
    class Window;
    class EditorCamera;

    class UIManager {
    public:
        explicit UIManager(Window* window);
        ~UIManager();
        UIManager(const UIManager&) = default;
        UIManager& operator=(const UIManager&) = default;
        UIManager(UIManager&&) = default;
        UIManager& operator=(UIManager&&) = default;

        void beginFrame();
        void endFrame() const;

        void renderMainMenuBar(const std::function<void()>& onImport, const std::function<void()>& onSave,
                               const std::function<void()>& onExit);

        void renderViewport(uint32_t textureID, glm::vec2& outSize, bool& outHovered, bool& outFocused,
                            const EditorCamera* camera);

        void renderInfoPanel(const std::unique_ptr<SceneProperties>& sceneProperties,
                             const std::unique_ptr<EditorCamera>& camera, int pointCount);

        void renderConsole();

    private:
        void renderDockspace();
        Window* m_windowRef = nullptr;
        bool m_resetLayout = true;
    };
}
