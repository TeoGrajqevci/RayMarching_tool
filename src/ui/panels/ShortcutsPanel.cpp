#include "rmt/ui/panels/ShortcutsPanel.h"

#include "imgui.h"

namespace rmt {

void renderShortcutsPanel(const UiWorkspaceGeometry& geometry,
                          bool& showHelpPopup) {
    if (!showHelpPopup) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(geometry.rightX, geometry.rightShortcutsY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(geometry.rightPaneWidth, geometry.rightShortcutsH), ImGuiCond_Always);
    if (ImGui::Begin("Shortcuts", &showHelpPopup,
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Transform Operations:");
        ImGui::BulletText("Shift + D  Duplicate selected shapes");
        ImGui::BulletText("X  Delete selection or constrain to X axis");
        ImGui::BulletText("Curve edit: Tab toggles mode, Shift + A adds node, X deletes node");
        ImGui::BulletText("Curve edit: G moves selected node (X/Y/Z to constrain)");
        ImGui::BulletText("C  Focus camera on last selected shape");
        ImGui::BulletText("G / R / S  Translate / Rotate / Scale mode");
        ImGui::BulletText("Use gizmo handles directly in viewport");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Constraints:");
        ImGui::BulletText("X / Y / Z during transform constrains axis");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Interface:");
        ImGui::BulletText("H toggles interface visibility");
    #if defined(__APPLE__)
        ImGui::BulletText("Cmd + Z / Cmd + Shift + Z  Undo / Redo");
    #else
        ImGui::BulletText("Ctrl + Z / Ctrl + Shift + Z  Undo / Redo");
    #endif
        ImGui::BulletText("ESC cancels active transform");
        ImGui::BulletText("Shift + Click toggles multi-selection");
        ImGui::BulletText("Drop OBJ/FBX files into viewport to import as mesh SDF");
    }
    ImGui::End();
}

} // namespace rmt
