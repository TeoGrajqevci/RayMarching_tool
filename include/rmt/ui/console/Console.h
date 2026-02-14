#pragma once

#include <string>
#include <vector>

#include "imgui.h"

namespace rmt {

class Console {
public:
    struct LogEntry {
        std::string message;
        float timestamp;
        int level;
    };

    static Console& getInstance() {
        static Console instance;
        return instance;
    }

    void addLog(const std::string& message, int level = 0);
    void clear();
    void render(bool* p_open);

private:
    std::vector<LogEntry> logs;
    bool autoScroll = true;
    float startTime;

    Console();
    ImVec4 getLogColor(int level);
};

} // namespace rmt
