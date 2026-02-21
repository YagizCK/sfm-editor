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

#include "UIPanel.h"
#include "Core/Types.hpp"
#include "Core/EditorSystem.h"
#include "Renderer/EditorCamera.h"

#include <cstdint>


namespace sfmeditor {
    class ViewportPanel : public UIPanel {
    public:
        ViewportPanel(EditorCamera* camera, EditorSystem* editorSystem);
        ~ViewportPanel() override = default;

        void onRender() override;

        void setTextureID(const uint32_t id) { m_textureID = id; }
        ViewportInfo& getViewportInfo() { return m_viewportInfo; }

    private:
        void renderOverlayControls();

        EditorCamera* m_camera;
        EditorSystem* m_editorSystem;
        uint32_t m_textureID = 0;
        ViewportInfo m_viewportInfo;
    };
}
