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

#include "Shader.h"
#include "Core/Types.hpp"
#include "EditorCamera.h"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace sfmeditor {
    class LineRenderer {
    public:
        LineRenderer();
        ~LineRenderer();
        LineRenderer(const LineRenderer&) = delete;
        LineRenderer& operator=(const LineRenderer&) = delete;
        LineRenderer(LineRenderer&&) = default;
        LineRenderer& operator=(LineRenderer&&) = default;

        void addLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, float duration = 0.0f);

        void addRay(const glm::vec3& origin, const glm::vec3& direction, float length, const glm::vec3& color,
                    float duration = 0.0f);

        void onUpdate(float dt);

        void draw(const std::unique_ptr<EditorCamera>& camera);

        void clear();

    private:
        uint32_t m_VAO = 0;
        uint32_t m_VBO = 0;

        std::vector<LineVertex> m_renderBuffer;

        std::vector<LineData> m_lines;

        std::unique_ptr<Shader> m_shader;
    };
}
