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

#include "SceneExporter.h"

#include "Core/Logger.h"

#include <fstream>
#include <filesystem>

namespace sfmeditor {
    bool SceneExporter::exportFile(const std::string& filepath, const std::vector<Point>& points) {
        const std::filesystem::path path(filepath);
        const std::string ext = path.extension().string();

        if (ext == ".bin") return exportCOLMAP(filepath, points);
        if (ext == ".ply") return exportPLY(filepath, points);
        if (ext == ".obj") return exportOBJ(filepath, points);
        if (ext == ".xyz") return exportXYZ(filepath, points);
        if (ext == ".txt") return exportXYZ(filepath, points);

        Logger::error("Unsupported export format: " + ext);
        return false;
    }

    bool SceneExporter::exportCOLMAP(const std::string& filepath, const std::vector<Point>& points) {
        std::ofstream file(filepath, std::ios::binary);
        if (!file) return false;

        // Header: uint64 num_points
        const uint64_t numPoints = points.size();
        file.write(reinterpret_cast<const char*>(&numPoints), sizeof(uint64_t));

        uint64_t idCounter = 1;

        for (const auto& [position, color] : points) {
            uint64_t id = idCounter++;
            const double xyz[3] = {
                static_cast<double>(position.x), static_cast<double>(position.y), static_cast<double>(position.z)
            };
            const uint8_t rgb[3] = {
                static_cast<uint8_t>(color.r * 255), static_cast<uint8_t>(color.g * 255),
                static_cast<uint8_t>(color.b * 255)
            };
            double error = 0.0;
            uint64_t trackLength = 0;

            file.write(reinterpret_cast<const char*>(&id), sizeof(uint64_t));
            file.write(reinterpret_cast<const char*>(xyz), 3 * sizeof(double));
            file.write(reinterpret_cast<const char*>(rgb), 3 * sizeof(uint8_t));
            file.write(reinterpret_cast<const char*>(&error), sizeof(double));
            file.write(reinterpret_cast<const char*>(&trackLength), sizeof(uint64_t));
        }

        file.close();
        Logger::info("Exported COLMAP Binary: " + filepath);
        return true;
    }

    bool SceneExporter::exportPLY(const std::string& filepath, const std::vector<Point>& points) {
        std::ofstream out(filepath);
        if (!out) return false;

        out << "ply\n";
        out << "format ascii 1.0\n";
        out << "element vertex " << points.size() << "\n";
        out << "property float x\n";
        out << "property float y\n";
        out << "property float z\n";
        out << "property uchar red\n";
        out << "property uchar green\n";
        out << "property uchar blue\n";
        out << "end_header\n";

        for (const auto& [position, color] : points) {
            out << position.x << " " << position.y << " " << position.z << " "
                << static_cast<int>(color.r * 255) << " "
                << static_cast<int>(color.g * 255) << " "
                << static_cast<int>(color.b * 255) << "\n";
        }
        Logger::info("Exported PLY: " + filepath);
        return true;
    }

    bool SceneExporter::exportOBJ(const std::string& filepath, const std::vector<Point>& points) {
        std::ofstream out(filepath);
        if (!out) return false;
        out << "# SFM Editor Export\n";
        for (const auto& [position, color] : points) {
            out << "v " << position.x << " " << position.y << " " << position.z << " "
                << color.r << " " << color.g << " " << color.b << "\n";
        }
        Logger::info("Exported OBJ: " + filepath);
        return true;
    }

    bool SceneExporter::exportXYZ(const std::string& filepath, const std::vector<Point>& points) {
        std::ofstream out(filepath);
        if (!out) return false;
        for (const auto& [position, color] : points) {
            out << position.x << " " << position.y << " " << position.z << " "
                << static_cast<int>(color.r * 255) << " "
                << static_cast<int>(color.g * 255) << " "
                << static_cast<int>(color.b * 255) << "\n";
        }
        Logger::info("Exported XYZ: " + filepath);
        return true;
    }
}
