#include "rmt/ui/panels/ModifierPanel.h"

#include <cmath>

#include "imgui.h"

namespace rmt {

namespace {

const float kPi = 3.14159265358979323846f;

float computeOrthoScaleForCamera(const float cameraPos[3], const float cameraTarget[3], float fovDegrees) {
    const float dx = cameraPos[0] - cameraTarget[0];
    const float dy = cameraPos[1] - cameraTarget[1];
    const float dz = cameraPos[2] - cameraTarget[2];
    const float cameraDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
    const float safeFovDegrees = clampUiFloat(fovDegrees, 20.0f, 120.0f);
    const float halfFovRadians = safeFovDegrees * (kPi / 180.0f) * 0.5f;
    return std::max(0.05f, cameraDistance * std::tan(halfFovRadians));
}

} // namespace

void renderModifierPanel(const UiWorkspaceGeometry& geometry,
                         std::vector<Shape>& shapes,
                         std::vector<int>& selectedShapes,
                         TransformationState& transformState,
                         const float cameraPos[3],
                         const float cameraTarget[3],
                         float lightDir[3],
                         float lightColor[3],
                         float ambientColor[3],
                         float backgroundColor[3],
                         float& ambientIntensity,
                         float& directLightIntensity,
                         RenderSettings& renderSettings,
                         bool denoiserAvailable,
                         bool denoiserUsingGPU,
                         int pathSampleCount) {
    ImGui::SetNextWindowPos(ImVec2(geometry.rightX, geometry.rightModifierY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(geometry.rightPaneWidth, geometry.rightModifierH), ImGuiCond_Always);
    ImGui::Begin("Modifier", nullptr,
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse);

    if (ImGui::BeginTabBar("ModifierTabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Modifiers")) {
            if (!selectedShapes.empty()) {
                const int selIdx = selectedShapes[0];
                if (selIdx >= 0 && selIdx < static_cast<int>(shapes.size())) {
                    Shape& selectedShape = shapes[selIdx];

                    int addModifierSelection = 0;
                    const char* modifierItems[] = { "Add modifier...", "Bend", "Twist", "Mirror" };
                    if (ImGui::Combo("##ModifierAddSelector", &addModifierSelection, modifierItems, IM_ARRAYSIZE(modifierItems))) {
                        if (addModifierSelection == 1) {
                            selectedShape.bendModifierEnabled = true;
                        } else if (addModifierSelection == 2) {
                            selectedShape.twistModifierEnabled = true;
                        } else if (addModifierSelection == 3) {
                            selectedShape.mirrorModifierEnabled = true;
                            if (!selectedShape.mirrorAxis[0] && !selectedShape.mirrorAxis[1] && !selectedShape.mirrorAxis[2]) {
                                selectedShape.mirrorAxis[0] = true;
                            }
                            selectedShapes.clear();
                            selectedShapes.push_back(selIdx);
                            selectMirrorHelperForShape(selIdx, selectedShape, transformState);
                        }
                    }

                    bool hasAnyModifier = false;

                    if (selectedShape.bendModifierEnabled) {
                        hasAnyModifier = true;
                        ImGui::Separator();
                        ImGui::TextUnformatted("Bend");
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove##ModifierBend")) {
                            selectedShape.bendModifierEnabled = false;
                            selectedShape.bend[0] = 0.0f;
                            selectedShape.bend[1] = 0.0f;
                            selectedShape.bend[2] = 0.0f;
                        }
                        if (selectedShape.bendModifierEnabled) {
                            ImGui::DragFloat3("Amount (XYZ)##ModifierBendAmount", selectedShape.bend, 0.01f, -20.0f, 20.0f);
                        }
                    }

                    if (selectedShape.twistModifierEnabled) {
                        hasAnyModifier = true;
                        ImGui::Separator();
                        ImGui::TextUnformatted("Twist");
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove##ModifierTwist")) {
                            selectedShape.twistModifierEnabled = false;
                            selectedShape.twist[0] = 0.0f;
                            selectedShape.twist[1] = 0.0f;
                            selectedShape.twist[2] = 0.0f;
                        }
                        if (selectedShape.twistModifierEnabled) {
                            ImGui::DragFloat3("Amount (XYZ)##ModifierTwistAmount", selectedShape.twist, 0.01f, -20.0f, 20.0f);
                        }
                    }

                    if (selectedShape.mirrorModifierEnabled) {
                        hasAnyModifier = true;
                        ImGui::Separator();
                        ImGui::TextUnformatted("Mirror");
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove##ModifierMirror")) {
                            selectedShape.mirrorModifierEnabled = false;
                            selectedShape.mirrorAxis[0] = false;
                            selectedShape.mirrorAxis[1] = false;
                            selectedShape.mirrorAxis[2] = false;
                            selectedShape.mirrorSmoothness = 0.0f;
                            clearMirrorHelperSelection(transformState);
                        }
                        if (selectedShape.mirrorModifierEnabled) {
                            const bool hadAnyAxis = selectedShape.mirrorAxis[0] ||
                                                    selectedShape.mirrorAxis[1] ||
                                                    selectedShape.mirrorAxis[2];
                            ImGui::Checkbox("X##ModifierMirrorAxisX", &selectedShape.mirrorAxis[0]);
                            ImGui::SameLine();
                            ImGui::Checkbox("Y##ModifierMirrorAxisY", &selectedShape.mirrorAxis[1]);
                            ImGui::SameLine();
                            ImGui::Checkbox("Z##ModifierMirrorAxisZ", &selectedShape.mirrorAxis[2]);
                            ImGui::DragFloat("Smoothness##ModifierMirrorSmoothness",
                                             &selectedShape.mirrorSmoothness,
                                             0.005f,
                                             0.0f,
                                             2.0f);
                            const bool hasAnyAxis = selectedShape.mirrorAxis[0] ||
                                                    selectedShape.mirrorAxis[1] ||
                                                    selectedShape.mirrorAxis[2];
                            if (!hadAnyAxis && hasAnyAxis) {
                                selectMirrorHelperForShape(selIdx, selectedShape, transformState);
                            }
                            if (!selectedShape.mirrorAxis[0] &&
                                !selectedShape.mirrorAxis[1] &&
                                !selectedShape.mirrorAxis[2] &&
                                transformState.mirrorHelperSelected &&
                                transformState.mirrorHelperShapeIndex == selIdx) {
                                clearMirrorHelperSelection(transformState);
                            }
                        }
                    }

                    if (!hasAnyModifier) {
                        ImGui::Spacing();
                        ImGui::TextDisabled("No active modifiers.");
                    }
                } else {
                    selectedShapes.clear();
                    clearMirrorHelperSelection(transformState);
                    ImGui::TextDisabled("Select a shape in the Scene panel.");
                }
            } else {
                ImGui::TextDisabled("Select a shape in the Scene panel.");
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Image")) {
            ImGui::TextUnformatted("Camera");
            ImGui::SliderFloat("FOV (Degrees)##ImageParamsFov", &renderSettings.cameraFovDegrees, 20.0f, 120.0f, "%.1f");

            const char* projectionItems[] = { "Perspective", "Orthographic" };
            ImGui::Combo("Projection##ImageParamsProjection",
                         &renderSettings.cameraProjectionMode,
                         projectionItems,
                         IM_ARRAYSIZE(projectionItems));

            const float cameraDx = cameraPos[0] - cameraTarget[0];
            const float cameraDy = cameraPos[1] - cameraTarget[1];
            const float cameraDz = cameraPos[2] - cameraTarget[2];
            const float cameraDistance = std::sqrt(cameraDx * cameraDx + cameraDy * cameraDy + cameraDz * cameraDz);
            const float orthoScale = computeOrthoScaleForCamera(cameraPos, cameraTarget, renderSettings.cameraFovDegrees);
            if (renderSettings.cameraProjectionMode == CAMERA_PROJECTION_ORTHOGRAPHIC) {
                ImGui::Text("Orthographic Size: %.3f", orthoScale);
            }
            ImGui::Text("Camera Distance: %.3f", cameraDistance);

            const char* rayMarchQualityItems[] = { "Ultra", "High", "Medium", "Low" };
            ImGui::Combo("Raymarch Quality##ImageParamsRaymarchQuality",
                         &renderSettings.rayMarchQuality,
                         rayMarchQualityItems,
                         IM_ARRAYSIZE(rayMarchQualityItems));

            ImGui::Separator();
            ImGui::TextUnformatted("Path Tracer");
            ImGui::SliderInt("Max Bounces##ImageParamsPathBounces", &renderSettings.pathTracerMaxBounces, 1, 12);
            ImGui::Text("Accumulated Samples: %d", std::max(pathSampleCount, 0));

            ImGui::Separator();
            ImGui::TextUnformatted("Denoiser");
            if (denoiserAvailable) {
                ImGui::Text("Status: Available (%s)", denoiserUsingGPU ? "GPU" : "CPU");
            } else {
                ImGui::TextDisabled("Status: Unavailable");
            }

            if (!denoiserAvailable) {
                ImGui::BeginDisabled();
            }
            ImGui::Checkbox("Enable Denoiser##ImageParamsDenoiserEnabled", &renderSettings.denoiserEnabled);
            if (renderSettings.denoiserEnabled) {
                ImGui::SliderInt("Start Sample##ImageParamsDenoiseStart", &renderSettings.denoiseStartSample, 1, 256);
                ImGui::SliderInt("Interval##ImageParamsDenoiseInterval", &renderSettings.denoiseInterval, 1, 64);
            }
            if (!denoiserAvailable) {
                ImGui::EndDisabled();
            }

            ImGui::Separator();
            ImGui::TextUnformatted("Ambient");
            ImGui::TextUnformatted("Ambient Color");
            ImGui::ColorEdit3("##ImageAmbientColor", ambientColor);
            ImGui::DragFloat("Ambient Intensity##ImageAmbientIntensity", &ambientIntensity, 0.1f, 0.0f, 0.0f, "%.3f");
            ambientIntensity = std::max(ambientIntensity, 0.0f);

            ImGui::Separator();
            ImGui::TextUnformatted("Background");
            ImGui::TextUnformatted("Background Color");
            ImGui::ColorEdit3("##ImageBackgroundColor", backgroundColor);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Lighting")) {
            const bool pointSelected =
                transformState.selectedPointLightIndex >= 0 &&
                transformState.selectedPointLightIndex < static_cast<int>(transformState.pointLights.size());
            const bool anyLightPresent = transformState.sunLightEnabled || !transformState.pointLights.empty();
            if (!anyLightPresent) {
                ImGui::TextDisabled("No light in scene.");
                ImGui::TextDisabled("Use Shift + A -> Light -> Sun/Point to add one.");
            } else if (transformState.sunLightEnabled && transformState.sunLightSelected) {
                ImGui::TextUnformatted("Light Direction");
                if (ImGui::DragFloat3("##ModifierLightingDirection", lightDir, 0.01f)) {
                    const float len = std::sqrt(lightDir[0] * lightDir[0] + lightDir[1] * lightDir[1] + lightDir[2] * lightDir[2]);
                    if (len > 1e-6f) {
                        lightDir[0] /= len;
                        lightDir[1] /= len;
                        lightDir[2] /= len;
                    }
                }

                ImGui::TextUnformatted("Light Color");
                ImGui::ColorEdit3("##ModifierLightingColor", lightColor);
                ImGui::DragFloat("Direct Intensity##ModifierDirectIntensity", &directLightIntensity, 0.1f, 0.0f, 0.0f, "%.3f");
                directLightIntensity = std::max(directLightIntensity, 0.0f);
                ImGui::TextDisabled("Press X to delete the selected Sun light.");
            } else if (pointSelected) {
                PointLightState& pointLight = transformState.pointLights[static_cast<std::size_t>(transformState.selectedPointLightIndex)];
                ImGui::TextUnformatted("Point Light Position");
                ImGui::DragFloat3("##ModifierPointLightPosition", pointLight.position, 0.01f);

                ImGui::TextUnformatted("Point Light Color");
                ImGui::ColorEdit3("##ModifierPointLightColor", pointLight.color);
                ImGui::DragFloat("Point Intensity##ModifierPointIntensity", &pointLight.intensity, 0.1f, 0.0f, 0.0f, "%.3f");
                pointLight.intensity = std::max(pointLight.intensity, 0.0f);
                ImGui::SliderFloat("Point Range##ModifierPointRange", &pointLight.range, 0.25f, 50.0f);
                ImGui::SliderFloat("Point Radius##ModifierPointRadius", &pointLight.radius, 0.0f, 2.0f);
                ImGui::Text("Point Lights in scene: %d", static_cast<int>(transformState.pointLights.size()));
                ImGui::TextDisabled("Press X to delete the selected Point light.");
            } else {
                ImGui::TextDisabled("Select a light helper in the viewport to edit lighting.");
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

} // namespace rmt
