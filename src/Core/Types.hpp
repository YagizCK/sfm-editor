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

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

namespace sfmeditor {
    struct ViewportInfo {
        glm::vec2 size = {1.0f, 1.0f};
        glm::vec2 position = {0.0f, 0.0f};
        bool focused = false;
        bool hovered = false;
    };

    struct Point {
        glm::vec3 position;
        glm::vec3 color;
        float selected = 0.0f;
    };

    struct SceneProperties {
        glm::vec3 backgroundColor = glm::vec3(0.1f, 0.1f, 0.1f);
        bool showGrid = true;
        bool showAxes = true;
        float pointSize = 6.0f;
    };

    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;
    };

    struct LineVertex {
        glm::vec3 position;
        glm::vec3 color;
    };

    struct LineData {
        glm::vec3 start;
        glm::vec3 end;
        glm::vec3 color;
        float lifetime;
    };
}
