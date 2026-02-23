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
#include <unordered_set>
#include <unordered_map>
#include <limits>

namespace sfmeditor {
    bool SceneExporter::exportFile(const std::string& filepath, const SfMScene& scene) {
        const std::filesystem::path path(filepath);
        const std::string ext = path.extension().string();

        if (ext == ".bin") return exportCOLMAP(filepath, scene);
        if (ext == ".txt") return exportCOLMAPText(filepath, scene);
        if (ext == ".ply") return exportPLY(filepath, scene);
        if (ext == ".obj") return exportOBJ(filepath, scene);
        if (ext == ".xyz") return exportXYZ(filepath, scene);

        Logger::error("Unsupported export format: " + ext);
        return false;
    }

    bool SceneExporter::exportCOLMAP(const std::string& filepath, const SfMScene& scene) {
        std::filesystem::path fullPath(filepath);
        std::filesystem::path exportDir = fullPath.parent_path().empty() ? "." : fullPath.parent_path();
        constexpr uint64_t invalidPoint3DId = std::numeric_limits<uint64_t>::max();

        std::unordered_set<uint64_t> deletedPointIDs;
        uint64_t actualNumPoints;
        extractValidPoints(scene, deletedPointIDs, actualNumPoints);

        // 1. CAMERAS.BIN
        if (std::ofstream camFile(exportDir / "cameras.bin", std::ios::binary); camFile) {
            auto uniqueCameras = getUniqueCameras(scene);
            uint64_t numCameras = uniqueCameras.size();
            camFile.write(reinterpret_cast<const char*>(&numCameras), sizeof(uint64_t));

            for (const auto& [cam_id, camPtr] : uniqueCameras) {
                camFile.write(reinterpret_cast<const char*>(&cam_id), sizeof(uint32_t));
                camFile.write(reinterpret_cast<const char*>(&camPtr->modelId), sizeof(int));
                camFile.write(reinterpret_cast<const char*>(&camPtr->width), sizeof(uint64_t));
                camFile.write(reinterpret_cast<const char*>(&camPtr->height), sizeof(uint64_t));
                camFile.write(reinterpret_cast<const char*>(camPtr->extraParams.data()),
                              camPtr->extraParams.size() * sizeof(double));
            }
        } else return false;

        // 2. IMAGES.BIN
        if (std::ofstream imgFile(exportDir / "images.bin", std::ios::binary); imgFile) {
            uint64_t numImages = scene.cameras.size();
            imgFile.write(reinterpret_cast<const char*>(&numImages), sizeof(uint64_t));

            for (const auto& [image_id, cam] : scene.cameras) {
                imgFile.write(reinterpret_cast<const char*>(&image_id), sizeof(uint32_t));

                glm::quat q_colmap;
                glm::vec3 t_colmap;
                computeColmapExtrinsics(cam, q_colmap, t_colmap);

                double extrinsics[7] = {
                    q_colmap.w, q_colmap.x, q_colmap.y, q_colmap.z, t_colmap.x, t_colmap.y, t_colmap.z
                };
                imgFile.write(reinterpret_cast<const char*>(extrinsics), 7 * sizeof(double));

                imgFile.write(reinterpret_cast<const char*>(&cam.cameraID), sizeof(uint32_t));
                imgFile.write(cam.imageName.c_str(), cam.imageName.length() + 1);

                uint64_t numPoints2D = cam.features.size();
                imgFile.write(reinterpret_cast<const char*>(&numPoints2D), sizeof(uint64_t));

                for (const auto& feat : cam.features) {
                    double coords[2] = {feat.coordinates.x, feat.coordinates.y};
                    uint64_t p3d_id = deletedPointIDs.contains(feat.point3D_id) ? invalidPoint3DId : feat.point3D_id;

                    imgFile.write(reinterpret_cast<const char*>(coords), 2 * sizeof(double));
                    imgFile.write(reinterpret_cast<const char*>(&p3d_id), sizeof(uint64_t));
                }
            }
        } else return false;

        // 3. POINTS3D.BIN
        if (std::ofstream ptsFile(exportDir / "points3D.bin", std::ios::binary); ptsFile) {
            ptsFile.write(reinterpret_cast<const char*>(&actualNumPoints), sizeof(uint64_t));

            for (size_t i = 0; i < scene.points.size(); ++i) {
                const auto& p = scene.points[i];
                if (p.selected < -0.5f) continue;

                const bool hasMeta = (i < scene.metadata.size());
                uint64_t id = hasMeta ? scene.metadata[i].original_id : (i + 1);
                const double xyz[3] = {p.position.x, p.position.y, p.position.z};
                const uint8_t rgb[3] = {
                    static_cast<uint8_t>(p.color.r * 255), static_cast<uint8_t>(p.color.g * 255),
                    static_cast<uint8_t>(p.color.b * 255)
                };
                double error = hasMeta ? scene.metadata[i].error : 0.0;
                uint64_t trackLength = hasMeta ? scene.metadata[i].observations.size() : 0;

                ptsFile.write(reinterpret_cast<const char*>(&id), sizeof(uint64_t));
                ptsFile.write(reinterpret_cast<const char*>(xyz), 3 * sizeof(double));
                ptsFile.write(reinterpret_cast<const char*>(rgb), 3 * sizeof(uint8_t));
                ptsFile.write(reinterpret_cast<const char*>(&error), sizeof(double));
                ptsFile.write(reinterpret_cast<const char*>(&trackLength), sizeof(uint64_t));

                if (hasMeta) {
                    for (const auto& obs : scene.metadata[i].observations) {
                        ptsFile.write(reinterpret_cast<const char*>(&obs.image_id), sizeof(uint32_t));
                        ptsFile.write(reinterpret_cast<const char*>(&obs.point2D_idx), sizeof(uint32_t));
                    }
                }
            }
        } else return false;

        Logger::info("Full COLMAP Binary Data Exported to directory: " + exportDir.string());
        return true;
    }

