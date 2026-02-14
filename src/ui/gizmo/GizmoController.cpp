#include "rmt/ui/gizmo/GizmoController.h"

#include <algorithm>
#include <cmath>

#include "ImGuizmo.h"
#include "rmt/common/math/Vec3Math.h"

namespace rmt {

namespace {

const float kPi = 3.14159265358979323846f;

void buildViewMatrix(const float eye[3], const float target[3], float out[16]) {
    float x[3];
    float y[3] = { 0.0f, 1.0f, 0.0f };
    float z[3] = {
        eye[0] - target[0],
        eye[1] - target[1],
        eye[2] - target[2]
    };
    vec3Normalize(z);
    vec3Normalize(y);
    vec3Cross(y, z, x);
    if (vec3Length(x) < 1e-6f) {
        y[0] = 0.0f;
        y[1] = 0.0f;
        y[2] = 1.0f;
        vec3Cross(y, z, x);
    }
    vec3Normalize(x);
    vec3Cross(z, x, y);
    vec3Normalize(y);

    out[0] = x[0]; out[1] = y[0]; out[2] = z[0]; out[3] = 0.0f;
    out[4] = x[1]; out[5] = y[1]; out[6] = z[1]; out[7] = 0.0f;
    out[8] = x[2]; out[9] = y[2]; out[10] = z[2]; out[11] = 0.0f;
    out[12] = -vec3Dot(x, eye);
    out[13] = -vec3Dot(y, eye);
    out[14] = -vec3Dot(z, eye);
    out[15] = 1.0f;
}

float computeOrthoScaleForCamera(const float cameraPos[3], const float cameraTarget[3], float fovDegrees) {
    const float dx = cameraPos[0] - cameraTarget[0];
    const float dy = cameraPos[1] - cameraTarget[1];
    const float dz = cameraPos[2] - cameraTarget[2];
    const float cameraDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
    const float safeFovDegrees = clampUiFloat(fovDegrees, 20.0f, 120.0f);
    const float halfFovRadians = safeFovDegrees * (kPi / 180.0f) * 0.5f;
    return std::max(0.05f, cameraDistance * std::tan(halfFovRadians));
}

void buildProjectionMatrix(float fovYRadians,
                           float aspect,
                           float nearPlane,
                           float farPlane,
                           int projectionMode,
                           float orthoScale,
                           float out[16]) {
    std::fill(out, out + 16, 0.0f);
    const float safeAspect = std::max(aspect, 1e-5f);

    if (projectionMode == CAMERA_PROJECTION_ORTHOGRAPHIC) {
        const float safeOrthoScale = std::max(orthoScale, 1e-4f);
        const float right = safeOrthoScale * safeAspect;
        const float top = safeOrthoScale;

        out[0] = 1.0f / right;
        out[5] = 1.0f / top;
        out[10] = -2.0f / (farPlane - nearPlane);
        out[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        out[15] = 1.0f;
        return;
    }

    const float tanHalfFov = std::tan(fovYRadians * 0.5f);
    out[0] = 1.0f / (safeAspect * tanHalfFov);
    out[5] = 1.0f / tanHalfFov;
    out[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    out[11] = -1.0f;
    out[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
}

void composeShapeMatrix(const Shape& shape, float out[16]) {
    float translation[3] = { shape.center[0], shape.center[1], shape.center[2] };
    float rotationDeg[3] = {
        shape.rotation[0] * (180.0f / kPi),
        shape.rotation[1] * (180.0f / kPi),
        shape.rotation[2] * (180.0f / kPi)
    };
    float scale[3] = {
        std::max(0.01f, shape.scale[0]),
        std::max(0.01f, shape.scale[1]),
        std::max(0.01f, shape.scale[2])
    };
    ImGuizmo::RecomposeMatrixFromComponents(translation, rotationDeg, scale, out);
}

void decomposeShapeMatrix(const float matrix[16], Shape& shape) {
    float translation[3];
    float rotationDeg[3];
    float scale[3];
    ImGuizmo::DecomposeMatrixToComponents(matrix, translation, rotationDeg, scale);

    shape.center[0] = translation[0];
    shape.center[1] = translation[1];
    shape.center[2] = translation[2];
    shape.rotation[0] = rotationDeg[0] * (kPi / 180.0f);
    shape.rotation[1] = rotationDeg[1] * (kPi / 180.0f);
    shape.rotation[2] = rotationDeg[2] * (kPi / 180.0f);
    shape.scale[0] = std::max(0.01f, std::fabs(scale[0]));
    shape.scale[1] = std::max(0.01f, std::fabs(scale[1]));
    shape.scale[2] = std::max(0.01f, std::fabs(scale[2]));
}

ImGuizmo::OPERATION toGizmoOperation(int operation) {
    if (operation == 1) {
        return ImGuizmo::ROTATE;
    }
    if (operation == 2) {
        return ImGuizmo::SCALE;
    }
    return ImGuizmo::TRANSLATE;
}

ImGuizmo::MODE toGizmoMode(int mode) {
    return mode == 0 ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
}

void normalizeVec3(float v[3]) {
    const float len = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len > 1e-6f) {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
}

void directionFromEulerRadians(const float euler[3], float outDir[3]) {
    const float cx = std::cos(euler[0]);
    const float sx = std::sin(euler[0]);
    const float cy = std::cos(euler[1]);
    const float sy = std::sin(euler[1]);
    const float cz = std::cos(euler[2]);
    const float sz = std::sin(euler[2]);

    float v[3] = {0.0f, 0.0f, 1.0f};

    float vx = v[0];
    float vy = cx * v[1] - sx * v[2];
    float vz = sx * v[1] + cx * v[2];
    v[0] = vx; v[1] = vy; v[2] = vz;

    vx = cy * v[0] + sy * v[2];
    vy = v[1];
    vz = -sy * v[0] + cy * v[2];
    v[0] = vx; v[1] = vy; v[2] = vz;

    vx = cz * v[0] - sz * v[1];
    vy = sz * v[0] + cz * v[1];
    vz = v[2];
    v[0] = vx; v[1] = vy; v[2] = vz;

    outDir[0] = v[0];
    outDir[1] = v[1];
    outDir[2] = v[2];
    normalizeVec3(outDir);
}

void eulerDegreesFromDirection(const float dir[3], float outEulerDeg[3]) {
    float n[3] = {dir[0], dir[1], dir[2]};
    normalizeVec3(n);
    const float yaw = std::atan2(n[0], n[2]);
    const float pitch = std::asin(std::max(-1.0f, std::min(1.0f, n[1])));
    outEulerDeg[0] = pitch * (180.0f / kPi);
    outEulerDeg[1] = yaw * (180.0f / kPi);
    outEulerDeg[2] = 0.0f;
}

bool projectWorldToViewport(const float world[3],
                            const float eye[3],
                            const float target[3],
                            const RenderSettings& renderSettings,
                            const ImVec2& viewportPos,
                            const ImVec2& viewportSize,
                            ImVec2& outScreen) {
    float forward[3] = {
        target[0] - eye[0],
        target[1] - eye[1],
        target[2] - eye[2]
    };
    normalizeVec3(forward);

    float upDefault[3] = {0.0f, 1.0f, 0.0f};
    float right[3] = {
        forward[1] * upDefault[2] - forward[2] * upDefault[1],
        forward[2] * upDefault[0] - forward[0] * upDefault[2],
        forward[0] * upDefault[1] - forward[1] * upDefault[0]
    };
    normalizeVec3(right);
    float up[3] = {
        right[1] * forward[2] - right[2] * forward[1],
        right[2] * forward[0] - right[0] * forward[2],
        right[0] * forward[1] - right[1] * forward[0]
    };
    normalizeVec3(up);

    const float rel[3] = {
        world[0] - eye[0],
        world[1] - eye[1],
        world[2] - eye[2]
    };

    const float camX = rel[0] * right[0] + rel[1] * right[1] + rel[2] * right[2];
    const float camY = rel[0] * up[0] + rel[1] * up[1] + rel[2] * up[2];
    const float camZ = rel[0] * forward[0] + rel[1] * forward[1] + rel[2] * forward[2];

    const float aspect = std::max(1e-5f, viewportSize.x / std::max(viewportSize.y, 1e-5f));
    float ndcX = 0.0f;
    float ndcY = 0.0f;

    if (renderSettings.cameraProjectionMode == CAMERA_PROJECTION_ORTHOGRAPHIC) {
        const float orthoScale = computeOrthoScaleForCamera(eye, target, renderSettings.cameraFovDegrees);
        ndcX = camX / std::max(1e-5f, orthoScale * aspect);
        ndcY = camY / std::max(1e-5f, orthoScale);
    } else {
        if (camZ <= 1e-5f) {
            return false;
        }
        const float tanHalfFov = std::tan((renderSettings.cameraFovDegrees * (kPi / 180.0f)) * 0.5f);
        ndcX = camX / (camZ * std::max(1e-5f, tanHalfFov * aspect));
        ndcY = camY / (camZ * std::max(1e-5f, tanHalfFov));
    }

    outScreen.x = viewportPos.x + (ndcX * 0.5f + 0.5f) * viewportSize.x;
    outScreen.y = viewportPos.y + (1.0f - (ndcY * 0.5f + 0.5f)) * viewportSize.y;
    return true;
}

void drawSunHelperOverlay(ImDrawList* viewportDrawList,
                          const ImVec2& viewportPos,
                          const ImVec2& viewportSize,
                          const TransformationState& transformState,
                          const float lightDir[3],
                          const float cameraPos[3],
                          const float cameraTarget[3],
                          const RenderSettings& renderSettings) {
    float dir[3] = { -lightDir[0], -lightDir[1], -lightDir[2] };
    normalizeVec3(dir);

    const float sunPos[3] = {
        transformState.sunLightPosition[0],
        transformState.sunLightPosition[1],
        transformState.sunLightPosition[2]
    };
    const float handlePos[3] = {
        sunPos[0] + dir[0] * transformState.sunLightHandleDistance,
        sunPos[1] + dir[1] * transformState.sunLightHandleDistance,
        sunPos[2] + dir[2] * transformState.sunLightHandleDistance
    };

    ImVec2 sunScreen(0.0f, 0.0f);
    ImVec2 handleScreen(0.0f, 0.0f);
    if (!projectWorldToViewport(sunPos, cameraPos, cameraTarget, renderSettings, viewportPos, viewportSize, sunScreen)) {
        return;
    }
    if (!projectWorldToViewport(handlePos, cameraPos, cameraTarget, renderSettings, viewportPos, viewportSize, handleScreen)) {
        return;
    }

    const bool selected = transformState.sunLightSelected;
    const bool handleActive = transformState.sunLightHandleDragActive;
    const ImU32 sunColor = selected ? IM_COL32(255, 220, 100, 255) : IM_COL32(235, 190, 90, 220);
    const ImU32 ringColor = selected ? IM_COL32(255, 245, 180, 230) : IM_COL32(250, 220, 150, 200);
    const ImU32 handleColor = handleActive ? IM_COL32(255, 170, 60, 255) : IM_COL32(255, 210, 120, 235);

    const float iconRadius = selected ? 10.0f : 8.0f;
    const float rayInner = iconRadius + 2.0f;
    const float rayOuter = iconRadius + 6.0f;

    viewportDrawList->AddLine(sunScreen, handleScreen, ringColor, selected ? 2.4f : 1.8f);
    viewportDrawList->AddCircleFilled(sunScreen, iconRadius * 0.6f, sunColor, 24);
    viewportDrawList->AddCircle(sunScreen, iconRadius, ringColor, 24, 1.8f);

    for (int i = 0; i < 8; ++i) {
        const float angle = (2.0f * kPi * static_cast<float>(i)) / 8.0f;
        const ImVec2 p0(sunScreen.x + std::cos(angle) * rayInner,
                        sunScreen.y + std::sin(angle) * rayInner);
        const ImVec2 p1(sunScreen.x + std::cos(angle) * rayOuter,
                        sunScreen.y + std::sin(angle) * rayOuter);
        viewportDrawList->AddLine(p0, p1, ringColor, 1.4f);
    }

    viewportDrawList->AddCircleFilled(handleScreen, handleActive ? 6.0f : 5.0f, handleColor, 18);
    viewportDrawList->AddCircle(handleScreen, handleActive ? 8.0f : 7.0f, ringColor, 18, 1.6f);
}

void drawPointLightHelperOverlay(ImDrawList* viewportDrawList,
                                 const ImVec2& viewportPos,
                                 const ImVec2& viewportSize,
                                 const TransformationState& transformState,
                                 const float cameraPos[3],
                                 const float cameraTarget[3],
                                 const RenderSettings& renderSettings) {
    for (int i = 0; i < static_cast<int>(transformState.pointLights.size()); ++i) {
        const PointLightState& pointLight = transformState.pointLights[static_cast<std::size_t>(i)];
        const float pointPos[3] = {
            pointLight.position[0],
            pointLight.position[1],
            pointLight.position[2]
        };

        ImVec2 pointScreen(0.0f, 0.0f);
        if (!projectWorldToViewport(pointPos, cameraPos, cameraTarget, renderSettings, viewportPos, viewportSize, pointScreen)) {
            continue;
        }

        const bool selected = transformState.selectedPointLightIndex == i;
        const ImU32 coreColor = selected ? IM_COL32(255, 185, 120, 255) : IM_COL32(245, 170, 95, 230);
        const ImU32 haloColor = selected ? IM_COL32(255, 225, 180, 220) : IM_COL32(255, 210, 160, 180);
        const ImU32 crossColor = selected ? IM_COL32(255, 240, 200, 230) : IM_COL32(255, 225, 180, 190);

        const float coreRadius = selected ? 6.5f : 5.0f;
        const float haloRadius = selected ? 13.0f : 10.0f;

        viewportDrawList->AddCircleFilled(pointScreen, coreRadius, coreColor, 24);
        viewportDrawList->AddCircle(pointScreen, haloRadius, haloColor, 28, 1.7f);
        viewportDrawList->AddLine(ImVec2(pointScreen.x - haloRadius * 0.65f, pointScreen.y),
                                  ImVec2(pointScreen.x + haloRadius * 0.65f, pointScreen.y),
                                  crossColor,
                                  1.4f);
        viewportDrawList->AddLine(ImVec2(pointScreen.x, pointScreen.y - haloRadius * 0.65f),
                                  ImVec2(pointScreen.x, pointScreen.y + haloRadius * 0.65f),
                                  crossColor,
                                  1.4f);
    }
}

} // namespace

void renderViewportGizmo(ImDrawList* viewportDrawList,
                         const ImVec2& viewportPos,
                         const ImVec2& viewportSize,
                         std::vector<Shape>& shapes,
                         const std::vector<int>& selectedShapes,
                         TransformationState& transformState,
                         float lightDir[3],
                         const float cameraPos[3],
                         const float cameraTarget[3],
                         const RenderSettings& renderSettings,
                         UiRuntimeState& uiState) {
    if (!uiState.showViewportGizmo) {
        return;
    }

    if (viewportSize.x <= 1.0f || viewportSize.y <= 1.0f) {
        return;
    }

    const ImVec2 viewportClipMin = viewportPos;
    const ImVec2 viewportClipMax(viewportPos.x + viewportSize.x, viewportPos.y + viewportSize.y);
    viewportDrawList->PushClipRect(viewportClipMin, viewportClipMax, true);

    if (transformState.sunLightEnabled) {
        drawSunHelperOverlay(viewportDrawList,
                             viewportPos,
                             viewportSize,
                             transformState,
                             lightDir,
                             cameraPos,
                             cameraTarget,
                             renderSettings);
    }
    if (!transformState.pointLights.empty()) {
        drawPointLightHelperOverlay(viewportDrawList,
                                    viewportPos,
                                    viewportSize,
                                    transformState,
                                    cameraPos,
                                    cameraTarget,
                                    renderSettings);
    }

    float view[16];
    float projection[16];

    buildViewMatrix(cameraPos, cameraTarget, view);
    const float gizmoFovRadians = renderSettings.cameraFovDegrees * (kPi / 180.0f);
    const float gizmoOrthoScale = computeOrthoScaleForCamera(cameraPos, cameraTarget, renderSettings.cameraFovDegrees);
    buildProjectionMatrix(gizmoFovRadians,
                          viewportSize.x / viewportSize.y,
                          0.01f,
                          200.0f,
                          renderSettings.cameraProjectionMode,
                          gizmoOrthoScale,
                          projection);

    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(renderSettings.cameraProjectionMode == CAMERA_PROJECTION_ORTHOGRAPHIC);
    ImGuizmo::AllowAxisFlip(false);
    ImGuizmo::SetDrawlist(viewportDrawList);
    ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

    if (transformState.selectedPointLightIndex >= 0 &&
        transformState.selectedPointLightIndex < static_cast<int>(transformState.pointLights.size())) {
        PointLightState& pointLight =
            transformState.pointLights[static_cast<std::size_t>(transformState.selectedPointLightIndex)];
        float translation[3] = {
            pointLight.position[0],
            pointLight.position[1],
            pointLight.position[2]
        };
        float rotationDeg[3] = {0.0f, 0.0f, 0.0f};
        float scale[3] = {1.0f, 1.0f, 1.0f};
        float model[16];
        ImGuizmo::RecomposeMatrixFromComponents(translation, rotationDeg, scale, model);

        ImGuizmo::OPERATION operation = toGizmoOperation(uiState.gizmoOperation);
        if (operation != ImGuizmo::TRANSLATE) {
            operation = ImGuizmo::TRANSLATE;
        }

        if (ImGuizmo::Manipulate(view, projection, operation, ImGuizmo::WORLD, model)) {
            float outTranslation[3];
            float outRotationDeg[3];
            float outScale[3];
            ImGuizmo::DecomposeMatrixToComponents(model, outTranslation, outRotationDeg, outScale);

            pointLight.position[0] = outTranslation[0];
            pointLight.position[1] = outTranslation[1];
            pointLight.position[2] = outTranslation[2];
        }
        viewportDrawList->PopClipRect();
        return;
    }

    if (transformState.sunLightEnabled && transformState.sunLightSelected) {
        float translation[3] = {
            transformState.sunLightPosition[0],
            transformState.sunLightPosition[1],
            transformState.sunLightPosition[2]
        };
        float rotationDeg[3];
        eulerDegreesFromDirection(lightDir, rotationDeg);
        float scale[3] = {1.0f, 1.0f, 1.0f};
        float model[16];
        ImGuizmo::RecomposeMatrixFromComponents(translation, rotationDeg, scale, model);

        ImGuizmo::OPERATION operation = toGizmoOperation(uiState.gizmoOperation);
        if (operation == ImGuizmo::SCALE) {
            operation = ImGuizmo::TRANSLATE;
        }
        ImGuizmo::MODE mode = toGizmoMode(uiState.gizmoMode);
        if (operation == ImGuizmo::ROTATE) {
            mode = ImGuizmo::LOCAL;
        }

        if (ImGuizmo::Manipulate(view, projection, operation, mode, model)) {
            float outTranslation[3];
            float outRotationDeg[3];
            float outScale[3];
            ImGuizmo::DecomposeMatrixToComponents(model, outTranslation, outRotationDeg, outScale);

            transformState.sunLightPosition[0] = outTranslation[0];
            transformState.sunLightPosition[1] = outTranslation[1];
            transformState.sunLightPosition[2] = outTranslation[2];

            if (operation == ImGuizmo::ROTATE) {
                const float eulerRad[3] = {
                    outRotationDeg[0] * (kPi / 180.0f),
                    outRotationDeg[1] * (kPi / 180.0f),
                    outRotationDeg[2] * (kPi / 180.0f)
                };
                directionFromEulerRadians(eulerRad, lightDir);
                normalizeVec3(lightDir);
            }
        }
        viewportDrawList->PopClipRect();
        return;
    }

    if (selectedShapes.empty()) {
        viewportDrawList->PopClipRect();
        return;
    }

    const int selIdx = selectedShapes[0];
    if (selIdx < 0 || selIdx >= static_cast<int>(shapes.size())) {
        viewportDrawList->PopClipRect();
        return;
    }

    const bool helperSelectionActive =
        transformState.mirrorHelperSelected &&
        transformState.mirrorHelperShapeIndex == selIdx &&
        shapes[selIdx].mirrorModifierEnabled &&
        (shapes[selIdx].mirrorAxis[0] || shapes[selIdx].mirrorAxis[1] || shapes[selIdx].mirrorAxis[2]);

    if (helperSelectionActive) {
        if (uiState.elongateScaleDragActive) {
            uiState.elongateScaleDragActive = false;
            uiState.elongateScaleDragShape = -1;
        }

        float helperTranslation[3] = {
            shapes[selIdx].mirrorOffset[0],
            shapes[selIdx].mirrorOffset[1],
            shapes[selIdx].mirrorOffset[2]
        };
        float helperRotationDeg[3] = { 0.0f, 0.0f, 0.0f };
        float helperScale[3] = { 1.0f, 1.0f, 1.0f };
        float helperModel[16];
        ImGuizmo::RecomposeMatrixFromComponents(helperTranslation, helperRotationDeg, helperScale, helperModel);

        if (ImGuizmo::Manipulate(view, projection, ImGuizmo::TRANSLATE, ImGuizmo::WORLD, helperModel)) {
            float manipulatedTranslation[3];
            float manipulatedRotationDeg[3];
            float manipulatedScale[3];
            ImGuizmo::DecomposeMatrixToComponents(helperModel, manipulatedTranslation, manipulatedRotationDeg, manipulatedScale);

            const float deltaX = manipulatedTranslation[0] - shapes[selIdx].mirrorOffset[0];
            const float deltaY = manipulatedTranslation[1] - shapes[selIdx].mirrorOffset[1];
            const float deltaZ = manipulatedTranslation[2] - shapes[selIdx].mirrorOffset[2];
            shapes[selIdx].mirrorOffset[0] = manipulatedTranslation[0];
            shapes[selIdx].mirrorOffset[1] = manipulatedTranslation[1];
            shapes[selIdx].mirrorOffset[2] = manipulatedTranslation[2];
            shapes[selIdx].center[0] += deltaX;
            shapes[selIdx].center[1] += deltaY;
            shapes[selIdx].center[2] += deltaZ;
        }
        viewportDrawList->PopClipRect();
        return;
    }

    float model[16];
    composeShapeMatrix(shapes[selIdx], model);

    const ImGuizmo::OPERATION operation = toGizmoOperation(uiState.gizmoOperation);
    ImGuizmo::MODE mode = toGizmoMode(uiState.gizmoMode);
    if (operation == ImGuizmo::SCALE) {
        mode = ImGuizmo::LOCAL;
    }
    const bool elongateScaleMode = (operation == ImGuizmo::SCALE && uiState.globalScaleMode == SCALE_MODE_ELONGATE);
    if (!elongateScaleMode && uiState.elongateScaleDragActive) {
        uiState.elongateScaleDragActive = false;
        uiState.elongateScaleDragShape = -1;
    }

    const bool manipulated = ImGuizmo::Manipulate(view, projection, operation, mode, model);
    const bool usingGizmo = ImGuizmo::IsUsing();

    if (elongateScaleMode) {
        if (usingGizmo && (!uiState.elongateScaleDragActive || uiState.elongateScaleDragShape != selIdx)) {
            uiState.elongateScaleDragActive = true;
            uiState.elongateScaleDragShape = selIdx;
            uiState.elongateScaleStartScale[0] = std::max(0.01f, shapes[selIdx].scale[0]);
            uiState.elongateScaleStartScale[1] = std::max(0.01f, shapes[selIdx].scale[1]);
            uiState.elongateScaleStartScale[2] = std::max(0.01f, shapes[selIdx].scale[2]);
            uiState.elongateScaleStartElongation[0] = shapes[selIdx].elongation[0];
            uiState.elongateScaleStartElongation[1] = shapes[selIdx].elongation[1];
            uiState.elongateScaleStartElongation[2] = shapes[selIdx].elongation[2];
        }

        if (usingGizmo && uiState.elongateScaleDragShape == selIdx) {
            float translation[3];
            float rotationDeg[3];
            float manipulatedScale[3];
            ImGuizmo::DecomposeMatrixToComponents(model, translation, rotationDeg, manipulatedScale);
            shapes[selIdx].elongation[0] = uiState.elongateScaleStartElongation[0] +
                                           (manipulatedScale[0] - uiState.elongateScaleStartScale[0]);
            shapes[selIdx].elongation[1] = uiState.elongateScaleStartElongation[1] +
                                           (manipulatedScale[1] - uiState.elongateScaleStartScale[1]);
            shapes[selIdx].elongation[2] = uiState.elongateScaleStartElongation[2] +
                                           (manipulatedScale[2] - uiState.elongateScaleStartScale[2]);
        } else if (!usingGizmo && uiState.elongateScaleDragActive) {
            uiState.elongateScaleDragActive = false;
            uiState.elongateScaleDragShape = -1;
        }
    } else if (manipulated) {
        decomposeShapeMatrix(model, shapes[selIdx]);
    }

    viewportDrawList->PopClipRect();
}

} // namespace rmt
