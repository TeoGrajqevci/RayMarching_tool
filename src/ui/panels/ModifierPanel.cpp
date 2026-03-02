#include "rmt/ui/panels/ModifierPanel.h"

#include <algorithm>
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

void sanitizeModifierStackOrder(Shape& shape) {
    const int defaults[3] = { SHAPE_MODIFIER_BEND, SHAPE_MODIFIER_TWIST, SHAPE_MODIFIER_ARRAY };
    bool used[3] = { false, false, false };
    int sanitized[3] = { SHAPE_MODIFIER_BEND, SHAPE_MODIFIER_TWIST, SHAPE_MODIFIER_ARRAY };
    int writeIndex = 0;

    for (int i = 0; i < 3; ++i) {
        const int value = shape.modifierStack[i];
        if (value < SHAPE_MODIFIER_BEND || value > SHAPE_MODIFIER_ARRAY) {
            continue;
        }
        if (!used[value]) {
            sanitized[writeIndex++] = value;
            used[value] = true;
        }
    }

    for (int i = 0; i < 3; ++i) {
        const int value = defaults[i];
        if (!used[value]) {
            sanitized[writeIndex++] = value;
            used[value] = true;
        }
    }

    shape.modifierStack[0] = sanitized[0];
    shape.modifierStack[1] = sanitized[1];
    shape.modifierStack[2] = sanitized[2];
}

bool isStackModifierActive(const Shape& shape, int modifierId) {
    if (modifierId == SHAPE_MODIFIER_BEND) {
        return shape.bendModifierEnabled;
    }
    if (modifierId == SHAPE_MODIFIER_TWIST) {
        return shape.twistModifierEnabled;
    }
    if (modifierId == SHAPE_MODIFIER_ARRAY) {
        return shape.arrayModifierEnabled;
    }
    return false;
}

const char* modifierStackLabel(int modifierId) {
    if (modifierId == SHAPE_MODIFIER_BEND) {
        return "Bend";
    }
    if (modifierId == SHAPE_MODIFIER_TWIST) {
        return "Twist";
    }
    if (modifierId == SHAPE_MODIFIER_ARRAY) {
        return "Array";
    }
    return "Unknown";
}

