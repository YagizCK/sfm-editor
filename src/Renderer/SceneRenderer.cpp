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
        m_postProcessShader = std::make_unique<Shader>("assets/shaders/postprocess.vert",
                                                       "assets/shaders/postprocess.frag");
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

    void SceneRenderer::updateBuffers(std::vector<Point>& points, EditorSystem* editorSystem) {
        if (points.empty()) return;

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        const size_t totalPoints = points.size();
        const float threshold = totalPoints * m_thresholdFactor;

        auto& changed = editorSystem->getSelectionManager()->changedIndices;

        if (!changed.empty()) {
            if (changed.size() < threshold) {
                for (const unsigned int idx : changed) {
                    glBufferSubData(GL_ARRAY_BUFFER, idx * sizeof(Point), sizeof(Point), &points[idx]);
                }
            } else {
                glBufferSubData(GL_ARRAY_BUFFER, 0, totalPoints * sizeof(Point), points.data());
            }
            changed.clear();
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

    void SceneRenderer::initPostProcess() {
        constexpr float quadVertices[] = {
            -1.0f, 1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f,

            -1.0f, 1.0f, 0.0f, 1.0f,
            1.0f, -1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f
        };

        glGenVertexArrays(1, &m_ppVAO);
        glGenBuffers(1, &m_ppVBO);
        glBindVertexArray(m_ppVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_ppVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), static_cast<void*>(nullptr));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);
    }

    void SceneRenderer::renderPostProcess(const uint32_t inputTexture, const EditorCamera* camera,
                                          const ViewportInfo& vp) const {
        glDisable(GL_DEPTH_TEST);
        m_postProcessShader->bind();

        const int modelId = camera->lensModel;
        const glm::vec2 resolution = vp.size;
        const glm::vec2 principal = resolution * camera->principalPoint;

        const float focalY = (resolution.y * 0.5f) / std::tan(glm::radians(camera->FOV * 0.5f));
        const auto focal = glm::vec2(focalY, focalY);

        m_postProcessShader->setInt("u_ModelId", modelId);
        m_postProcessShader->setVec2("u_Focal", focal);
        m_postProcessShader->setVec2("u_Principal", principal);
        m_postProcessShader->setVec2("u_Resolution", resolution);
        m_postProcessShader->setInt("u_ScreenTexture", 0);

        m_postProcessShader->setFloatArray("u_DistParams", camera->distParams, 8);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);

        glBindVertexArray(m_ppVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        m_postProcessShader->unbind();
        glEnable(GL_DEPTH_TEST);
    }
}
