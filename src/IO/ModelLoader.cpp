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
    SfMScene ModelLoader::load(const std::string& filepath) {
        std::filesystem::path path = std::filesystem::weakly_canonical(filepath);
        const std::string ext = path.extension().string();

        Logger::info("Loading file: " + path.string());

        SfMScene scene;

        if (ext == ".bin") {
            scene = loadColmapBinary(path.string());
            loadColmapCameras(path.parent_path().string(), scene);
        } else if (ext == ".txt") scene = loadColmapText(path.string());
        else if (ext == ".ply") scene = loadPLY(path.string());
        else if (ext == ".obj") scene = loadOBJ(path.string());
        else if (ext == ".xyz") scene = loadXYZ(path.string());
        else Logger::error("Unsupported format: " + ext);

        std::filesystem::path currentDir = path.parent_path();
        bool foundImages = false;

        for (int i = 0; i < 4; ++i) {
            std::filesystem::path potentialImagesPath = currentDir / "images";
            if (std::filesystem::exists(potentialImagesPath) && std::filesystem::is_directory(potentialImagesPath)) {
                scene.imageBasePath = potentialImagesPath.string();
                foundImages = true;
                break;
            }

            if (currentDir.has_parent_path()) {
                currentDir = currentDir.parent_path();
            } else {
                break;
            }
        }

        if (!foundImages) {
            scene.imageBasePath = path.parent_path().string();
            Logger::warn("Could not find 'images' directory. Defaulting to: " + scene.imageBasePath);
        } else {
            Logger::info("Found images directory at: " + scene.imageBasePath);
        }

        return scene;
    }

    SfMScene ModelLoader::loadColmapBinary(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) return SfMScene{};

        uint64_t numPoints = 0;
        file.read(reinterpret_cast<char*>(&numPoints), sizeof(uint64_t));

        SfMScene scene;
        scene.points.reserve(numPoints);
        scene.metadata.reserve(numPoints);

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

            Point p;
            p.position = {static_cast<float>(xyz[0]), static_cast<float>(xyz[1]), static_cast<float>(xyz[2])};
            p.color = {rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f};
            scene.points.push_back(p);

            PointMetadata meta;
            meta.original_id = id;
            meta.error = error;
            meta.observations.reserve(trackLength);

            for (uint64_t j = 0; j < trackLength; ++j) {
                uint32_t image_id, point2D_idx;
                file.read(reinterpret_cast<char*>(&image_id), sizeof(uint32_t));
                file.read(reinterpret_cast<char*>(&point2D_idx), sizeof(uint32_t));
                meta.observations.push_back({image_id, point2D_idx});
            }
            scene.metadata.push_back(meta);
        }

        return scene;
    }

    SfMScene ModelLoader::loadColmapText(const std::string& filepath) {
        std::ifstream file(filepath);
        std::string line;

        std::getline(file, line);
        if (line.find("#") != std::string::npos && line.find("3D point") == std::string::npos) {
            return loadXYZ(filepath);
        }

        file.clear();
        file.seekg(0);

        SfMScene scene;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::stringstream ss(line);
            uint64_t id;
            double x, y, z;
            int r, g, b;
            double error;

            if (ss >> id >> x >> y >> z >> r >> g >> b >> error) {
                Point p;
                p.position = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
                p.color = {r / 255.0f, g / 255.0f, b / 255.0f};
                scene.points.push_back(p);

                PointMetadata meta;
                meta.original_id = id;
                meta.error = error;

                uint32_t img_id, pt2d_idx;
                while (ss >> img_id >> pt2d_idx) {
                    meta.observations.push_back({img_id, pt2d_idx});
                }
                scene.metadata.push_back(meta);
            }
        }
        return scene;
    }

    struct ColmapCameraDef {
        int model_id;
        uint64_t width;
        uint64_t height;
        std::vector<double> params;
    };

    void ModelLoader::loadColmapCameras(const std::string& directory, SfMScene& scene) {
        std::string camerasPath = (std::filesystem::path(directory) / "cameras.bin").string();
        std::ifstream camFile(camerasPath, std::ios::binary);

        std::unordered_map<uint32_t, ColmapCameraDef> cameraDefs;

        if (camFile) {
            uint64_t numCameras;
            camFile.read(reinterpret_cast<char*>(&numCameras), sizeof(uint64_t));

            for (uint64_t i = 0; i < numCameras; ++i) {
                uint32_t cam_id;
                ColmapCameraDef def;

                camFile.read(reinterpret_cast<char*>(&cam_id), sizeof(uint32_t));
                camFile.read(reinterpret_cast<char*>(&def.model_id), sizeof(int));
                camFile.read(reinterpret_cast<char*>(&def.width), sizeof(uint64_t));
                camFile.read(reinterpret_cast<char*>(&def.height), sizeof(uint64_t));

                size_t numParams = 0;
                switch (def.model_id) {
                case 0: numParams = 3;
                    break; // SIMPLE_PINHOLE: f, cx, cy
                case 1: numParams = 4;
                    break; // PINHOLE: fx, fy, cx, cy
                case 2: numParams = 4;
                    break; // SIMPLE_RADIAL: f, cx, cy, k
                case 3: numParams = 5;
                    break; // RADIAL: f, cx, cy, k1, k2
                case 4: numParams = 8;
                    break; // OPENCV: fx, fy, cx, cy, k1, k2, p1, p2
                case 5: numParams = 12;
                    break; // OPENCV_FISHEYE: fx, fy, cx, cy, k1, k2, k3, k4, k5, k6, k7, k8
                case 6: numParams = 5;
                    break; // FULL_OPENCV: fx, fy, cx, cy, k1, k2, p1, p2, k3, k4, k5, k6
                default:
                    Logger::warn("Unknown camera model ID: " + std::to_string(def.model_id));
                    numParams = 3; // Fallback
                }

                def.params.resize(numParams);
                camFile.read(reinterpret_cast<char*>(def.params.data()), numParams * sizeof(double));

                cameraDefs[cam_id] = def;
            }
        }

        std::string imagesPath = (std::filesystem::path(directory) / "images.bin").string();
        std::ifstream imgFile(imagesPath, std::ios::binary);

        if (!imgFile) {
            Logger::warn("images.bin not found in " + directory + ". Cameras will not be loaded.");
            return;
        }

        uint64_t numImages;
        imgFile.read(reinterpret_cast<char*>(&numImages), sizeof(uint64_t));

        for (uint64_t i = 0; i < numImages; ++i) {
            CameraPose cam;

            uint32_t image_id;
            double qw, qx, qy, qz, tx, ty, tz;
            uint32_t camera_id;

            imgFile.read(reinterpret_cast<char*>(&image_id), sizeof(uint32_t));
            imgFile.read(reinterpret_cast<char*>(&qw), sizeof(double));
            imgFile.read(reinterpret_cast<char*>(&qx), sizeof(double));
            imgFile.read(reinterpret_cast<char*>(&qy), sizeof(double));
            imgFile.read(reinterpret_cast<char*>(&qz), sizeof(double));
            imgFile.read(reinterpret_cast<char*>(&tx), sizeof(double));
            imgFile.read(reinterpret_cast<char*>(&ty), sizeof(double));
            imgFile.read(reinterpret_cast<char*>(&tz), sizeof(double));
            imgFile.read(reinterpret_cast<char*>(&camera_id), sizeof(uint32_t));

            std::string imageName;
            char c;
            while (imgFile.read(&c, 1) && c != '\0') {
                imageName += c;
            }

            uint64_t numPoints2D;
            imgFile.read(reinterpret_cast<char*>(&numPoints2D), sizeof(uint64_t));

            cam.features.resize(numPoints2D);
            for (uint64_t j = 0; j < numPoints2D; ++j) {
                double x, y;
                uint64_t point3D_id;
                imgFile.read(reinterpret_cast<char*>(&x), sizeof(double));
                imgFile.read(reinterpret_cast<char*>(&y), sizeof(double));
                imgFile.read(reinterpret_cast<char*>(&point3D_id), sizeof(uint64_t));

                cam.features[j].coordinates = glm::vec2(static_cast<float>(x), static_cast<float>(y));
            }

            glm::quat q(static_cast<float>(qw), static_cast<float>(qx), static_cast<float>(qy), static_cast<float>(qz));
            glm::vec3 t(static_cast<float>(tx), static_cast<float>(ty), static_cast<float>(tz));

            glm::mat3 R = glm::mat3_cast(q);
            glm::vec3 cameraCenter = -glm::transpose(R) * t;
            glm::quat cameraOrientation = glm::quat_cast(glm::transpose(R));

            cam.cameraID = camera_id;
            cam.imageName = imageName;
            cam.position = cameraCenter;
            cam.orientation = cameraOrientation;

            if (cameraDefs.contains(camera_id)) {
                const auto& def = cameraDefs[camera_id];
                cam.modelId = def.model_id;
                cam.width = def.width;
                cam.height = def.height;
                cam.extraParams = def.params;

                if (def.model_id == 0 || def.model_id == 2 || def.model_id == 3) {
                    // f, cx, cy
                    cam.focalLength = static_cast<float>(def.params[0]);
                    cam.focalLengthY = cam.focalLength;
                    cam.principalPointX = static_cast<float>(def.params[1]);
                    cam.principalPointY = static_cast<float>(def.params[2]);
                } else if (def.model_id == 1 || def.model_id == 4) {
                    // fx, fy, cx, cy
                    cam.focalLength = static_cast<float>(def.params[0]);
                    cam.focalLengthY = static_cast<float>(def.params[1]);
                    cam.principalPointX = static_cast<float>(def.params[2]);
                    cam.principalPointY = static_cast<float>(def.params[3]);
                }
            } else {
                cam.modelId = -1;
                cam.width = 1920;
                cam.height = 1080;
                cam.focalLength = 1000.0f;
                cam.focalLengthY = 1000.0f;
            }

            scene.cameras[image_id] = cam;
        }

        Logger::info(std::format("Successfully loaded {} camera poses.", scene.cameras.size()));
    }

    SfMScene ModelLoader::loadPLY(const std::string& filepath) {
        SfMScene scene;
        std::ifstream in(filepath);
        std::string line;
        bool headerEnded = false;
        int vertexCount = 0;

        while (std::getline(in, line)) {
            if (line.find("element vertex") != std::string::npos) {
                std::stringstream ss(line);
                std::string temp;
                ss >> temp >> temp >> vertexCount;
                scene.points.reserve(vertexCount);
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
                scene.points.push_back(p);
            }
        }
        return scene;
    }

    SfMScene ModelLoader::loadOBJ(const std::string& filepath) {
        SfMScene scene;
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
                scene.points.push_back(p);
            }
        }
        return scene;
    }

    SfMScene ModelLoader::loadXYZ(const std::string& filepath) {
        SfMScene scene;
        std::ifstream in(filepath);
        std::string line;

        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::stringstream ss(line);
            float r, g, b;
            Point p;
            if (ss >> p.position.x >> p.position.y >> p.position.z >> r >> g >> b) {
                p.color = {r / 255.0f, g / 255.0f, b / 255.0f};
                scene.points.push_back(p);
            }
        }
        return scene;
    }
}