void swapModifierStackEntries(Shape& shape, int sourceId, int targetId) {
    if (sourceId == targetId) {
        return;
    }

    int sourceIndex = -1;
    int targetIndex = -1;
    for (int i = 0; i < 3; ++i) {
        if (shape.modifierStack[i] == sourceId) {
            sourceIndex = i;
        }
        if (shape.modifierStack[i] == targetId) {
            targetIndex = i;
        }
    }

    if (sourceIndex < 0 || targetIndex < 0) {
        return;
    }

    std::swap(shape.modifierStack[sourceIndex], shape.modifierStack[targetIndex]);
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
                 ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoTitleBar);

    if (ImGui::BeginTabBar("ModifierTabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Modifiers")) {
            if (!selectedShapes.empty()) {
                const int selIdx = selectedShapes[0];
                if (selIdx >= 0 && selIdx < static_cast<int>(shapes.size())) {
                    Shape& selectedShape = shapes[selIdx];
                    sanitizeModifierStackOrder(selectedShape);

                    int addModifierSelection = 0;
                    const char* modifierItems[] = { "Add modifier...", "Bend", "Twist", "Mirror", "Array" };
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
                        } else if (addModifierSelection == 4) {
                            selectedShape.arrayModifierEnabled = true;
                            if (!selectedShape.arrayAxis[0] && !selectedShape.arrayAxis[1] && !selectedShape.arrayAxis[2]) {
                                selectedShape.arrayAxis[0] = true;
                            }
                            selectedShape.arrayRepeatCount[0] = std::max(selectedShape.arrayRepeatCount[0], 3);
                            selectedShape.arrayRepeatCount[1] = std::max(selectedShape.arrayRepeatCount[1], 1);
                            selectedShape.arrayRepeatCount[2] = std::max(selectedShape.arrayRepeatCount[2], 1);
                        }
                    }

                    int activeStackModifiers = 0;
                    for (int i = 0; i < 3; ++i) {
                        if (isStackModifierActive(selectedShape, selectedShape.modifierStack[i])) {
                            ++activeStackModifiers;
                        }
                    }
                    if (activeStackModifiers > 1) {
                        ImGui::Separator();
                        ImGui::TextUnformatted("Modifier Stack (drag to reorder)");
                        for (int i = 0; i < 3; ++i) {
                            const int modifierId = selectedShape.modifierStack[i];
                            if (!isStackModifierActive(selectedShape, modifierId)) {
                                continue;
                            }

                            ImGui::PushID(i);
                            ImGui::Button(modifierStackLabel(modifierId), ImVec2(-1.0f, 0.0f));

                            if (ImGui::BeginDragDropSource()) {
                                ImGui::SetDragDropPayload("ModifierStackReorder", &modifierId, sizeof(int));
                                ImGui::TextUnformatted(modifierStackLabel(modifierId));
                                ImGui::EndDragDropSource();
                            }

                            if (ImGui::BeginDragDropTarget()) {
                                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ModifierStackReorder")) {
                                    const int sourceId = *static_cast<const int*>(payload->Data);
                                    swapModifierStackEntries(selectedShape, sourceId, modifierId);
                                }
                                ImGui::EndDragDropTarget();
                            }
                            ImGui::PopID();
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

                    if (selectedShape.arrayModifierEnabled) {
                        hasAnyModifier = true;
                        ImGui::Separator();
                        ImGui::TextUnformatted("Array");
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove##ModifierArray")) {
                            selectedShape.arrayModifierEnabled = false;
                            selectedShape.arrayAxis[0] = false;
                            selectedShape.arrayAxis[1] = false;
                            selectedShape.arrayAxis[2] = false;
                            selectedShape.arraySpacing[0] = 2.0f;
                            selectedShape.arraySpacing[1] = 2.0f;
                            selectedShape.arraySpacing[2] = 2.0f;
                            selectedShape.arrayRepeatCount[0] = 3;
                            selectedShape.arrayRepeatCount[1] = 3;
                            selectedShape.arrayRepeatCount[2] = 3;
                            selectedShape.arraySmoothness = 0.0f;
                        }
                        if (selectedShape.arrayModifierEnabled) {
                            ImGui::Checkbox("X##ModifierArrayAxisX", &selectedShape.arrayAxis[0]);
                            ImGui::SameLine();
                            ImGui::Checkbox("Y##ModifierArrayAxisY", &selectedShape.arrayAxis[1]);
                            ImGui::SameLine();
                            ImGui::Checkbox("Z##ModifierArrayAxisZ", &selectedShape.arrayAxis[2]);
                            ImGui::SliderInt("Count X##ModifierArrayCountX", &selectedShape.arrayRepeatCount[0], 1, 32);
                            ImGui::SliderInt("Count Y##ModifierArrayCountY", &selectedShape.arrayRepeatCount[1], 1, 32);
                            ImGui::SliderInt("Count Z##ModifierArrayCountZ", &selectedShape.arrayRepeatCount[2], 1, 32);
                            ImGui::SliderFloat("Spacing X##ModifierArraySpacingX", &selectedShape.arraySpacing[0], 0.1f, 20.0f, "%.3f");
                            ImGui::SliderFloat("Spacing Y##ModifierArraySpacingY", &selectedShape.arraySpacing[1], 0.1f, 20.0f, "%.3f");
                            ImGui::SliderFloat("Spacing Z##ModifierArraySpacingZ", &selectedShape.arraySpacing[2], 0.1f, 20.0f, "%.3f");
                            ImGui::SliderFloat("Smoothness##ModifierArraySmoothness", &selectedShape.arraySmoothness, 0.0f, 2.0f, "%.3f");
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
            const char* rayMarchQualityItems[] = { "Ultra", "High", "Medium", "Low" };
            ImGui::Combo("Raymarch Quality##ImageParamsRaymarchQuality",
                         &renderSettings.rayMarchQuality,
                         rayMarchQualityItems,
                         IM_ARRAYSIZE(rayMarchQualityItems));

            ImGui::Separator();
            ImGui::TextUnformatted("Path Tracer");
            ImGui::SliderInt("Max Bounces##ImageParamsPathBounces", &renderSettings.pathTracerMaxBounces, 1, 12);
            ImGui::Checkbox("Use Fixed Seed##ImageParamsPathFixedSeedToggle", &renderSettings.pathTracerUseFixedSeed);
            if (renderSettings.pathTracerUseFixedSeed) {
                ImGui::InputInt("Fixed Seed##ImageParamsPathFixedSeed", &renderSettings.pathTracerFixedSeed);
                renderSettings.pathTracerFixedSeed = std::max(renderSettings.pathTracerFixedSeed, 0);
            }
            ImGui::Checkbox("Guided Sampling + MIS##ImageParamsPathGuidedSampling", &renderSettings.pathTracerGuidedSamplingEnabled);
            if (renderSettings.pathTracerGuidedSamplingEnabled) {
                ImGui::SliderFloat("Guided Mix##ImageParamsPathGuidedMix", &renderSettings.pathTracerGuidedSamplingMix, 0.0f, 0.5f, "%.2f");
                ImGui::SliderFloat("MIS Power##ImageParamsPathMisPower", &renderSettings.pathTracerMisPower, 1.0f, 2.0f, "%.2f");
            }
            ImGui::SliderInt("Russian Roulette Start##ImageParamsPathRrStart", &renderSettings.pathTracerRussianRouletteStartBounce, 1, 8);
            ImGui::SliderFloat("Internal Resolution Scale##ImageParamsPathResScale", &renderSettings.pathTracerResolutionScale, 0.5f, 1.0f, "%.2f");
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
            ImGui::ColorEdit3("##ImageAmbientColor", ambientColor);
            ImGui::DragFloat("Ambient Intensity##ImageAmbientIntensity", &ambientIntensity, 0.1f, 0.0f, 0.0f, "%.3f");
            ambientIntensity = std::max(ambientIntensity, 0.0f);

            ImGui::Separator();
            ImGui::TextUnformatted("Background");
            ImGui::ColorEdit3("##ImageBackgroundColor", backgroundColor);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Camera")) {
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

            ImGui::Separator();
            ImGui::TextUnformatted("Physical Camera (Path Tracer)");
            ImGui::TextDisabled("Only affects the path tracer renderer.");
            ImGui::Checkbox("Enable Physical Camera##CameraPathPhysicalEnable", &renderSettings.pathTracerPhysicalCameraEnabled);
            if (renderSettings.pathTracerPhysicalCameraEnabled) {
                ImGui::SliderFloat("Focal Length (mm)##CameraPathPhysicalFocal",
                                   &renderSettings.pathTracerCameraFocalLengthMm,
                                   8.0f,
                                   400.0f,
                                   "%.1f");
                ImGui::SliderFloat("Sensor Width (mm)##CameraPathPhysicalSensorW",
                                   &renderSettings.pathTracerCameraSensorWidthMm,
                                   4.0f,
                                   70.0f,
                                   "%.2f");
                ImGui::SliderFloat("Sensor Height (mm)##CameraPathPhysicalSensorH",
                                   &renderSettings.pathTracerCameraSensorHeightMm,
                                   3.0f,
                                   70.0f,
                                   "%.2f");
                ImGui::SliderFloat("Aperture (f-stop)##CameraPathPhysicalAperture",
                                   &renderSettings.pathTracerCameraApertureFNumber,
                                   0.7f,
                                   64.0f,
                                   "f/%.1f");
                ImGui::DragFloat("Focus Distance##CameraPathPhysicalFocusDistance",
                                 &renderSettings.pathTracerCameraFocusDistance,
                                 0.05f,
                                 0.05f,
                                 5000.0f,
                                 "%.3f");
                ImGui::SliderInt("Aperture Blades##CameraPathPhysicalBladeCount",
                                 &renderSettings.pathTracerCameraBladeCount,
                                 0,
                                 12);
                if (renderSettings.pathTracerCameraBladeCount >= 3) {
                    ImGui::SliderFloat("Blade Rotation (deg)##CameraPathPhysicalBladeRotation",
                                       &renderSettings.pathTracerCameraBladeRotationDegrees,
                                       -180.0f,
                                       180.0f,
                                       "%.1f");
                }
                ImGui::SliderFloat("Anamorphic Ratio##CameraPathPhysicalAnamorphic",
                                   &renderSettings.pathTracerCameraAnamorphicRatio,
                                   0.25f,
                                   4.0f,
                                   "%.2f");
                ImGui::SliderFloat("Chromatic Aberration##CameraPathPhysicalChromatic",
                                   &renderSettings.pathTracerCameraChromaticAberration,
                                   0.0f,
                                   8.0f,
                                   "%.2f px");
                ImGui::DragFloat("Shutter (seconds)##CameraPathPhysicalShutter",
                                 &renderSettings.pathTracerCameraShutterSeconds,
                                 0.0001f,
                                 1.0f / 8000.0f,
                                 30.0f,
                                 "%.5f");
                ImGui::SliderFloat("ISO##CameraPathPhysicalIso",
                                   &renderSettings.pathTracerCameraIso,
                                   25.0f,
                                   25600.0f,
                                   "%.0f");
                ImGui::SliderFloat("Exposure Compensation (EV)##CameraPathPhysicalExposureComp",
                                   &renderSettings.pathTracerCameraExposureCompensationEv,
                                   -10.0f,
                                   10.0f,
                                   "%.2f");
            }

            if (renderSettings.pathTracerPhysicalCameraEnabled &&
                renderSettings.cameraProjectionMode == CAMERA_PROJECTION_PERSPECTIVE) {
                ImGui::TextDisabled("Path tracer FOV comes from Physical Camera settings.");
            }

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
