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

#include "AnalyticsPanel.h"

#include <imgui.h>
#include <algorithm>
#include <format>
#include <unordered_set>


namespace sfmeditor {
    AnalyticsPanel::AnalyticsPanel(SfMScene* scene, EditorSystem* editorSystem)
        : m_scene(scene), m_editorSystem(editorSystem) {
    }

    void AnalyticsPanel::drawHistogramWithTooltip(const char* label, const std::vector<float>& data,
                                                  const float maxAxisValue, const std::string& tooltipLabel,
                                                  const bool isFloatLabel, const std::string& minLabelText,
                                                  const std::string& maxLabelText) {
        const int histCount = static_cast<int>(data.size());
        const auto histSize = ImVec2(ImGui::GetContentRegionAvail().x, 100);

        ImGui::PlotHistogram(label, data.data(), histCount, 0, nullptr, 0.0f, FLT_MAX, histSize);

        if (ImGui::IsItemHovered()) {
            const ImVec2 mousePos = ImGui::GetIO().MousePos;
            const ImVec2 itemPos = ImGui::GetItemRectMin();
            const ImVec2 itemSize = ImGui::GetItemRectSize();

            const float t = std::clamp((mousePos.x - itemPos.x) / itemSize.x, 0.0f, 0.999f);
            const int hoveredBin = static_cast<int>(t * histCount);

            ImGui::BeginTooltip();
            if (isFloatLabel) {
                const float binWidth = maxAxisValue / static_cast<float>(histCount);
                const float valStart = hoveredBin * binWidth;
                const float valEnd = valStart + binWidth;
                ImGui::Text("%s: %.3f - %.3f", tooltipLabel.c_str(), valStart, valEnd);
            } else {
                ImGui::Text("%s: %d", tooltipLabel.c_str(), hoveredBin);
            }
            ImGui::Text("Point Count: %.0f", data[hoveredBin]);
            ImGui::EndTooltip();
        }

        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y - 4));
        ImGui::TextDisabled("%s", minLabelText.c_str());
        ImGui::SameLine(histSize.x - ImGui::CalcTextSize(maxLabelText.c_str()).x);
        ImGui::TextDisabled("%s", maxLabelText.c_str());
    }

    void AnalyticsPanel::refreshData() {
        m_errorHistogram.assign(50, 0.0f);
        m_trackHistogram.assign(20, 0.0f);
        m_imageStats.clear();
        m_maxError = 3.0f;
        m_avgError = 0.0f;
        m_maxTrackLength = 0;
        m_avgTrackLength = 0.0f;

        size_t validPoints = 0;

        for (size_t i = 0; i < m_scene->points.size(); ++i) {
            if (m_scene->points[i].selected < -0.5f || i >= m_scene->metadata.size()) continue;

            const auto& meta = m_scene->metadata[i];

            if (meta.error > m_maxError) m_maxError = static_cast<float>(meta.error);
            m_avgError += static_cast<float>(meta.error);

            m_maxTrackLength = std::max(meta.observations.size(), m_maxTrackLength);
            m_avgTrackLength += static_cast<float>(meta.observations.size());

            validPoints++;
        }

        if (validPoints > 0) {
            m_avgError /= static_cast<float>(validPoints);
            m_avgTrackLength /= static_cast<float>(validPoints);

            const int errorBinCount = static_cast<int>(m_errorHistogram.size());
            const int trackBinCount = static_cast<int>(m_trackHistogram.size());

            for (size_t i = 0; i < m_scene->points.size(); ++i) {
                if (m_scene->points[i].selected < -0.5f || i >= m_scene->metadata.size()) continue;

                const auto& meta = m_scene->metadata[i];

                int errorBin = static_cast<int>((meta.error / m_maxError) * (errorBinCount - 1));
                m_errorHistogram[std::clamp(errorBin, 0, errorBinCount - 1)] += 1.0f;

                int trackBin = static_cast<int>(meta.observations.size());
                m_trackHistogram[std::clamp(trackBin, 0, trackBinCount - 1)] += 1.0f;
            }
        }

        m_imageStats.reserve(m_scene->images.size());
        for (const auto& [id, img] : m_scene->images) {
            m_imageStats.push_back({id, img.imageName, img.cameraID, img.features.size()});
        }

        m_needsRefresh = false;
    }

    void AnalyticsPanel::onRender() {
        if (!isOpen) return;

        if (ImGui::Begin("Analytics & Filtering", &isOpen)) {
            if (ImGui::Button("Refresh Data", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                m_needsRefresh = true;
            }

            if (m_needsRefresh) refreshData();

            if (ImGui::BeginTabBar("AnalyticsTabs")) {
                if (ImGui::BeginTabItem("Points")) {
                    renderPointsTab();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Images")) {
                    renderImagesTab();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    void AnalyticsPanel::renderPointsTab() {
        ImGui::Dummy(ImVec2(0, 5));

        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Reprojection Error");
        ImGui::Text("Average Error: %.4f px | Max Error: %.4f px", m_avgError, m_maxError);

        const std::string maxErrLabel = std::format("{:.2f} px", m_maxError);
        drawHistogramWithTooltip("##ErrorHist", m_errorHistogram, m_maxError, "Error Range", true, "0.0 px",
                                 maxErrLabel);

        ImGui::Dummy(ImVec2(0, 5));
        ImGui::DragFloat("Error Threshold", &m_errorThresholdFilter, 0.05f, 0.05f, m_maxError);
        if (ImGui::Button("Select Points Above Error Threshold", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            m_editorSystem->getSelectionManager()->selectPointsByError(m_errorThresholdFilter);
        }

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 5));

        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Track Length (Observations)");
        ImGui::Text("Average Tracks: %.2f cameras | Max Tracks: %zu cameras", m_avgTrackLength, m_maxTrackLength);

        drawHistogramWithTooltip("##TrackHist", m_trackHistogram, static_cast<float>(m_maxTrackLength),
                                 "Cameras Observed", false, "0", "19+");

        ImGui::Dummy(ImVec2(0, 5));
        ImGui::DragInt("Track Threshold", &m_trackThresholdFilter, 1, 1, 10);
        if (ImGui::Button("Select Points Below Track Threshold", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            m_editorSystem->getSelectionManager()->selectPointsByTrackLength(m_trackThresholdFilter);
        }
    }

    void AnalyticsPanel::renderImagesTab() {
        ImGui::Dummy(ImVec2(0, 5));
        ImGui::Text("Total Images: %zu", m_imageStats.size());
        ImGui::Dummy(ImVec2(0, 5));

        static constexpr ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
            ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
            ImGuiTableFlags_BordersV | ImGuiTableFlags_ScrollY;

        if (ImGui::BeginTable("ImageStatsTable", 4, flags, ImVec2(0.0f, ImGui::GetContentRegionAvail().y))) {
            ImGui::TableSetupColumn("Image ID", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed,
                                    0.0f, 0);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.0f, 1);
            ImGui::TableSetupColumn("Sensor ID", ImGuiTableColumnFlags_WidthFixed, 0.0f, 2);
            ImGui::TableSetupColumn("Features", ImGuiTableColumnFlags_WidthFixed, 0.0f, 3);
            ImGui::TableHeadersRow();

            if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs()) {
                if (sorts_specs->SpecsDirty) {
                    const auto& spec = sorts_specs->Specs[0];
                    std::sort(m_imageStats.begin(), m_imageStats.end(),
                              [&](const ImageStatData& a, const ImageStatData& b) {
                                  bool res = false;
                                  switch (spec.ColumnUserID) {
                                  case 0: res = a.imageID < b.imageID;
                                      break;
                                  case 1: res = a.name < b.name;
                                      break;
                                  case 2: res = a.cameraID < b.cameraID;
                                      break;
                                  case 3: res = a.featureCount < b.featureCount;
                                      break;
                                  }
                                  return spec.SortDirection == ImGuiSortDirection_Ascending ? res : !res;
                              });
                    sorts_specs->SpecsDirty = false;
                }
            }

            const auto& selectedVec = m_editorSystem->getSelectionManager()->selectedImageIDs;
            std::unordered_set<uint32_t> selectedSet(selectedVec.begin(), selectedVec.end());

            for (const auto& stat : m_imageStats) {
                ImGui::TableNextRow();
                ImGui::PushID(stat.imageID);

                ImGui::TableSetColumnIndex(0);
                const bool isSelected = selectedSet.contains(stat.imageID);

                if (ImGui::Selectable(std::to_string(stat.imageID).c_str(), isSelected,
                                      ImGuiSelectableFlags_SpanAllColumns)) {
                    if (!ImGui::GetIO().KeyCtrl) {
                        m_editorSystem->getSelectionManager()->clearSelection(false);
                    }
                    if (isSelected) {
                        m_editorSystem->getSelectionManager()->removeImageFromSelection(stat.imageID);
                    } else {
                        m_editorSystem->getSelectionManager()->addImageToSelection(stat.imageID);
                    }
                    m_editorSystem->updateGizmoCenter();
                }

                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(stat.name.c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%u", stat.cameraID);

                ImGui::TableSetColumnIndex(3);
                auto color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                if (stat.featureCount < 100) color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                else if (stat.featureCount < 500) color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);

                ImGui::TextColored(color, "%zu", stat.featureCount);

                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
}
