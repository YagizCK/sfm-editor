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

namespace sfmeditor {
    class ModelLoader {
    public:
        static SfMScene load(const std::string& filepath);

    private:
        static SfMScene loadColmapBinary(const std::string& filepath);
        static SfMScene loadColmapText(const std::string& filepath);

        static void loadColmapCameras(const std::string& directory, SfMScene& scene);

        static SfMScene loadPLY(const std::string& filepath);
        static SfMScene loadOBJ(const std::string& filepath);
        static SfMScene loadXYZ(const std::string& filepath);
    };
}
