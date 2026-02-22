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

#include "Panels/UIPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/PropertiesPanel.h"
#include "Panels/ConsolePanel.h"

#include <vector>
#include <memory>
#include <functional>


namespace sfmeditor {
    class Window;

    class UIManager {
    public:
        explicit UIManager(Window* window);
        ~UIManager();

        void initPanels(SceneProperties* sceneProperties, EditorCamera* camera, SfMScene* scene,
                        EditorSystem* editorSystem);

        void beginFrame();
        void endFrame() const;

        void renderMainMenuBar(const std::function<void()>& onImport, const std::function<void()>& onSave,
                               const std::function<void()>& onExit, const std::function<void()>& onUndo,
                               const std::function<void()>& onRedo);

        void renderPanels() const;

        ViewportPanel* getViewportPanel() const { return m_viewportPanel.get(); }

    private:
        void renderDockspace();

        Window* m_windowRef = nullptr;
        bool m_resetLayout = true;

        std::unique_ptr<ViewportPanel> m_viewportPanel;

        std::vector<std::unique_ptr<UIPanel>> m_otherPanels;
    };
}
