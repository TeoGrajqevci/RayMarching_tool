#include "rmt/ui/console/Console.h"

namespace rmt {

Console::Console() {
    startTime = ImGui::GetTime();
}

void Console::addLog(const std::string& message, int level) {
    LogEntry entry;
    entry.message = message;
    entry.timestamp = ImGui::GetTime() - startTime;
    entry.level = level;
    logs.push_back(entry);

    if (logs.size() > 1000) {
        logs.erase(logs.begin(), logs.begin() + 100);
    }
}

void Console::clear() {
    logs.clear();
    startTime = ImGui::GetTime();
}

ImVec4 Console::getLogColor(int level) {
    switch (level) {
        case 0: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        case 1: return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
        case 2: return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
        default: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void Console::render(bool* p_open) {
    if (!ImGui::Begin("Export Console", p_open, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginMenuBar()) {
        if (ImGui::Button("Clear")) {
            clear();
        }
        ImGui::Separator();
        ImGui::Checkbox("Auto-scroll", &autoScroll);
        ImGui::Separator();

        static bool showInfo = true;
        static bool showWarning = true;
        static bool showError = true;
        ImGui::Text("Show:");
        ImGui::SameLine();
        if (ImGui::Checkbox("Info", &showInfo)) {}
        ImGui::SameLine();
        if (ImGui::Checkbox("Warnings", &showWarning)) {}
        ImGui::SameLine();
        if (ImGui::Checkbox("Errors", &showError)) {}

        ImGui::EndMenuBar();
    }

    const float footer_height_to_reserve =
        ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false,
                          ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

        static bool showInfo = true;
        static bool showWarning = true;
        static bool showError = true;

        for (const auto& entry : logs) {
            if ((entry.level == 0 && !showInfo) || (entry.level == 1 && !showWarning) ||
                (entry.level == 2 && !showError)) {
                continue;
            }

            ImVec4 color = getLogColor(entry.level);
            ImGui::PushStyleColor(ImGuiCol_Text, color);

            const char* icon = entry.level == 0 ? "ℹ️" : (entry.level == 1 ? "⚠️" : "❌");
            ImGui::Text("[%.2fs] %s %s", entry.timestamp, icon, entry.message.c_str());

            ImGui::PopStyleColor();
        }

        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    ImGui::Separator();

    int infoCount = 0;
    int warningCount = 0;
    int errorCount = 0;
    for (const auto& entry : logs) {
        if (entry.level == 0) infoCount++;
        else if (entry.level == 1) warningCount++;
        else if (entry.level == 2) errorCount++;
    }

    ImGui::Text("Total: %d | Info: %d | Warnings: %d | Errors: %d",
                (int)logs.size(), infoCount, warningCount, errorCount);

    ImGui::End();
}

} // namespace rmt
