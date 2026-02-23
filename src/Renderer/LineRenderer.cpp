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

#include "LineRenderer.h"

#include <glad/glad.h>


namespace sfmeditor {
    LineRenderer::LineRenderer() {
        m_shader = std::make_unique<Shader>("assets/shaders/line.vert", "assets/shaders/line.frag");

        glCreateVertexArrays(1, &m_VAO);
        glCreateBuffers(1, &m_VBO);

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        m_maxVertices = 10000;
        glBufferData(GL_ARRAY_BUFFER, m_maxVertices * sizeof(LineVertex), nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), static_cast<void*>(nullptr));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, color));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    LineRenderer::~LineRenderer() {
        if (m_VAO)
            glDeleteVertexArrays(1, &m_VAO);
        if (m_VBO)
            glDeleteBuffers(1, &m_VBO);
    }

    void LineRenderer::addLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color,
                               const float duration) {
        m_lines.push_back({start, end, color, duration});
    }

    void LineRenderer::addRay(const glm::vec3& origin, const glm::vec3& direction, const float length,
                              const glm::vec3& color, const float duration) {
        addLine(origin, origin + (direction * length), color, duration);
    }

    void LineRenderer::onUpdate(const float dt) {
        for (size_t i = 0; i < m_lines.size();) {
            if (m_lines[i].lifetime > 0.0f) {
                m_lines[i].lifetime -= dt;

                if (m_lines[i].lifetime <= 0.0f) {
                    if (i != m_lines.size() - 1) {
                        m_lines[i] = m_lines.back();
                    }
                    m_lines.pop_back();
                    continue;
                }
            }
            i++;
        }
    }

    void LineRenderer::clear() {
        m_lines.clear();
        m_renderBuffer.clear();
    }

    void LineRenderer::draw(const std::unique_ptr<EditorCamera>& camera) {
        if (m_lines.empty()) return;

        m_renderBuffer.clear();
        m_renderBuffer.reserve(m_lines.size() * 2);

        for (const auto& line : m_lines) {
            m_renderBuffer.push_back({line.start, line.color});
            m_renderBuffer.push_back({line.end, line.color});
        }

        const int vertexCount = static_cast<int>(m_renderBuffer.size());

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        if (vertexCount > m_maxVertices) {
            m_maxVertices = vertexCount + 5000;
            glBufferData(GL_ARRAY_BUFFER, m_maxVertices * sizeof(LineVertex), nullptr, GL_DYNAMIC_DRAW);
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(LineVertex), m_renderBuffer.data());

        m_shader->bind();
        m_shader->setMat4("u_ViewProjection", camera->getViewProjection());

        glBindVertexArray(m_VAO);

        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_renderBuffer.size()));
        glLineWidth(1.0f);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_shader->unbind();
    }
}