    bool SceneExporter::exportCOLMAPText(const std::string& filepath, const SfMScene& scene) {
        std::filesystem::path fullPath(filepath);
        std::filesystem::path exportDir = fullPath.parent_path().empty() ? "." : fullPath.parent_path();
        constexpr uint64_t invalidPoint3DId = std::numeric_limits<uint64_t>::max();

        std::unordered_set<uint64_t> deletedPointIDs;
        uint64_t actualNumPoints;
        extractValidPoints(scene, deletedPointIDs, actualNumPoints);

        // 1. CAMERAS.TXT
        if (std::ofstream camFile(exportDir / "cameras.txt"); camFile) {
            auto uniqueCameras = getUniqueCameras(scene);
            camFile << "# Camera list with one line of data per camera:\n"
                << "#   CAMERA_ID, MODEL, WIDTH, HEIGHT, PARAMS[]\n"
                << "# Number of cameras: " << uniqueCameras.size() << "\n";

            const char* modelNames[] = {
                "SIMPLE_PINHOLE", "PINHOLE", "SIMPLE_RADIAL", "RADIAL", "OPENCV", "OPENCV_FISHEYE", "FULL_OPENCV"
            };
            camFile << std::fixed << std::setprecision(6);

            for (const auto& [cam_id, camPtr] : uniqueCameras) {
                std::string modelStr = (camPtr->modelId >= 0 && camPtr->modelId <= 6)
                                           ? modelNames[camPtr->modelId]
                                           : "UNKNOWN";
                camFile << cam_id << " " << modelStr << " " << camPtr->width << " " << camPtr->height;
                for (double param : camPtr->extraParams) camFile << " " << param;
                camFile << "\n";
            }
        } else return false;

        // 2. IMAGES.TXT
        if (std::ofstream imgFile(exportDir / "images.txt"); imgFile) {
            imgFile << "# Image list with two lines of data per image:\n"
                << "#   IMAGE_ID, QW, QX, QY, QZ, TX, TY, TZ, CAMERA_ID, NAME\n"
                << "#   POINTS2D[] as (X, Y, POINT3D_ID)\n"
                << "# Number of images: " << scene.cameras.size() << "\n";

            imgFile << std::fixed << std::setprecision(6);
            for (const auto& [image_id, cam] : scene.cameras) {
                glm::quat q;
                glm::vec3 t;
                computeColmapExtrinsics(cam, q, t);

                imgFile << image_id << " " << q.w << " " << q.x << " " << q.y << " " << q.z << " "
                    << t.x << " " << t.y << " " << t.z << " " << cam.cameraID << " " << cam.imageName << "\n";

                for (size_t i = 0; i < cam.features.size(); ++i) {
                    uint64_t p3d_id = deletedPointIDs.contains(cam.features[i].point3D_id)
                                          ? invalidPoint3DId
                                          : cam.features[i].point3D_id;

                    if (p3d_id == invalidPoint3DId)
                        imgFile << cam.features[i].coordinates.x << " " << cam.features[i].
                                                                           coordinates.y << " -1";
                    else
                        imgFile << cam.features[i].coordinates.x << " " << cam.features[i].coordinates.y << " " <<
                            p3d_id;

                    if (i != cam.features.size() - 1) imgFile << " ";
                }
                imgFile << "\n";
            }
        } else return false;

        // 3. POINTS3D.TXT
        if (std::ofstream ptsFile(exportDir / "points3D.txt"); ptsFile) {
            ptsFile << "# 3D point list with one line of data per point:\n"
                << "#   POINT3D_ID, X, Y, Z, R, G, B, ERROR, TRACK[] as (IMAGE_ID, POINT2D_IDX)\n"
                << "# Number of points: " << actualNumPoints << "\n";

            ptsFile << std::fixed << std::setprecision(6);
            for (size_t i = 0; i < scene.points.size(); ++i) {
                const auto& p = scene.points[i];
                if (p.selected < -0.5f) continue;

                const bool hasMeta = (i < scene.metadata.size());
                uint64_t id = hasMeta ? scene.metadata[i].original_id : (i + 1);
                double error = hasMeta ? scene.metadata[i].error : 0.0;

                ptsFile << id << " " << p.position.x << " " << p.position.y << " " << p.position.z << " "
                    << static_cast<int>(p.color.r * 255) << " " << static_cast<int>(p.color.g * 255) << " "
                    << static_cast<int>(p.color.b * 255) << " " << error;

                if (hasMeta) {
                    for (const auto& obs : scene.metadata[i].observations) {
                        ptsFile << " " << obs.image_id << " " << obs.point2D_idx;
                    }
                }
                ptsFile << "\n";
            }
        } else return false;

        Logger::info("Full COLMAP Text Data Exported to directory: " + exportDir.string());
        return true;
    }

