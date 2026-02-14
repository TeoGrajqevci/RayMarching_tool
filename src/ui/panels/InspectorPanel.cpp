#include "rmt/ui/panels/InspectorPanel.h"

#include <algorithm>

#include "imgui.h"

namespace rmt {

void renderInspectorPanel(const UiWorkspaceGeometry& geometry,
                          std::vector<Shape>& shapes,
                          std::vector<int>& selectedShapes,
                          TransformationState& transformState,
                          int globalScaleMode) {
    ImGui::SetNextWindowPos(ImVec2(geometry.leftX, geometry.leftY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(geometry.leftPaneWidth, geometry.leftInspectorH), ImGuiCond_Always);
    ImGui::Begin("Inspector", nullptr,
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse);

    if (!selectedShapes.empty()) {
        const int selIdx = selectedShapes[0];
        if (selIdx >= 0 && selIdx < static_cast<int>(shapes.size())) {
            Shape& selectedShape = shapes[selIdx];
            if (ImGui::BeginTabBar("ShapeParametersTabs", ImGuiTabBarFlags_None)) {
                if (ImGui::BeginTabItem("Transform")) {
                    ImGui::DragFloat3("Center", selectedShape.center, 0.01f);
                    switch (selectedShape.type) {
                        case SHAPE_SPHERE:
                            ImGui::DragFloat("Radius", &selectedShape.param[0], 0.01f);
                            selectedShape.param[0] = std::max(selectedShape.param[0], 0.001f);
                            break;
                        case SHAPE_BOX:
                            ImGui::DragFloat3("Half Extents", selectedShape.param, 0.01f);
                            selectedShape.param[0] = std::max(selectedShape.param[0], 0.001f);
                            selectedShape.param[1] = std::max(selectedShape.param[1], 0.001f);
                            selectedShape.param[2] = std::max(selectedShape.param[2], 0.001f);
                            break;
                        case SHAPE_TORUS:
                            ImGui::DragFloat("Major Radius", &selectedShape.param[0], 0.01f);
                            ImGui::DragFloat("Minor Radius", &selectedShape.param[1], 0.01f);
                            selectedShape.param[0] = std::max(selectedShape.param[0], 0.001f);
                            selectedShape.param[1] = std::max(selectedShape.param[1], 0.001f);
                            break;
                        case SHAPE_CYLINDER:
                            ImGui::DragFloat("Radius", &selectedShape.param[0], 0.01f);
                            ImGui::DragFloat("Half Height", &selectedShape.param[1], 0.01f);
                            selectedShape.param[0] = std::max(selectedShape.param[0], 0.001f);
                            selectedShape.param[1] = std::max(selectedShape.param[1], 0.001f);
                            break;
                        case SHAPE_CONE:
                            ImGui::DragFloat("Base Radius", &selectedShape.param[0], 0.01f);
                            ImGui::DragFloat("Half Height", &selectedShape.param[1], 0.01f);
                            selectedShape.param[0] = std::max(selectedShape.param[0], 0.001f);
                            selectedShape.param[1] = std::max(selectedShape.param[1], 0.001f);
                            break;
                        case SHAPE_MANDELBULB: {
                            ImGui::DragFloat("Power", &selectedShape.param[0], 0.05f, 2.0f, 16.0f);
                            int iterations = std::max(1, static_cast<int>(selectedShape.param[1] + 0.5f));
                            if (ImGui::SliderInt("Iterations", &iterations, 1, 64)) {
                                selectedShape.param[1] = static_cast<float>(iterations);
                            }
                            ImGui::DragFloat("Bailout", &selectedShape.param[2], 0.05f, 2.0f, 20.0f);
                            ImGui::DragFloat("Derivative Bias", &selectedShape.fractalExtra[0], 0.01f, 0.01f, 4.0f);
                            ImGui::DragFloat("Distance Scale", &selectedShape.fractalExtra[1], 0.01f, 0.01f, 4.0f);
                            selectedShape.param[0] = std::max(selectedShape.param[0], 2.0f);
                            selectedShape.param[1] = std::max(selectedShape.param[1], 1.0f);
                            selectedShape.param[2] = std::max(selectedShape.param[2], 2.0f);
                            selectedShape.fractalExtra[0] = std::max(selectedShape.fractalExtra[0], 0.01f);
                            selectedShape.fractalExtra[1] = std::max(selectedShape.fractalExtra[1], 0.01f);
                            break;
                        }
                        case SHAPE_MENGER_SPONGE: {
                            ImGui::DragFloat("Half Extent", &selectedShape.param[0], 0.01f, 0.01f, 4.0f);
                            int iterations = std::max(1, static_cast<int>(selectedShape.param[1] + 0.5f));
                            if (ImGui::SliderInt("Iterations##Menger", &iterations, 1, 8)) {
                                selectedShape.param[1] = static_cast<float>(iterations);
                            }
                            ImGui::DragFloat("Hole Scale", &selectedShape.param[2], 0.01f, 2.0f, 4.0f);
                            selectedShape.param[0] = std::max(selectedShape.param[0], 0.01f);
                            selectedShape.param[1] = std::max(selectedShape.param[1], 1.0f);
                            selectedShape.param[2] = std::max(selectedShape.param[2], 2.0f);
                            break;
                        }
                        case SHAPE_MESH_SDF: {
                            ImGui::DragFloat("Mesh Scale", &selectedShape.param[0], 0.01f, 0.01f, 100.0f);
                            selectedShape.param[0] = std::max(selectedShape.param[0], 0.01f);
                            const int meshId = static_cast<int>(selectedShape.param[1] + 0.5f);
                            ImGui::Text("Mesh ID: %d", meshId);
                            if (!selectedShape.meshSourcePath.empty()) {
                                ImGui::TextWrapped("Source: %s", selectedShape.meshSourcePath.c_str());
                            }
                            ImGui::TextDisabled("Import new meshes by dropping OBJ/FBX files in the viewport.");
                            break;
                        }
                        default:
                            ImGui::Text("Unknown shape type.");
                            break;
                    }

                    float rotDeg[3] = {
                        selectedShape.rotation[0] * 57.2958f,
                        selectedShape.rotation[1] * 57.2958f,
                        selectedShape.rotation[2] * 57.2958f
                    };
                    if (ImGui::DragFloat3("Rotation", rotDeg, 0.1f)) {
                        selectedShape.rotation[0] = rotDeg[0] * 0.0174533f;
                        selectedShape.rotation[1] = rotDeg[1] * 0.0174533f;
                        selectedShape.rotation[2] = rotDeg[2] * 0.0174533f;
                    }

                    selectedShape.scaleMode = globalScaleMode;
                    ImGui::Text("Scale Mode (Global): %s", globalScaleMode == SCALE_MODE_ELONGATE ? "Elongate" : "Deform");

                    if (globalScaleMode == SCALE_MODE_ELONGATE) {
                        ImGui::DragFloat3("Elongation", selectedShape.elongation, 0.01f, -100.0f, 100.0f);
                    } else {
                        ImGui::DragFloat3("Scale", selectedShape.scale, 0.01f, 0.01f, 100.0f);
                    }

                    selectedShape.scale[0] = std::max(selectedShape.scale[0], 0.01f);
                    selectedShape.scale[1] = std::max(selectedShape.scale[1], 0.01f);
                    selectedShape.scale[2] = std::max(selectedShape.scale[2], 0.01f);

                    ImGui::DragFloat("Rounding", &selectedShape.extra, 0.005f, 0.0f, 2.0f);
                    selectedShape.extra = std::max(selectedShape.extra, 0.0f);
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Material")) {
                    ImGui::ColorEdit3("Albedo", selectedShape.albedo);
                    ImGui::DragFloat("Metallic", &selectedShape.metallic, 0.01f, 0.0f, 1.0f);
                    ImGui::DragFloat("Roughness", &selectedShape.roughness, 0.01f, 0.0f, 1.0f);
                    ImGui::ColorEdit3("Emission Color", selectedShape.emission);
                    ImGui::DragFloat("Emission Strength", &selectedShape.emissionStrength, 0.05f, 0.0f, 100.0f);
                    ImGui::SliderFloat("Transmission", &selectedShape.transmission, 0.0f, 1.0f);
                    ImGui::DragFloat("IOR", &selectedShape.ior, 0.01f, 1.0f, 2.5f);
                    selectedShape.transmission = std::max(0.0f, std::min(selectedShape.transmission, 1.0f));
                    selectedShape.ior = std::max(selectedShape.ior, 1.0f);
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Blend")) {
                    const char* blendModeItems[] = { "None", "Smooth Union", "Smooth Subtraction", "Smooth Intersection" };
                    ImGui::Combo("Blend Mode", &selectedShape.blendOp, blendModeItems, IM_ARRAYSIZE(blendModeItems));
                    ImGui::DragFloat("Smoothness", &selectedShape.smoothness, 0.01f, 0.001f, 1.0f);
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        } else {
            selectedShapes.clear();
            clearMirrorHelperSelection(transformState);
            ImGui::TextDisabled("Select a shape in the Scene panel.");
        }
    } else {
        ImGui::TextDisabled("Select a shape in the Scene panel.");
    }

    ImGui::End();
}

} // namespace rmt
