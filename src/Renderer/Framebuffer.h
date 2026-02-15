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

#include <cstdint>

namespace sfmeditor {
    class Framebuffer {
    public:
        Framebuffer(uint32_t width, uint32_t height);
        ~Framebuffer();
        Framebuffer(const Framebuffer&) = default;
        Framebuffer& operator=(const Framebuffer&) = default;
        Framebuffer(Framebuffer&&) = default;
        Framebuffer& operator=(Framebuffer&&) = default;

        void bind() const;
        void unbind() const;
        void resize(uint32_t width, uint32_t height);

        uint32_t getTextureID() const { return m_colorAttachment; }

    private:
        uint32_t m_rendererID = 0;
        uint32_t m_colorAttachment = 0;
        uint32_t m_depthAttachment = 0;
        uint32_t m_width, m_height;
    };
}