    bool SceneExporter::exportPLY(const std::string& filepath, const SfMScene& scene) {
        std::ofstream out(filepath);
        if (!out) return false;

        size_t actualNumPoints = 0;
        for (const auto& p : scene.points) {
            if (p.selected >= -0.5f) actualNumPoints++;
        }

        out << "ply\nformat ascii 1.0\nelement vertex " << actualNumPoints << "\n"
            << "property float x\nproperty float y\nproperty float z\n"
            << "property uchar red\nproperty uchar green\nproperty uchar blue\nend_header\n";

        for (const auto& p : scene.points) {
            if (p.selected < -0.5f) continue;
            out << p.position.x << " " << p.position.y << " " << p.position.z << " "
                << static_cast<int>(p.color.r * 255) << " " << static_cast<int>(p.color.g * 255) << " "
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
                << static_cast<int>(p.color.r * 255) << " " << static_cast<int>(p.color.g * 255) << " "
                << static_cast<int>(p.color.b * 255) << "\n";
        }
        Logger::info("Exported XYZ: " + filepath);
        return true;
    }

    void SceneExporter::extractValidPoints(const SfMScene& scene, std::unordered_set<uint64_t>& outDeletedIDs,
                                           uint64_t& outActualNumPoints) {
        outActualNumPoints = 0;
        outDeletedIDs.clear();

        for (size_t i = 0; i < scene.points.size(); ++i) {
            const bool hasMeta = (i < scene.metadata.size());
            uint64_t id = hasMeta ? scene.metadata[i].original_id : (i + 1);

            if (scene.points[i].selected < -0.5f) {
                outDeletedIDs.insert(id);
            } else {
                outActualNumPoints++;
            }
        }
    }

    std::unordered_map<uint32_t, const CameraPose*> SceneExporter::getUniqueCameras(const SfMScene& scene) {
        std::unordered_map<uint32_t, const CameraPose*> uniqueCameras;
        for (const auto& [image_id, cam] : scene.cameras) {
            if (!uniqueCameras.contains(cam.cameraID)) {
                uniqueCameras[cam.cameraID] = &cam;
            }
        }
        return uniqueCameras;
    }

    void SceneExporter::computeColmapExtrinsics(const CameraPose& cam, glm::quat& outQ, glm::vec3& outT) {
        glm::mat3 R_editor = glm::mat3_cast(cam.orientation);
        glm::mat3 R_colmap = glm::transpose(R_editor);
        outQ = glm::quat_cast(R_colmap);
        outT = -R_colmap * cam.position;
    }
}
