#include "rmt/ui/panels/InspectorPanel.h"

#include <algorithm>
#include <cstdint>
#include <string>

#include "imgui.h"

#include "rmt/rendering/Texture2D.h"
#include "rmt/ui/console/Console.h"

namespace rmt {

namespace {

bool isPointInsideRect(double x, double y, const ImVec2& minRect, const ImVec2& maxRect) {
    return x >= static_cast<double>(minRect.x) &&
           x <= static_cast<double>(maxRect.x) &&
           y >= static_cast<double>(minRect.y) &&
           y <= static_cast<double>(maxRect.y);
}

std::string fileNameFromPath(const std::string& path) {
    const std::string::size_type slash = path.find_last_of("/\\");
    if (slash == std::string::npos) {
        return path;
    }
    return path.substr(slash + 1);
}

void consumeMaterialTextureDrop(const char* channelLabel,
                                std::string& destinationPath,
                                const ImVec2& rectMin,
                                const ImVec2& rectMax,
                                const std::vector<DroppedFileEvent>& droppedFiles,
                                std::vector<char>& consumedDropFlags) {
    const std::size_t dropCount = std::min(droppedFiles.size(), consumedDropFlags.size());
    for (std::size_t dropIndex = 0; dropIndex < dropCount; ++dropIndex) {
        if (consumedDropFlags[dropIndex] != 0) {
            continue;
        }

        const DroppedFileEvent& dropped = droppedFiles[dropIndex];
        if (dropped.path.empty() || !isPointInsideRect(dropped.mouseX, dropped.mouseY, rectMin, rectMax)) {
            continue;
        }

        consumedDropFlags[dropIndex] = 1;
        if (!isSupportedMaterialTextureFile(dropped.path)) {
            Console::getInstance().addLog(
                std::string("Unsupported ") + channelLabel + " texture file: " + dropped.path +
                " (supported: .png/.jpg/.jpeg/.bmp/.tga/.ppm)",
                1);
            return;
        }

        MaterialTextureInfo textureInfo;
        if (!acquireMaterialTexture(dropped.path, textureInfo) || textureInfo.textureId == 0) {
            Console::getInstance().addLog(std::string("Failed to load ") + channelLabel + " texture: " + dropped.path, 2);
            return;
        }

        destinationPath = dropped.path;
        Console::getInstance().addLog(std::string("Assigned ") + channelLabel + " texture: " + dropped.path, 0);
        return;
    }
}

void renderMaterialTexturePreview(const char* channelLabel, std::string& texturePath) {
    if (texturePath.empty()) {
        return;
    }

    MaterialTextureInfo textureInfo;
    const bool loaded = acquireMaterialTexture(texturePath, textureInfo) && textureInfo.textureId != 0;

    ImGui::PushID(channelLabel);
    if (loaded) {
        ImGui::Image(static_cast<ImTextureID>(textureInfo.textureId), ImVec2(44.0f, 44.0f));
    } else {
        ImGui::Dummy(ImVec2(44.0f, 44.0f));
    }
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("%s Texture", channelLabel);
    ImGui::TextDisabled("%s", fileNameFromPath(texturePath).c_str());
    if (ImGui::SmallButton("Clear")) {
        texturePath.clear();
    }
    ImGui::EndGroup();
    ImGui::PopID();
}

} // namespace

void renderInspectorPanel(const ImVec2& viewportPos,
                          const ImVec2& viewportSize,
                          std::vector<Shape>& shapes,
                          std::vector<int>& selectedShapes,
                          TransformationState& transformState,
                          int globalScaleMode,
                          const std::vector<DroppedFileEvent>& droppedFiles,
                          std::vector<char>& consumedDropFlags) {
    if (selectedShapes.empty()) {
        return;
    }

    const int selIdx = selectedShapes[0];
    if (selIdx < 0 || selIdx >= static_cast<int>(shapes.size())) {
        selectedShapes.clear();
        clearMirrorHelperSelection(transformState);
        return;
    }

    const float overlayPadding = 12.0f;
    const float maxPanelWidth = 300.0f;
    const float minPanelWidth = 220.0f;
    const float maxPanelHeight = 320.0f;
    const float minPanelHeight = 150.0f;
    const float availableWidth = std::max(0.0f, viewportSize.x - overlayPadding * 2.0f);
    const float availableHeight = std::max(0.0f, viewportSize.y - overlayPadding * 2.0f);
    const float targetWidth = std::max(minPanelWidth, std::min(maxPanelWidth, availableWidth * 0.28f));
    const float panelWidth = std::min(targetWidth, std::max(220.0f, availableWidth));
    const float targetHeight = std::max(minPanelHeight, std::min(maxPanelHeight, availableHeight * 0.42f));
    const float panelHeight = std::min(targetHeight, availableHeight);

    ImGui::SetNextWindowPos(ImVec2(viewportPos.x + overlayPadding, viewportPos.y + overlayPadding), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
    ImGui::Begin("Inspector", nullptr,
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse);

    Shape& selectedShape = shapes[selIdx];
    if (selectedShape.type != SHAPE_CURVE &&
        transformState.curveNodeSelected &&
        transformState.curveNodeShapeIndex == selIdx) {
        transformState.curveNodeSelected = false;
        transformState.curveNodeShapeIndex = -1;
        transformState.curveNodeIndex = -1;
        transformState.curveEditMode = false;
    }

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
                case SHAPE_CURVE: {
                    const int nodeCount = static_cast<int>(selectedShape.curveNodes.size());
                    ImGui::Text("Nodes: %d", nodeCount);

                    const char* editLabel = transformState.curveEditMode ? "Exit Curve Edit" : "Edit Curve";
                    if (ImGui::Button(editLabel)) {
                        transformState.curveEditMode = !transformState.curveEditMode;
                        transformState.curveNodeMoveModeActive = false;
                        transformState.curveNodeMoveConstrained = false;
                        transformState.curveNodeMoveAxis = -1;
                    }
                    ImGui::SameLine();
                    ImGui::TextDisabled("Tab toggles");

                    if (transformState.curveEditMode) {
                        ImGui::TextDisabled("Curve Edit: click nodes in viewport, use gizmo or G to move, Shift+A to insert.");
                    } else {
                        ImGui::TextDisabled("Enable Curve Edit to manipulate nodes from viewport.");
                    }

                    if (nodeCount <= 0) {
                        CurveNode defaultNode;
                        selectedShape.curveNodes.push_back(defaultNode);
                    }

                    if (transformState.curveEditMode) {
                        if (transformState.curveNodeShapeIndex != selIdx ||
                            transformState.curveNodeIndex < 0 ||
                            transformState.curveNodeIndex >= static_cast<int>(selectedShape.curveNodes.size())) {
                            transformState.curveNodeShapeIndex = selIdx;
                            transformState.curveNodeIndex = 0;
                            transformState.curveNodeSelected = true;
                        }

                        ImGui::BeginChild("CurveNodeList", ImVec2(0.0f, 120.0f), true);
                        for (int nodeIndex = 0; nodeIndex < static_cast<int>(selectedShape.curveNodes.size()); ++nodeIndex) {
                            const bool isSelected = transformState.curveNodeSelected &&
                                                    transformState.curveNodeShapeIndex == selIdx &&
                                                    transformState.curveNodeIndex == nodeIndex;
                            if (ImGui::Selectable(("Node " + std::to_string(nodeIndex)).c_str(), isSelected)) {
                                transformState.curveNodeSelected = true;
                                transformState.curveNodeShapeIndex = selIdx;
                                transformState.curveNodeIndex = nodeIndex;
                            }
                        }
                        ImGui::EndChild();

                        if (transformState.curveNodeSelected &&
                            transformState.curveNodeShapeIndex == selIdx &&
                            transformState.curveNodeIndex >= 0 &&
                            transformState.curveNodeIndex < static_cast<int>(selectedShape.curveNodes.size())) {
                            CurveNode& node = selectedShape.curveNodes[static_cast<std::size_t>(transformState.curveNodeIndex)];
                            ImGui::DragFloat3("Node Position", node.position, 0.01f);
                            ImGui::DragFloat("Node Radius", &node.radius, 0.005f, 0.001f, 5.0f);
                            node.radius = std::max(node.radius, 0.001f);

                            if (ImGui::Button("Delete Node") && selectedShape.curveNodes.size() > 1) {
                                selectedShape.curveNodes.erase(
                                    selectedShape.curveNodes.begin() + transformState.curveNodeIndex
                                );
                                const int newCount = static_cast<int>(selectedShape.curveNodes.size());
                                transformState.curveNodeIndex = std::max(0, std::min(transformState.curveNodeIndex, newCount - 1));
                                transformState.curveNodeShapeIndex = selIdx;
                                transformState.curveNodeSelected = true;
                            }
                        }
                    }
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
            const bool hasAlbedoTexture = !selectedShape.albedoTexturePath.empty();
            if (hasAlbedoTexture) {
                ImGui::BeginDisabled();
            }
            ImGui::ColorEdit3("Albedo", selectedShape.albedo);
            if (hasAlbedoTexture) {
                ImGui::EndDisabled();
            }
            consumeMaterialTextureDrop("albedo",
                                       selectedShape.albedoTexturePath,
                                       ImGui::GetItemRectMin(),
                                       ImGui::GetItemRectMax(),
                                       droppedFiles,
                                       consumedDropFlags);
            renderMaterialTexturePreview("Albedo", selectedShape.albedoTexturePath);

            const bool hasMetallicTexture = !selectedShape.metallicTexturePath.empty();
            if (hasMetallicTexture) {
                ImGui::BeginDisabled();
            }
            ImGui::DragFloat("Metallic", &selectedShape.metallic, 0.01f, 0.0f, 1.0f);
            if (hasMetallicTexture) {
                ImGui::EndDisabled();
            }
            consumeMaterialTextureDrop("metallic",
                                       selectedShape.metallicTexturePath,
                                       ImGui::GetItemRectMin(),
                                       ImGui::GetItemRectMax(),
                                       droppedFiles,
                                       consumedDropFlags);
            renderMaterialTexturePreview("Metallic", selectedShape.metallicTexturePath);

            const bool hasRoughnessTexture = !selectedShape.roughnessTexturePath.empty();
            if (hasRoughnessTexture) {
                ImGui::BeginDisabled();
            }
            ImGui::DragFloat("Roughness", &selectedShape.roughness, 0.01f, 0.0f, 1.0f);
            if (hasRoughnessTexture) {
                ImGui::EndDisabled();
            }
            consumeMaterialTextureDrop("roughness",
                                       selectedShape.roughnessTexturePath,
                                       ImGui::GetItemRectMin(),
                                       ImGui::GetItemRectMax(),
                                       droppedFiles,
                                       consumedDropFlags);
            renderMaterialTexturePreview("Roughness", selectedShape.roughnessTexturePath);

            ImGui::TextUnformatted("Normal Map (drop on this label)");
            consumeMaterialTextureDrop("normal",
                                       selectedShape.normalTexturePath,
                                       ImGui::GetItemRectMin(),
                                       ImGui::GetItemRectMax(),
                                       droppedFiles,
                                       consumedDropFlags);
            renderMaterialTexturePreview("Normal", selectedShape.normalTexturePath);

            ImGui::DragFloat("Displacement Strength", &selectedShape.displacementStrength, 0.01f, 0.0f, 2.0f);
            consumeMaterialTextureDrop("displacement",
                                       selectedShape.displacementTexturePath,
                                       ImGui::GetItemRectMin(),
                                       ImGui::GetItemRectMax(),
                                       droppedFiles,
                                       consumedDropFlags);
            renderMaterialTexturePreview("Displacement", selectedShape.displacementTexturePath);

            ImGui::ColorEdit3("Emission Color", selectedShape.emission);
            ImGui::DragFloat("Emission Strength", &selectedShape.emissionStrength, 0.05f, 0.0f, 100.0f);
            ImGui::SliderFloat("Transmission", &selectedShape.transmission, 0.0f, 1.0f);
            ImGui::DragFloat("IOR", &selectedShape.ior, 0.01f, 1.0f, 2.5f);
            ImGui::SliderFloat("Dispersion", &selectedShape.dispersion, 0.0f, 1.0f);
            selectedShape.metallic = std::max(0.0f, std::min(selectedShape.metallic, 1.0f));
            selectedShape.roughness = std::max(0.0f, std::min(selectedShape.roughness, 1.0f));
            selectedShape.displacementStrength = std::max(selectedShape.displacementStrength, 0.0f);
            selectedShape.transmission = std::max(0.0f, std::min(selectedShape.transmission, 1.0f));
            selectedShape.ior = std::max(selectedShape.ior, 1.0f);
            selectedShape.dispersion = std::max(0.0f, std::min(selectedShape.dispersion, 1.0f));
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

    ImGui::End();
}

} // namespace rmt
