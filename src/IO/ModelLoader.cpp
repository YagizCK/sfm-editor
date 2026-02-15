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

#include "ModelLoader.h"

#include "Core/Logger.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

namespace sfmeditor {
    std::vector<Point> ModelLoader::load(const std::string& filepath) {
        const std::filesystem::path path(filepath);
        const std::string ext = path.extension().string();

        Logger::info("Loading file: " + filepath);

        if (ext == ".bin") return loadColmapBinary(filepath);
        if (ext == ".txt") return loadColmapText(filepath);
        if (ext == ".ply") return loadPLY(filepath);
        if (ext == ".obj") return loadOBJ(filepath);
        if (ext == ".xyz") return loadXYZ(filepath);

        Logger::error("Unsupported format: " + ext);
        return {};
    }

    std::vector<Point> ModelLoader::loadColmapBinary(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) return {};

        // uint64 num_points
        // loop:
        //   uint64 point3D_id
        //   double x, y, z
        //   uint8 r, g, b
        //   double error
        //   uint64 track_length
        //   loop(track_length): uint32 image_id, uint32 point2D_idx

        uint64_t numPoints = 0;
        file.read(reinterpret_cast<char*>(&numPoints), sizeof(uint64_t));

        std::vector<Point> points;
        points.reserve(numPoints);

        for (uint64_t i = 0; i < numPoints; ++i) {
            uint64_t id;
            double xyz[3];
            uint8_t rgb[3];
            double error;
            uint64_t trackLength;

            file.read(reinterpret_cast<char*>(&id), sizeof(uint64_t));
            file.read(reinterpret_cast<char*>(xyz), 3 * sizeof(double));
            file.read(reinterpret_cast<char*>(rgb), 3 * sizeof(uint8_t));
            file.read(reinterpret_cast<char*>(&error), sizeof(double));
            file.read(reinterpret_cast<char*>(&trackLength), sizeof(uint64_t));

            // image_id (4 byte) + point2D_idx (4 byte) = 8 byte
            file.seekg(trackLength * 8, std::ios::cur);

            Point p;
            p.position = {static_cast<float>(xyz[0]), static_cast<float>(xyz[1]), static_cast<float>(xyz[2])};
            p.color = {rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f};
            points.push_back(p);
        }

        return points;
    }

    std::vector<Point> ModelLoader::loadColmapText(const std::string& filepath) {
        std::ifstream file(filepath);
        std::string line;

        std::getline(file, line);
        if (line.find("#") != std::string::npos && line.find("3D point") == std::string::npos) {
            return loadXYZ(filepath);
        }

        file.clear();
        file.seekg(0);

        std::vector<Point> points;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::stringstream ss(line);
            uint64_t id;
            double x, y, z;
            int r, g, b;
            double error;
            // COLMAP TXT: ID X Y Z R G B ERROR TRACK[]
            if (ss >> id >> x >> y >> z >> r >> g >> b >> error) {
                Point p;
                p.position = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
                p.color = {r / 255.0f, g / 255.0f, b / 255.0f};
                points.push_back(p);
            }
        }
        return points;
    }

    std::vector<Point> ModelLoader::loadCOLMAP(const std::string& filepath) {
        return {};
    }

    std::vector<Point> ModelLoader::loadPLY(const std::string& filepath) {
        std::vector<Point> points;
        std::ifstream in(filepath);
        std::string line;
        bool headerEnded = false;
        int vertexCount = 0;

        while (std::getline(in, line)) {
            if (line.find("element vertex") != std::string::npos) {
                std::stringstream ss(line);
                std::string temp;
                ss >> temp >> temp >> vertexCount;
                points.reserve(vertexCount);
            }
            if (line == "end_header") {
                headerEnded = true;
                continue;
            }
            if (!headerEnded) continue;

            std::stringstream ss(line);
            float x, y, z, r, g, b;
            if (ss >> x >> y >> z >> r >> g >> b) {
                Point p;
                p.position = {x, y, z};
                if (r > 1.0f || g > 1.0f || b > 1.0f) {
                    p.color = {r / 255.0f, g / 255.0f, b / 255.0f};
                } else {
                    p.color = {r, g, b};
                }
                points.push_back(p);
            }
        }
        return points;
    }

    std::vector<Point> ModelLoader::loadOBJ(const std::string& filepath) {
        std::vector<Point> points;
        std::ifstream in(filepath);
        std::string line;

        while (std::getline(in, line)) {
            if (line.substr(0, 2) == "v ") {
                std::stringstream ss(line.substr(2));
                float x, y, z, r, g, b;
                ss >> x >> y >> z;

                Point p;
                p.position = {x, y, z};

                if (ss >> r >> g >> b) {
                    p.color = {r, g, b};
                } else {
                    p.color = {1.0f, 1.0f, 1.0f};
                }
                points.push_back(p);
            }
        }
        return points;
    }

    std::vector<Point> ModelLoader::loadXYZ(const std::string& filepath) {
        std::vector<Point> points;
        std::ifstream in(filepath);
        std::string line;

        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::stringstream ss(line);
            float r, g, b;
            Point p;
            if (ss >> p.position.x >> p.position.y >> p.position.z >> r >> g >> b) {
                p.color = {r / 255.0f, g / 255.0f, b / 255.0f};
                points.push_back(p);
            }
        }
        return points;
    }
}
