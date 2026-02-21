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

#include "SceneRenderer.h"

#include "Core/Logger.h"
#include "Core/Input.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <format>


namespace sfmeditor {
    SceneRenderer::SceneRenderer() {
        m_pointShader = std::make_unique<Shader>("assets/shaders/basic.vert", "assets/shaders/basic.frag");
        m_pickingShader = std::make_unique<Shader>("assets/shaders/picking.vert", "assets/shaders/picking.frag");
    }

    SceneRenderer::~SceneRenderer() {
        if (m_VAO)
            glDeleteVertexArrays(1, &m_VAO);
        if (m_VBO)
            glDeleteBuffers(1, &m_VBO);
    }

    void SceneRenderer::initBuffers(const std::vector<Point>& points) {
        if (m_VAO) {
            glDeleteVertexArrays(1, &m_VAO);
            m_VAO = 0;
        }
        if (m_VBO) {
            glDeleteBuffers(1, &m_VBO);
            m_VBO = 0;
        }

        glCreateVertexArrays(1, &m_VAO);
        glCreateBuffers(1, &m_VBO);

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(Point), points.data(), GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), nullptr);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point),
                              reinterpret_cast<const void*>(offsetof(Point, color)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Point),
                              reinterpret_cast<const void*>(offsetof(Point, selected)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void SceneRenderer::updateBuffers(std::vector<Point>& points, EditorSystem* editorSystem) const {
        if (points.empty()) return;

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        const size_t totalPoints = points.size();
        const float threshold = totalPoints * m_thresholdFactor;

        if (editorSystem->pendingSelection) {
            editorSystem->pendingSelection = false;
            auto& changed = editorSystem->changedIndices;
            if (!changed.empty()) {
                if (changed.size() < threshold) {
                    for (const unsigned int idx : changed) {
                        const size_t offset = (idx * sizeof(Point)) + offsetof(Point, selected);
                        glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(float), &points[idx].selected);
                    }
                } else {
                    glBufferSubData(GL_ARRAY_BUFFER, 0, totalPoints * sizeof(Point), points.data());
                }
                changed.clear();
            }
        }

        if (editorSystem->hasSelection()) {
            const auto& selected = editorSystem->selectedPointIndices;
            const size_t selectionCount = selected.size();

            const glm::mat4 currentTransform = editorSystem->gizmoTransform;
            const glm::mat4 lastTransform = editorSystem->gizmoLastTransform;

            bool isMatrixChanged = false;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    if (std::abs(currentTransform[i][j] - lastTransform[i][j]) > 1e-5f) {
                        isMatrixChanged = true;
                        break;
                    }
                }
                if (isMatrixChanged) break;
            }

            if (isMatrixChanged) {
                const glm::mat4 deltaTransform = currentTransform * glm::inverse(lastTransform);

                for (const unsigned int idx : selected) {
                    const auto pos = glm::vec4(points[idx].position, 1.0f);
                    points[idx].position = glm::vec3(deltaTransform * pos);
                }

                if (selectionCount < threshold) {
                    for (const unsigned int idx : selected) {
                        glBufferSubData(GL_ARRAY_BUFFER, idx * sizeof(Point), sizeof(glm::vec3), &points[idx].position);
                    }
                } else {
                    glBufferSubData(GL_ARRAY_BUFFER, 0, totalPoints * sizeof(Point), points.data());
                }

                editorSystem->gizmoLastTransform = currentTransform;
            }

            if (editorSystem->pendingDeletion) {
                editorSystem->pendingDeletion = false;
                std::erase_if(points, [](const Point& p) { return p.selected > 0.5f; });

                const size_t newSize = points.size();
                if (selectionCount < threshold) {
                    glBufferSubData(GL_ARRAY_BUFFER, 0, newSize * sizeof(Point), points.data());
                } else {
                    glBufferData(GL_ARRAY_BUFFER, newSize * sizeof(Point), points.data(), GL_DYNAMIC_DRAW);
                }
                editorSystem->selectedPointIndices.clear();
                editorSystem->changedIndices.clear();
                Logger::info(std::format("{} points deleted.", selectionCount));
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void SceneRenderer::render(const std::vector<Point>& points, const SceneProperties* props,
                               const EditorCamera* camera) const {
        if (points.empty()) return;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        m_pointShader->bind();
        m_pointShader->setFloat("u_PointSize", props->pointSize);
        m_pointShader->setMat4("u_ViewProjection", camera->getViewProjection());

        glBindVertexArray(m_VAO);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points.size()));

        m_pointShader->unbind();
        glDisable(GL_BLEND);
    }

    void SceneRenderer::renderPickingPass(const std::vector<Point>& points, const SceneProperties* props,
                                          const EditorCamera* camera) const {
        if (points.empty()) return;

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        m_pickingShader->bind();
        m_pickingShader->setFloat("u_PointSize", props->pointSize);
        m_pickingShader->setMat4("u_ViewProjection", camera->getViewProjection());

        glBindVertexArray(m_VAO);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points.size()));

        m_pickingShader->unbind();
    }

    int SceneRenderer::readPointID(const int mouseX, const int mouseY, const int vpHeight) {
        const int glY = vpHeight - mouseY;

        unsigned char pixel[4];

        glReadPixels(mouseX, glY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

        int id = pixel[0] | (pixel[1] << 8) | (pixel[2] << 16);

        id -= 1;

        return id;
    }
}
