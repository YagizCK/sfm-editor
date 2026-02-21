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
#include <memory>
#include <string>
#include <unordered_map>


namespace sfmeditor {
    struct UITexture {
        uint32_t id = 0;
        int width = 0;
        int height = 0;
    };

    class PropertiesPanel : public UIPanel {
    public:
        PropertiesPanel(SceneProperties* sceneProperties, EditorCamera* camera, SfMScene* scene,
                        EditorSystem* editorSystem);
        ~PropertiesPanel() override = default;

        void onRender() override;

    private:
        UITexture getOrLoadImage(const std::string& filepath);

        SceneProperties* m_sceneProperties;
        EditorCamera* m_camera;
        SfMScene* m_scene;
        EditorSystem* m_editorSystem;

        std::unordered_map<std::string, UITexture> m_imageCache;
    };
}
