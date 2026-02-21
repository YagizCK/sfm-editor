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

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <string>
#include <vector>
#include <unordered_map>

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

    struct PointObservation {
        uint32_t image_id;
        uint32_t point2D_idx;
    };

    struct PointMetadata {
        uint64_t original_id;
        double error = 0.0;
        std::vector<PointObservation> observations;
    };

    struct Point2D {
        glm::vec2 coordinates;
    };

    struct CameraPose {
        uint32_t cameraID;
        std::string imageName;
        glm::vec3 position;
        glm::quat orientation;
        float focalLength;
        std::vector<Point2D> features;
    };

    struct SfMScene {
        std::string imageBasePath;
        std::vector<Point> points;
        std::vector<PointMetadata> metadata;
        std::unordered_map<uint32_t, CameraPose> cameras;
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
