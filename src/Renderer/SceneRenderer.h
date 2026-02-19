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
#include "Shader.h"
#include "EditorCamera.h"
#include "Core/EditorSystem.h"

#include <vector>
#include <memory>


namespace sfmeditor {
    class SceneRenderer {
    public:
        SceneRenderer();
        ~SceneRenderer();
        SceneRenderer(const SceneRenderer&) = default;
        SceneRenderer& operator=(const SceneRenderer&) = default;
        SceneRenderer(SceneRenderer&&) = default;
        SceneRenderer& operator=(SceneRenderer&&) = default;

        void initBuffers(const std::vector<Point>& points);

        void updateBuffers(std::vector<Point>& points, EditorSystem* editorSystem) const;

        void render(const std::vector<Point>& points, const SceneProperties* props, const EditorCamera* camera) const;

    private:
        uint32_t m_VAO = 0, m_VBO = 0;
        std::unique_ptr<Shader> m_pointShader;
    };
}
