#include "rmt/ui/panels/ScenePanel.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>

#include "imgui.h"

namespace rmt {

void renderScenePanel(const UiWorkspaceGeometry& geometry,
                      std::vector<Shape>& shapes,
                      std::vector<int>& selectedShapes,
                      int& editingShapeIndex,
                      char (&renameBuffer)[128],
                      TransformationState& transformState) {
    ImGui::SetNextWindowPos(ImVec2(geometry.rightX, geometry.rightY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(geometry.rightPaneWidth, geometry.rightSceneH), ImGuiCond_Always);
    ImGui::Begin("Scene", nullptr,
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Shapes: %d", static_cast<int>(shapes.size()));
    ImGui::Separator();

    for (std::size_t i = 0; i < shapes.size(); ++i) {
        if (i > 0) {
            ImGui::Separator();
        }

        ImVec4 iconColor;
        switch (shapes[i].type) {
            case SHAPE_SPHERE: iconColor = ImVec4(0.2f, 0.6f, 0.9f, 1.0f); break;
            case SHAPE_BOX: iconColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f); break;
            case SHAPE_TORUS: iconColor = ImVec4(0.9f, 0.9f, 0.3f, 1.0f); break;
            case SHAPE_CYLINDER: iconColor = ImVec4(0.7f, 0.3f, 0.9f, 1.0f); break;
            case SHAPE_CONE: iconColor = ImVec4(0.3f, 0.9f, 0.7f, 1.0f); break;
            case SHAPE_MANDELBULB: iconColor = ImVec4(0.95f, 0.55f, 0.2f, 1.0f); break;
            case SHAPE_MENGER_SPONGE: iconColor = ImVec4(0.65f, 0.85f, 1.0f, 1.0f); break;
            case SHAPE_MESH_SDF: iconColor = ImVec4(0.9f, 0.75f, 0.25f, 1.0f); break;
            default: iconColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
        ImGui::Bullet();
        ImGui::PopStyleColor();

        if (shapes[i].blendOp != BLEND_NONE) {
            ImGui::SameLine();
            ImVec4 blendColor;
            switch (shapes[i].blendOp) {
                case BLEND_SMOOTH_UNION: blendColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); break;
                case BLEND_SMOOTH_SUBTRACTION: blendColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break;
                case BLEND_SMOOTH_INTERSECTION: blendColor = ImVec4(0.0f, 0.0f, 1.0f, 1.0f); break;
                default: blendColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
            }
            ImGui::PushStyleColor(ImGuiCol_Text, blendColor);
            ImGui::Bullet();
            ImGui::PopStyleColor();
        }

        std::string shapeTypeName;
        switch (shapes[i].type) {
            case SHAPE_SPHERE: shapeTypeName = "Sphere "; break;
            case SHAPE_BOX: shapeTypeName = "Box "; break;
            case SHAPE_TORUS: shapeTypeName = "Torus "; break;
            case SHAPE_CYLINDER: shapeTypeName = "Cylinder "; break;
            case SHAPE_CONE: shapeTypeName = "Cone "; break;
            case SHAPE_MANDELBULB: shapeTypeName = "Mandelbulb "; break;
            case SHAPE_MENGER_SPONGE: shapeTypeName = "Menger Sponge "; break;
            case SHAPE_MESH_SDF: shapeTypeName = "Mesh SDF "; break;
            default: shapeTypeName = "Unknown "; break;
        }

        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (std::find(selectedShapes.begin(), selectedShapes.end(), static_cast<int>(i)) != selectedShapes.end()) {
            nodeFlags |= ImGuiTreeNodeFlags_Selected;
        }

        if (editingShapeIndex == static_cast<int>(i)) {
            ImGui::SameLine();
            char tempBuffer[128];
            std::strncpy(tempBuffer, renameBuffer, sizeof(tempBuffer));
            if (ImGui::InputText("##rename", tempBuffer, sizeof(tempBuffer),
                                 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
                shapes[i].name = tempBuffer;
                editingShapeIndex = -1;
            }
            if (!ImGui::IsItemActive() && (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1))) {
                editingShapeIndex = -1;
            }
            ImGui::TreeNodeEx((shapeTypeName + shapes[i].name).c_str(), nodeFlags);
        } else {
            ImGui::TreeNodeEx((shapeTypeName + shapes[i].name).c_str(), nodeFlags);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDoubleClicked(0)) {
                editingShapeIndex = static_cast<int>(i);
                std::strncpy(renameBuffer, shapes[i].name.c_str(), sizeof(renameBuffer));
                renameBuffer[sizeof(renameBuffer) - 1] = '\0';
            }
        }

        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("DND_SHAPE", &i, sizeof(std::size_t));
            ImGui::Text("Moving %s", (shapeTypeName + shapes[i].name).c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_SHAPE")) {
                const std::size_t srcIndex = *(const std::size_t*)payload->Data;
                if (srcIndex != i) {
                    Shape draggedShape = shapes[srcIndex];
                    shapes.erase(shapes.begin() + static_cast<std::ptrdiff_t>(srcIndex));
                    shapes.insert(shapes.begin() + static_cast<std::ptrdiff_t>(i), draggedShape);
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            editingShapeIndex = static_cast<int>(i);
            std::strncpy(renameBuffer, shapes[i].name.c_str(), sizeof(renameBuffer));
            renameBuffer[sizeof(renameBuffer) - 1] = '\0';
        }

        if (ImGui::IsItemClicked()) {
            if (ImGui::GetIO().KeyShift) {
                std::vector<int>::iterator it = std::find(selectedShapes.begin(), selectedShapes.end(), static_cast<int>(i));
                if (it != selectedShapes.end()) {
                    selectedShapes.erase(it);
                } else {
                    selectedShapes.push_back(static_cast<int>(i));
                }
            } else {
                selectedShapes.clear();
                selectedShapes.push_back(static_cast<int>(i));
            }
            clearMirrorHelperSelection(transformState);
            transformState.sunLightSelected = false;
            transformState.sunLightMoveModeActive = false;
            transformState.sunLightMoveConstrained = false;
            transformState.sunLightMoveAxis = -1;
            transformState.sunLightHandleDragActive = false;
            transformState.selectedPointLightIndex = -1;
            transformState.pointLightMoveModeActive = false;
            transformState.pointLightMoveConstrained = false;
            transformState.pointLightMoveAxis = -1;
        }
    }

    ImGui::End();
}

} // namespace rmt
