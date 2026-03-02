#include "rmt/ui/panels/ViewportPanel.h"

#include <algorithm>

#include "imgui.h"

namespace rmt {

ImDrawList* renderViewportPanel(const UiWorkspaceGeometry& geometry,
                                ImVec2& viewportPos,
                                ImVec2& viewportSize,
                                ImVec2& helpButtonPos,
                                bool& viewportHovered) {
    viewportPos = ImVec2(geometry.centerX, geometry.centerY);
    viewportSize = ImVec2(geometry.centerW, geometry.centerH);
    helpButtonPos = viewportPos;
    viewportHovered = false;

    ImGui::SetNextWindowPos(ImVec2(geometry.centerX, geometry.centerY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(geometry.centerW, geometry.centerH), ImGuiCond_Always);

    ImDrawList* viewportDrawList = nullptr;
    ImGui::Begin("Viewport", nullptr,
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoScrollWithMouse |
                 ImGuiWindowFlags_NoBackground |
                 ImGuiWindowFlags_NoDecoration);
    {
        viewportHovered = ImGui::IsWindowHovered();
        viewportDrawList = ImGui::GetWindowDrawList();
        ImVec2 contentStart = ImGui::GetCursorScreenPos();
        ImVec2 contentSize = ImGui::GetContentRegionAvail();
        contentSize.x = std::max(contentSize.x, 1.0f);
        contentSize.y = std::max(contentSize.y, 1.0f);

        viewportPos = contentStart;
        viewportSize = contentSize;
        helpButtonPos = contentStart;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 viewportMax(contentStart.x + contentSize.x, contentStart.y + contentSize.y);
        drawList->AddRect(contentStart, viewportMax, IM_COL32(120, 120, 120, 180));
    }
    ImGui::End();

    return viewportDrawList;
}

} // namespace rmt
