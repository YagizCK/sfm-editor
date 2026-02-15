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

#include "SceneGrid.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace sfmeditor {
    SceneGrid::SceneGrid() {
        m_shader = std::make_unique<Shader>("assets/shaders/grid.vert", "assets/shaders/grid.frag");

        constexpr float planeVertices[] = {
            -0.5f, 0.f, -0.5f,
            0.5f, 0.f, -0.5f,
            0.5f, 0.f, 0.5f,
            -0.5f, 0.f, 0.5f
        };
        const unsigned int indices[] = {0, 1, 2, 2, 3, 0};

        glCreateVertexArrays(1, &m_planeVAO);
        glCreateBuffers(1, &m_planeVBO);
        glCreateBuffers(1, &m_planeEBO);

        glBindVertexArray(m_planeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_planeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_planeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

        const float axisVerts[] = {
            0.f, 0.f, 0.f, m_axisLength, 0.f, 0.f, // X
            0.f, 0.f, 0.f, 0.f, m_axisLength, 0.f, // Y
            0.f, 0.f, 0.f, 0.f, 0.f, m_axisLength // Z
        };

        glCreateVertexArrays(1, &m_axesVAO);
        glCreateBuffers(1, &m_axesVBO);
        glBindVertexArray(m_axesVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_axesVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(axisVerts), axisVerts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glBindVertexArray(0);
    }

    SceneGrid::~SceneGrid() {
        glDeleteVertexArrays(1, &m_planeVAO);
        glDeleteBuffers(1, &m_planeVBO);
        glDeleteBuffers(1, &m_planeEBO);
        glDeleteVertexArrays(1, &m_axesVAO);
        glDeleteBuffers(1, &m_axesVBO);
    }

    void SceneGrid::draw(const std::unique_ptr<SceneProperties>& sceneProperties, const glm::mat4& view,
                         const glm::mat4& projection, const glm::vec3& cameraPos) const {
        if (!sceneProperties->showGrid && !sceneProperties->showAxes) return;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);

        m_shader->bind();
        m_shader->setMat4("u_View", view);
        m_shader->setMat4("u_Projection", projection);
        m_shader->setVec3("u_CameraPos", cameraPos);
        m_shader->setFloat("u_GridSize", m_gridSize);

        if (sceneProperties->showGrid) {
            m_shader->setInt("u_IsLine", 0);
            m_shader->setVec3("u_LineColor", m_gridColor);

            glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.f), {cameraPos.x, 0.f, cameraPos.z}),
                                         {m_gridSize, 1.f, m_gridSize});
            m_shader->setMat4("u_Model", model);

            glBindVertexArray(m_planeVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }

        if (sceneProperties->showAxes) {
            m_shader->setInt("u_IsLine", 1);
            m_shader->setMat4("u_Model", glm::mat4(1.f));
            glLineWidth(2.0f);

            glBindVertexArray(m_axesVAO);

            m_shader->setVec3("u_LineColor", m_xAxisColor);
            glDrawArrays(GL_LINES, 0, 2);

            m_shader->setVec3("u_LineColor", m_yAxisColor);
            glDrawArrays(GL_LINES, 2, 2);

            m_shader->setVec3("u_LineColor", m_zAxisColor);
            glDrawArrays(GL_LINES, 4, 2);
        }

        m_shader->unbind();
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
    }
}
