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
        bool isColmap = false;

        if (ext == ".bin") {
            scene = loadColmapBinary(path.string());
            loadColmapCameras(path.parent_path().string(), scene);
            isColmap = true;
        } else if (ext == ".txt") {
            scene = loadColmapText(path.string());
            loadColmapCamerasText(path.parent_path().string(), scene);
            isColmap = true;
        } else if (ext == ".ply") {
            scene = loadPLY(path.string());
        } else if (ext == ".obj") {
            scene = loadOBJ(path.string());
        } else if (ext == ".xyz") {
            scene = loadXYZ(path.string());
        } else {
            Logger::error("Unsupported format: " + ext);
        }

        if (isColmap) {
            std::filesystem::path currentDir = path.parent_path();
            bool foundImages = false;

            for (int i = 0; i < 4; ++i) {
                std::filesystem::path potentialImagesPath = currentDir / "images";
                if (std::filesystem::exists(potentialImagesPath) &&
                    std::filesystem::is_directory(potentialImagesPath)) {
                    scene.imageBasePath = potentialImagesPath.string();
                    foundImages = true;
                    break;
                }
                if (currentDir.has_parent_path()) {
                    currentDir = currentDir.parent_path();
                } else break;
            }

            if (!foundImages) {
                scene.imageBasePath = path.parent_path().string();
                Logger::warn("Could not find 'images' directory. Defaulting to: " + scene.imageBasePath);
            } else {
                Logger::info("Found images directory at: " + scene.imageBasePath);
            }
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

            scene.points.push_back({
                {static_cast<float>(xyz[0]), static_cast<float>(xyz[1]), static_cast<float>(xyz[2])},
                {rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f}, 0.0f
            });

            PointMetadata meta{id, error};
            meta.observations.reserve(trackLength);
            for (uint64_t j = 0; j < trackLength; ++j) {
                uint32_t image_id, point2D_idx;
                file.read(reinterpret_cast<char*>(&image_id), sizeof(uint32_t));
                file.read(reinterpret_cast<char*>(&point2D_idx), sizeof(uint32_t));
                meta.observations.push_back({image_id, point2D_idx});
            }
            scene.metadata.push_back(std::move(meta));
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
                scene.points.push_back({
                    {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)},
                    {r / 255.0f, g / 255.0f, b / 255.0f}, 0.0f
                });

                PointMetadata meta{id, error};
                uint32_t img_id, pt2d_idx;
                while (ss >> img_id >> pt2d_idx) {
                    meta.observations.push_back({img_id, pt2d_idx});
                }
                scene.metadata.push_back(std::move(meta));
            }
        }
        return scene;
    }

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
                    break;
                case 1:
                case 2: numParams = 4;
                    break;
                case 3:
                case 6: numParams = 5;
                    break;
                case 4: numParams = 8;
                    break;
                case 5: numParams = 12;
                    break;
                default: numParams = 3;
                }
                def.params.resize(numParams);
                camFile.read(reinterpret_cast<char*>(def.params.data()), numParams * sizeof(double));
                cameraDefs[cam_id] = def;
            }
        }

        std::string imagesPath = (std::filesystem::path(directory) / "images.bin").string();
        std::ifstream imgFile(imagesPath, std::ios::binary);
        if (!imgFile) return;

        uint64_t numImages;
        imgFile.read(reinterpret_cast<char*>(&numImages), sizeof(uint64_t));

        for (uint64_t i = 0; i < numImages; ++i) {
            CameraPose cam;
            uint32_t image_id, camera_id;
            double qw, qx, qy, qz, tx, ty, tz;

            imgFile.read(reinterpret_cast<char*>(&image_id), sizeof(uint32_t));
            imgFile.read(reinterpret_cast<char*>(&qw), sizeof(double));
            imgFile.read(reinterpret_cast<char*>(&qx), sizeof(double));
            imgFile.read(reinterpret_cast<char*>(&qy), sizeof(double));
            imgFile.read(reinterpret_cast<char*>(&qz), sizeof(double));
            imgFile.read(reinterpret_cast<char*>(&tx), sizeof(double));
            imgFile.read(reinterpret_cast<char*>(&ty), sizeof(double));
            imgFile.read(reinterpret_cast<char*>(&tz), sizeof(double));
            imgFile.read(reinterpret_cast<char*>(&camera_id), sizeof(uint32_t));

            char c;
            while (imgFile.read(&c, 1) && c != '\0') { cam.imageName += c; }

            uint64_t numPoints2D;
            imgFile.read(reinterpret_cast<char*>(&numPoints2D), sizeof(uint64_t));
            cam.features.resize(numPoints2D);

            for (uint64_t j = 0; j < numPoints2D; ++j) {
                double x, y;
                uint64_t point3D_id;
                imgFile.read(reinterpret_cast<char*>(&x), sizeof(double));
                imgFile.read(reinterpret_cast<char*>(&y), sizeof(double));
                imgFile.read(reinterpret_cast<char*>(&point3D_id), sizeof(uint64_t));
                cam.features[j] = {glm::vec2(static_cast<float>(x), static_cast<float>(y)), point3D_id};
            }

            cam.cameraID = camera_id;
            computeCameraExtrinsics(cam, qw, qx, qy, qz, tx, ty, tz);

            if (cameraDefs.contains(camera_id)) {
                applyCameraIntrinsics(cam, cameraDefs[camera_id]);
            } else {
                cam.modelId = -1;
                cam.width = 1920;
                cam.height = 1080;
                cam.focalLength = 1000.0f;
                cam.focalLengthY = 1000.0f;
            }

            scene.cameras[image_id] = std::move(cam);
        }
        Logger::info(std::format("Successfully loaded {} camera poses (Binary).", scene.cameras.size()));
    }

    void ModelLoader::loadColmapCamerasText(const std::string& directory, SfMScene& scene) {
        std::string camerasPath = (std::filesystem::path(directory) / "cameras.txt").string();
        std::ifstream camFile(camerasPath);
        std::unordered_map<uint32_t, ColmapCameraDef> cameraDefs;

        if (camFile) {
            std::string line;
            while (std::getline(camFile, line)) {
                if (line.empty() || line[0] == '#') continue;

                std::stringstream ss(line);
                uint32_t cam_id;
                std::string modelStr;
                uint64_t width, height;

                if (ss >> cam_id >> modelStr >> width >> height) {
                    ColmapCameraDef def{-1, width, height, {}};

                    if (modelStr == "SIMPLE_PINHOLE") def.model_id = 0;
                    else if (modelStr == "PINHOLE") def.model_id = 1;
                    else if (modelStr == "SIMPLE_RADIAL") def.model_id = 2;
                    else if (modelStr == "RADIAL") def.model_id = 3;
                    else if (modelStr == "OPENCV") def.model_id = 4;
                    else if (modelStr == "OPENCV_FISHEYE") def.model_id = 5;
                    else if (modelStr == "FULL_OPENCV") def.model_id = 6;

                    double param;
                    while (ss >> param) def.params.push_back(param);
                    cameraDefs[cam_id] = def;
                }
            }
        }

        std::string imagesPath = (std::filesystem::path(directory) / "images.txt").string();
        std::ifstream imgFile(imagesPath);
        if (!imgFile) return;

        std::string line1, line2;
        while (std::getline(imgFile, line1) && std::getline(imgFile, line2)) {
            if (line1.empty() || line1[0] == '#') {
                continue;
            }

            std::stringstream ss1(line1);
            CameraPose cam;
            uint32_t image_id, camera_id;
            double qw, qx, qy, qz, tx, ty, tz;

            if (ss1 >> image_id >> qw >> qx >> qy >> qz >> tx >> ty >> tz >> camera_id >> cam.imageName) {
                std::stringstream ss2(line2);
                double x, y;
                long long point3D_id_raw;

                while (ss2 >> x >> y >> point3D_id_raw) {
                    uint64_t p3d = (point3D_id_raw < 0)
                                       ? static_cast<uint64_t>(-1)
                                       : static_cast<uint64_t>(point3D_id_raw);
                    cam.features.push_back({glm::vec2(static_cast<float>(x), static_cast<float>(y)), p3d});
                }

                cam.cameraID = camera_id;
                computeCameraExtrinsics(cam, qw, qx, qy, qz, tx, ty, tz);

                if (cameraDefs.contains(camera_id)) {
                    applyCameraIntrinsics(cam, cameraDefs[camera_id]);
                } else {
                    cam.modelId = -1;
                    cam.width = 1920;
                    cam.height = 1080;
                    cam.focalLength = 1000.0f;
                    cam.focalLengthY = 1000.0f;
                }

                scene.cameras[image_id] = std::move(cam);
            }
        }
        Logger::info(std::format("Successfully loaded {} camera poses (Text).", scene.cameras.size()));
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

    void ModelLoader::computeCameraExtrinsics(CameraPose& cam, const double qw, const double qx, const double qy,
                                              const double qz, const double tx,
                                              const double ty, const double tz) {
        const glm::quat q(static_cast<float>(qw), static_cast<float>(qx), static_cast<float>(qy),
                          static_cast<float>(qz));
        const glm::vec3 t(static_cast<float>(tx), static_cast<float>(ty), static_cast<float>(tz));

        const glm::mat3 R = glm::mat3_cast(q);
        cam.position = -glm::transpose(R) * t;
        cam.orientation = glm::quat_cast(glm::transpose(R));
    }

    void ModelLoader::applyCameraIntrinsics(CameraPose& cam, const ColmapCameraDef& def) {
        cam.modelId = def.model_id;
        cam.width = def.width;
        cam.height = def.height;
        cam.extraParams = def.params;

        if (!def.params.empty()) {
            cam.focalLength = static_cast<float>(def.params[0]);
            if (def.model_id == 0 || def.model_id == 2 || def.model_id == 3) {
                // f, cx, cy
                cam.focalLengthY = cam.focalLength;
                cam.principalPointX = static_cast<float>(def.params[1]);
                cam.principalPointY = static_cast<float>(def.params[2]);
            } else if (def.model_id == 1 || def.model_id == 4) {
                // fx, fy, cx, cy
                cam.focalLengthY = static_cast<float>(def.params[1]);
                cam.principalPointX = static_cast<float>(def.params[2]);
                cam.principalPointY = static_cast<float>(def.params[3]);
            }
        }
    }
}
