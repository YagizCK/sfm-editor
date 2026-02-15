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

#include "Shader.h"

#include "Core/Logger.h"

#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>


namespace sfmeditor {
    Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
        const std::string fragmentCode = readFile(fragmentPath);
        const std::string vertexCode = readFile(vertexPath);

        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        const unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, nullptr);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        const unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, nullptr);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        m_rendererID = glCreateProgram();
        glAttachShader(m_rendererID, vertex);
        glAttachShader(m_rendererID, fragment);
        glLinkProgram(m_rendererID);
        checkCompileErrors(m_rendererID, "PROGRAM");

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    Shader::~Shader() {
        glDeleteProgram(m_rendererID);
    }

    void Shader::bind() const {
        glUseProgram(m_rendererID);
    }

    void Shader::unbind() const {
        glUseProgram(0);
    }

    void Shader::setBool(const std::string& name, bool value) const {
        glUniform1i(glGetUniformLocation(m_rendererID, name.c_str()), static_cast<int>(value));
    }

    void Shader::setInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(m_rendererID, name.c_str()), value);
    }

    void Shader::setFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(m_rendererID, name.c_str()), value);
    }

    void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
        glUniform2fv(glGetUniformLocation(m_rendererID, name.c_str()), 1, &value[0]);
    }

    void Shader::setVec2(const std::string& name, float x, float y) const {
        glUniform2f(glGetUniformLocation(m_rendererID, name.c_str()), x, y);
    }

    void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3fv(glGetUniformLocation(m_rendererID, name.c_str()), 1, &value[0]);
    }

    void Shader::setVec3(const std::string& name, float x, float y, float z) const {
        glUniform3f(glGetUniformLocation(m_rendererID, name.c_str()), x, y, z);
    }

    void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
        glUniform4fv(glGetUniformLocation(m_rendererID, name.c_str()), 1, &value[0]);
    }

    void Shader::setVec4(const std::string& name, float x, float y, float z, float w) const {
        glUniform4f(glGetUniformLocation(m_rendererID, name.c_str()), x, y, z, w);
    }

    void Shader::setMat2(const std::string& name, const glm::mat2& mat) const {
        glUniformMatrix2fv(glGetUniformLocation(m_rendererID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

    void Shader::setMat3(const std::string& name, const glm::mat3& mat) const {
        glUniformMatrix3fv(glGetUniformLocation(m_rendererID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

    void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(m_rendererID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

    std::string Shader::readFile(const std::string& filepath) {
        std::ifstream fileStream(filepath, std::ios::in);

        if (!fileStream.is_open()) {
            Logger::error("Cannot read file: " + filepath);
            return "";
        }

        std::stringstream sstr;
        sstr << fileStream.rdbuf();
        std::string content = sstr.str();
        fileStream.close();
        return content;
    }

    void Shader::checkCompileErrors(const unsigned int shader, const std::string& type) {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                Logger::error(type + "\n" + infoLog + "\n -- --------------------------------------------------- -- ");
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
                Logger::error(type + "\n" + infoLog + "\n -- --------------------------------------------------- -- ");
            }
        }
    }
}
