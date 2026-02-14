#include "rmt/ui/panels/ExportDialogPanel.h"

#include <cstring>
#include <string>

#include "imgui.h"

#include "rmt/io/mesh/export/MeshExporter.h"
#include "rmt/ui/console/Console.h"

namespace rmt {

void renderExportDialogPanel(std::vector<Shape>& shapes,
                             UiRuntimeState& uiState,
                             bool& showConsole,
                             int winWidth,
                             int winHeight) {
    if (!uiState.showExportDialog) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(winWidth * 0.5f, winHeight * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(520, 430), ImGuiCond_Appearing);
    ImGui::Begin("Export Settings", &uiState.showExportDialog, ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Export Mesh to OBJ Format");
    ImGui::Separator();

    ImGui::Text("File Path:");
    ImGui::InputText("##filepath", uiState.exportFilename, sizeof(uiState.exportFilename));
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Specify full path or just filename\nExample: /Users/yourname/Desktop/mesh.obj\nor just: my_mesh.obj");
    }

    ImGui::Text("Quick paths:");
    if (ImGui::Button("Desktop", ImVec2(70, 20))) {
        std::strcpy(uiState.exportFilename, "~/Desktop/exported_mesh.obj");
    }
    ImGui::SameLine();
    if (ImGui::Button("Documents", ImVec2(80, 20))) {
        std::strcpy(uiState.exportFilename, "~/Documents/exported_mesh.obj");
    }
    ImGui::SameLine();
    if (ImGui::Button("Current Dir", ImVec2(80, 20))) {
        std::strcpy(uiState.exportFilename, "exported_mesh.obj");
    }

    const std::string currentFilename(uiState.exportFilename);
    const bool hasObjExtension = currentFilename.length() >= 4 &&
                                 currentFilename.substr(currentFilename.length() - 4) == ".obj";
    if (!hasObjExtension && !currentFilename.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Note: .obj extension will be added automatically");
    }

    ImGui::Spacing();

    ImGui::Text("Resolution (higher = more detail, slower):");
    ImGui::SliderInt("##resolution", &uiState.exportResolution, 16, 512);
    ImGui::Text("Grid size: %dx%dx%d (%d voxels)",
                uiState.exportResolution,
                uiState.exportResolution,
                uiState.exportResolution,
                uiState.exportResolution * uiState.exportResolution * uiState.exportResolution);

    ImGui::Spacing();

    ImGui::Text("Bounding Box Size:");
    ImGui::SliderFloat("##boundingbox", &uiState.exportBoundingBox, 1.0f, 100.0f, "%.1f");
    ImGui::TextDisabled("Size of the export volume around your shapes");

    ImGui::Separator();

    ImGui::Text("Quality Presets:");
    ImGui::Columns(2, "presets", false);

    if (ImGui::Button("Draft (16x16x16)", ImVec2(140, 25))) {
        uiState.exportResolution = 16;
    }
    if (ImGui::Button("Low (32x32x32)", ImVec2(140, 25))) {
        uiState.exportResolution = 32;
    }

    ImGui::NextColumn();

    if (ImGui::Button("Medium (64x64x64)", ImVec2(140, 25))) {
        uiState.exportResolution = 64;
    }
    if (ImGui::Button("High (128x128x128)", ImVec2(140, 25))) {
        uiState.exportResolution = 128;
    }

    ImGui::Columns(1);

    if (ImGui::Button("Very High (256x256x256)", ImVec2(200, 25))) {
        uiState.exportResolution = 256;
    }
    ImGui::SameLine();
    if (ImGui::Button("Ultra (512x512x512)", ImVec2(200, 25))) {
        uiState.exportResolution = 512;
    }

    ImGui::Separator();

    const float estimatedTime = (uiState.exportResolution * uiState.exportResolution * uiState.exportResolution) / 500000.0f;
    const long long memoryUsage = static_cast<long long>(uiState.exportResolution) *
                                  uiState.exportResolution *
                                  uiState.exportResolution *
                                  4;

    if (uiState.exportResolution >= 256) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.0f, 1.0f), "WARNING: Very high resolution!");
        ImGui::Text("This may take several minutes and use significant memory.");
    } else if (uiState.exportResolution >= 128) {
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "Warning: High resolution may take time.");
    }

    ImGui::Text("Estimated time: %.1f seconds", estimatedTime);
    ImGui::Text("Memory usage: %.1f MB", memoryUsage / (1024.0f * 1024.0f));

    ImGui::Separator();

    const bool canExport = std::strlen(uiState.exportFilename) > 0;
    if (!canExport) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
    }

    if (ImGui::Button("Export", ImVec2(120, 35)) && canExport) {
        std::string finalFilename(uiState.exportFilename);
        if (finalFilename.length() < 4 || finalFilename.substr(finalFilename.length() - 4) != ".obj") {
            finalFilename += ".obj";
        }

        Console::getInstance().addLog("Starting export...", 0);
        Console::getInstance().addLog("File: " + finalFilename, 0);
        Console::getInstance().addLog("Resolution: " + std::to_string(uiState.exportResolution) + "x" +
                                      std::to_string(uiState.exportResolution) + "x" +
                                      std::to_string(uiState.exportResolution),
                                      0);
        Console::getInstance().addLog("Bounding box: " + std::to_string(uiState.exportBoundingBox), 0);

        showConsole = true;

        MeshExporter::setLogCallback([](const std::string& message, int level) {
            Console::getInstance().addLog(message, level);
        });

        const bool success = MeshExporter::exportToOBJ(
            shapes,
            finalFilename,
            uiState.exportResolution,
            uiState.exportBoundingBox);

        if (success) {
            Console::getInstance().addLog("Mesh exported successfully to: " + finalFilename, 0);
        } else {
            Console::getInstance().addLog("Failed to export mesh!", 2);
        }
        uiState.showExportDialog = false;
    }

    if (!canExport) {
        ImGui::PopStyleVar();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 35))) {
        uiState.showExportDialog = false;
    }

    if (!canExport) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Please enter a filename to export");
    }

    ImGui::End();
}

} // namespace rmt
