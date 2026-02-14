#include "rmt/ui/panels/ToolbarPanel.h"

#include "imgui.h"

namespace rmt {

void renderToolbarPanel(std::vector<Shape>& shapes,
                        UiRuntimeState& uiState,
                        TransformationState& transformState,
                        bool& showGUI,
                        bool& altRenderMode,
                        bool& usePathTracer,
                        bool& showConsole,
                        bool& showHelpPopup,
                        int winWidth) {
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(winWidth), 46.0f), ImGuiCond_Always);
    ImGui::Begin("Workspace Tools", nullptr,
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoScrollbar);

    if (ImGui::Button("Export OBJ")) {
        if (!shapes.empty()) {
            uiState.showExportDialog = true;
        }
    }

    ImGui::SameLine();
    const char* globalModeLabel = (uiState.globalScaleMode == SCALE_MODE_ELONGATE)
        ? "Mode: Elongate"
        : "Mode: Deform";
    if (ImGui::Button(globalModeLabel)) {
        uiState.globalScaleMode = (uiState.globalScaleMode == SCALE_MODE_ELONGATE)
            ? SCALE_MODE_DEFORM
            : SCALE_MODE_ELONGATE;
        for (Shape& shape : shapes) {
            shape.scaleMode = uiState.globalScaleMode;
        }
    }

    ImGui::SameLine();
    const char* rendererLabel = usePathTracer ? "Renderer: Path Tracer" : "Renderer: Ray March";
    if (ImGui::Button(rendererLabel)) {
        usePathTracer = !usePathTracer;
    }

    ImGui::SameLine();
    ImGui::Checkbox("Hide Overlays", &altRenderMode);

    ImGui::SameLine();
    ImGui::Checkbox("Console", &showConsole);

    ImGui::SameLine();
    ImGui::Checkbox("Shortcuts", &showHelpPopup);

    ImGui::SameLine();
    ImGui::Checkbox("Gizmo", &uiState.showViewportGizmo);

    ImGui::SameLine();
    ImGui::TextUnformatted("Op:");

    ImGui::SameLine();
    const char* gizmoOps[] = { "Move", "Rotate", "Scale" };
    ImGui::SetNextItemWidth(85.0f);
    ImGui::Combo("##GizmoOperationTopBar", &uiState.gizmoOperation, gizmoOps, IM_ARRAYSIZE(gizmoOps));

    ImGui::SameLine();
    ImGui::TextUnformatted("Space:");

    ImGui::SameLine();
    const char* gizmoModes[] = { "World", "Local" };
    ImGui::SetNextItemWidth(80.0f);
    ImGui::Combo("##GizmoModeTopBar", &uiState.gizmoMode, gizmoModes, IM_ARRAYSIZE(gizmoModes));
    transformState.useLocalSpace = (uiState.gizmoMode == 1);

    ImGui::SameLine();
    if (ImGui::Button("Hide UI (H)")) {
        showGUI = false;
    }

    ImGui::End();
}

} // namespace rmt
