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

#include <functional>
#include <vector>
#include <glm/vec2.hpp>

namespace sfmeditor {
    template <typename... Args>
    class Signal {
    public:
        using Slot = std::function<void(Args...)>;

        void connect(const Slot& slot) {
            m_slots.push_back(slot);
        }

        void emit(Args... args) {
            for (const auto& slot : m_slots) {
                slot(args...);
            }
        }

    private:
        std::vector<Slot> m_slots;
    };

    class Events {
    public:
        inline static Signal<int, int> onWindowResize;

        inline static Signal<int, bool> onKey;

        inline static Signal<int, bool> onMouseButton;

        inline static Signal<glm::vec2, glm::vec2> onMouseMove;

        inline static Signal<float> onMouseScroll;

        inline static Signal<std::string> onFileDrop;
    };
}
