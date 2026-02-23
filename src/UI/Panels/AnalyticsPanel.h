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

#include "UIPanel.h"
#include "Core/Types.hpp"
#include "Core/EditorSystem.h"

#include <vector>


namespace sfmeditor {
    struct ImageStatData {
        uint32_t imageID;
        std::string name;
        uint32_t cameraID;
        size_t featureCount;
    };

    class AnalyticsPanel : public UIPanel {
    public:
        AnalyticsPanel(SfMScene* scene, EditorSystem* editorSystem);
        ~AnalyticsPanel() override = default;

        void onRender() override;

        bool isOpen = false;

    private:
        static void drawHistogramWithTooltip(const char* label, const std::vector<float>& data,
                                             float maxAxisValue, const std::string& tooltipLabel,
                                             bool isFloatLabel, const std::string& minLabelText,
                                             const std::string& maxLabelText);

        void refreshData();
        void renderPointsTab();
        void renderImagesTab();

        SfMScene* m_scene;
        EditorSystem* m_editorSystem;

        bool m_needsRefresh = true;

        std::vector<float> m_errorHistogram;
        float m_maxError = 3.0f;
        float m_avgError = 0.0f;

        std::vector<float> m_trackHistogram;
        size_t m_maxTrackLength = 0;
        float m_avgTrackLength = 0.0f;

        std::vector<ImageStatData> m_imageStats;

        float m_errorThresholdFilter = 2.0f;
        int m_trackThresholdFilter = 2;
    };
}
