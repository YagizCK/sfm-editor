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

#include "ConsolePanel.h"

#include "Core/Logger.h"

#include <imgui.h>


namespace sfmeditor {
    void ConsolePanel::onRender() {
        ImGui::Begin("Logs");

        if (ImGui::Button("Clear")) {
            Logger::clear();
        }
        ImGui::SameLine();

        ImGui::Separator();

        ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        for (const auto& [level, message, timestamp] : Logger::getLogs()) {
            ImGui::TextDisabled("[%s]", timestamp.c_str());
            ImGui::SameLine();

            ImVec4 color;
            switch (level) {
            case LogLevel::Info: color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                break;
            case LogLevel::Warning: color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                break;
            case LogLevel::Error: color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                break;
            case LogLevel::Critical: color = ImVec4(1.0f, 0.0f, 1.0f, 1.0f);
                break;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(message.c_str());
            ImGui::PopStyleColor();
        }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }
}
