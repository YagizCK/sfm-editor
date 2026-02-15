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

#include <glm/glm.hpp>
#include <memory>

namespace sfmeditor {
    class SceneGrid {
    public:
        SceneGrid();
        ~SceneGrid();
        SceneGrid(const SceneGrid&) = delete;
        SceneGrid& operator=(const SceneGrid&) = delete;
        SceneGrid(SceneGrid&&) = default;
        SceneGrid& operator=(SceneGrid&&) = default;

        void draw(const std::unique_ptr<SceneProperties>& sceneProperties, const glm::mat4& view,
                  const glm::mat4& projection, const glm::vec3& cameraPos) const;

    private:
        uint32_t m_planeVAO = 0, m_planeVBO = 0, m_planeEBO = 0;
        uint32_t m_axesVAO = 0, m_axesVBO = 0;
        std::unique_ptr<Shader> m_shader;

        float m_gridSize = 4000.0f;
        float m_axisLength = 4000.0f;

        glm::vec3 m_gridColor = glm::vec3(0.4f);
        glm::vec3 m_xAxisColor = glm::vec3(1.0f, 0.1f, 0.1f);
        glm::vec3 m_yAxisColor = glm::vec3(0.1f, 1.0f, 0.1f);
        glm::vec3 m_zAxisColor = glm::vec3(0.1f, 0.1f, 1.0f);
    };
}
