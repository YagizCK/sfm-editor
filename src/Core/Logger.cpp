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

#include "Logger.h"

#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace sfmeditor {
    std::deque<LogEntry> Logger::m_logs;

    const std::string kReset = "\033[0m";
    const std::string kRed = "\033[31m";
    const std::string kGreen = "\033[32m";
    const std::string kYellow = "\033[33m";
    const std::string kWhite = "\033[37m";
    const std::string kMagenta = "\033[35m";

    void Logger::init() {
    }

    void Logger::info(const std::string& message) {
        log(LogLevel::Info, message);
    }

    void Logger::warn(const std::string& message) {
        log(LogLevel::Warning, message);
    }

    void Logger::error(const std::string& message) {
        log(LogLevel::Error, message);
    }

    void Logger::critical(const std::string& message) {
        log(LogLevel::Critical, message);
    }

    void Logger::log(LogLevel level, const std::string& message) {
        std::string timeStr = currentDateTime();

        m_logs.emplace_back(level, message, timeStr);
        if (m_logs.size() > 100) m_logs.pop_front();

        if (level == LogLevel::Critical) std::cerr << kMagenta << "[CRITICAL] " << message << kReset << "\n";
    }

    std::string Logger::currentDateTime() {
        const auto now = std::chrono::system_clock::now();
        auto inTimeT = std::chrono::system_clock::to_time_t(now);

        std::stringstream ss;
        struct tm timeInfo;

#ifdef _WIN32
        localtime_s(&timeInfo, &inTimeT);
#else
        localtime_r(&inTimeT, &timeInfo);
#endif

        ss << std::put_time(&timeInfo, "%H:%M:%S");
        return ss.str();
    }
}
