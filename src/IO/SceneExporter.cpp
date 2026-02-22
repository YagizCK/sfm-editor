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
    bool SceneExporter::exportFile(const std::string& filepath, const SfMScene& scene) {
        const std::filesystem::path path(filepath);
        const std::string ext = path.extension().string();

        if (ext == ".bin") return exportCOLMAP(filepath, scene);
        if (ext == ".ply") return exportPLY(filepath, scene);
        if (ext == ".obj") return exportOBJ(filepath, scene);
        if (ext == ".xyz" || ext == ".txt") return exportXYZ(filepath, scene);

        Logger::error("Unsupported export format: " + ext);
        return false;
    }

    bool SceneExporter::exportCOLMAP(const std::string& filepath, const SfMScene& scene) {
        std::ofstream file(filepath, std::ios::binary);
        if (!file) return false;

        uint64_t actualNumPoints = 0;
        for (const auto& p : scene.points) {
            if (p.selected >= -0.5f) actualNumPoints++;
        }

        file.write(reinterpret_cast<const char*>(&actualNumPoints), sizeof(uint64_t));

        for (size_t i = 0; i < scene.points.size(); ++i) {
            const auto& p = scene.points[i];

            if (p.selected < -0.5f) continue;

            const bool hasMeta = (i < scene.metadata.size());

            uint64_t id = hasMeta ? scene.metadata[i].original_id : (i + 1);
            const double xyz[3] = {
                static_cast<double>(p.position.x), static_cast<double>(p.position.y), static_cast<double>(p.position.z)
            };
            const uint8_t rgb[3] = {
                static_cast<uint8_t>(p.color.r * 255), static_cast<uint8_t>(p.color.g * 255),
                static_cast<uint8_t>(p.color.b * 255)
            };
            double error = hasMeta ? scene.metadata[i].error : 0.0;
            uint64_t trackLength = hasMeta ? scene.metadata[i].observations.size() : 0;

            file.write(reinterpret_cast<const char*>(&id), sizeof(uint64_t));
            file.write(reinterpret_cast<const char*>(xyz), 3 * sizeof(double));
            file.write(reinterpret_cast<const char*>(rgb), 3 * sizeof(uint8_t));
            file.write(reinterpret_cast<const char*>(&error), sizeof(double));
            file.write(reinterpret_cast<const char*>(&trackLength), sizeof(uint64_t));

            if (hasMeta) {
                for (const auto& obs : scene.metadata[i].observations) {
                    file.write(reinterpret_cast<const char*>(&obs.image_id), sizeof(uint32_t));
                    file.write(reinterpret_cast<const char*>(&obs.point2D_idx), sizeof(uint32_t));
                }
            }
        }

        file.close();
        Logger::info("Exported COLMAP Binary: " + filepath);
        return true;
    }

    bool SceneExporter::exportPLY(const std::string& filepath, const SfMScene& scene) {
        std::ofstream out(filepath);
        if (!out) return false;

        size_t actualNumPoints = 0;
        for (const auto& p : scene.points) {
            if (p.selected >= -0.5f) actualNumPoints++;
        }

        out << "ply\n";
        out << "format ascii 1.0\n";
        out << "element vertex " << actualNumPoints << "\n";
        out << "property float x\n";
        out << "property float y\n";
        out << "property float z\n";
        out << "property uchar red\n";
        out << "property uchar green\n";
        out << "property uchar blue\n";
        out << "end_header\n";

        for (const auto& p : scene.points) {
            if (p.selected < -0.5f) continue;

            out << p.position.x << " " << p.position.y << " " << p.position.z << " "
                << static_cast<int>(p.color.r * 255) << " "
                << static_cast<int>(p.color.g * 255) << " "
                << static_cast<int>(p.color.b * 255) << "\n";
        }
        Logger::info("Exported PLY: " + filepath);
        return true;
    }

    bool SceneExporter::exportOBJ(const std::string& filepath, const SfMScene& scene) {
        std::ofstream out(filepath);
        if (!out) return false;
        out << "# SFM Editor Export\n";
        for (const auto& p : scene.points) {
            if (p.selected < -0.5f) continue;

            out << "v " << p.position.x << " " << p.position.y << " " << p.position.z << " "
                << p.color.r << " " << p.color.g << " " << p.color.b << "\n";
        }
        Logger::info("Exported OBJ: " + filepath);
        return true;
    }

    bool SceneExporter::exportXYZ(const std::string& filepath, const SfMScene& scene) {
        std::ofstream out(filepath);
        if (!out) return false;
        for (const auto& p : scene.points) {
            if (p.selected < -0.5f) continue;

            out << p.position.x << " " << p.position.y << " " << p.position.z << " "
                << static_cast<int>(p.color.r * 255) << " "
                << static_cast<int>(p.color.g * 255) << " "
                << static_cast<int>(p.color.b * 255) << "\n";
        }
        Logger::info("Exported XYZ: " + filepath);
        return true;
    }
}
