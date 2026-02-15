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

#include <string>
#include <vector>

namespace sfmeditor {
    class SceneExporter {
    public:
        static bool exportFile(const std::string& filepath, const std::vector<Point>& points);

    private:
        static bool exportCOLMAP(const std::string& filepath, const std::vector<Point>& points);
        static bool exportPLY(const std::string& filepath, const std::vector<Point>& points);
        static bool exportOBJ(const std::string& filepath, const std::vector<Point>& points);
        static bool exportXYZ(const std::string& filepath, const std::vector<Point>& points);
    };
}
