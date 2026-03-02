#include "rmt/input/Input.h"

#include <GLFW/glfw3.h>

#include "ImGuizmo.h"
#include "rmt/RenderSettings.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace rmt {

void InputManager::processMousePickingAndCameraDrag(GLFWwindow* window,
                                                    std::vector<Shape>& shapes,
                                                    std::vector<int>& selectedShapes,
                                                    float cameraPos[3],
                                                    float cameraTarget[3],
                                                    float& camTheta,
                                                    float& camPhi,
                                                    bool& cameraDragging,
                                                    double& lastMouseX,
                                                    double& lastMouseY,
                                                    bool& mouseWasPressed,
                                                    TransformationState& ts,
                                                    float lightDir[3],
                                                    const RenderSettings& renderSettings,
                                                    const ImVec2& viewportPos,
                                                    const ImVec2& viewportSize) {
    static bool panningMode = false;

    const bool pointLightSelected =
        ts.selectedPointLightIndex >= 0 &&
        ts.selectedPointLightIndex < static_cast<int>(ts.pointLights.size());

    if ((ImGuizmo::IsUsing() || ImGuizmo::IsOver()) &&
        (!selectedShapes.empty() ||
         (ts.sunLightEnabled && ts.sunLightSelected) ||
         pointLightSelected)) {
        cameraDragging = false;
        mouseWasPressed = false;
        panningMode = false;
        return;
    }

    if (!ts.sunLightEnabled && (ts.sunLightSelected || ts.sunLightMoveModeActive || ts.sunLightHandleDragActive)) {
        ts.sunLightSelected = false;
        ts.sunLightMoveModeActive = false;
        ts.sunLightMoveConstrained = false;
        ts.sunLightMoveAxis = -1;
        ts.sunLightHandleDragActive = false;
    }

    if (ts.mirrorHelperMoveModeActive) {
        cameraDragging = false;
        mouseWasPressed = false;
        panningMode = false;
        return;
    }

    if (ts.sunLightMoveModeActive || ts.sunLightHandleDragActive || ts.pointLightMoveModeActive) {
        cameraDragging = false;
        mouseWasPressed = false;
        panningMode = false;
        return;
    }

    if (ts.curveNodeMoveModeActive || ts.curveNodeScaleModeActive) {
        cameraDragging = false;
        mouseWasPressed = false;
        panningMode = false;
        return;
    }

    double viewportX = viewportPos.x;
    double viewportY = viewportPos.y;
    double viewportW = viewportSize.x;
    double viewportH = viewportSize.y;
    if (viewportW <= 1.0 || viewportH <= 1.0) {
        int winWidth, winHeight;
        glfwGetWindowSize(window, &winWidth, &winHeight);
        viewportX = 0.0;
        viewportY = 0.0;
        viewportW = std::max(1, winWidth);
        viewportH = std::max(1, winHeight);
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    const bool mouseInViewport = mouseX >= viewportX && mouseX <= (viewportX + viewportW) &&
                                 mouseY >= viewportY && mouseY <= (viewportY + viewportH);
    const bool blockByUiPanel = shouldBlockViewportInput(mouseX, mouseY);

    auto computeOrthoScale = [&]() -> float {
        const float dx = cameraPos[0] - cameraTarget[0];
        const float dy = cameraPos[1] - cameraTarget[1];
        const float dz = cameraPos[2] - cameraTarget[2];
        const float cameraDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
        const float safeFovDegrees = std::max(20.0f, std::min(120.0f, renderSettings.cameraFovDegrees));
        const float halfFovRadians = safeFovDegrees * (3.14159265358979323846f / 180.0f) * 0.5f;
        return std::max(0.05f, cameraDistance * std::tan(halfFovRadians));
    };

    auto projectWorldToViewport = [&](const float world[3], ImVec2& outScreen, float& outDepth) -> bool {
        float forward[3] = {
            cameraTarget[0] - cameraPos[0],
            cameraTarget[1] - cameraPos[1],
            cameraTarget[2] - cameraPos[2]
        };
        float fLen = std::sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
        if (fLen < 1e-6f) {
            return false;
        }
        forward[0] /= fLen;
        forward[1] /= fLen;
        forward[2] /= fLen;

        float upDefault[3] = {0.0f, 1.0f, 0.0f};
        float right[3] = {
            forward[1] * upDefault[2] - forward[2] * upDefault[1],
            forward[2] * upDefault[0] - forward[0] * upDefault[2],
            forward[0] * upDefault[1] - forward[1] * upDefault[0],
        };
        float rLen = std::sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
        if (rLen < 1e-6f) {
            return false;
        }
        right[0] /= rLen;
        right[1] /= rLen;
        right[2] /= rLen;

        float up[3] = {
            right[1] * forward[2] - right[2] * forward[1],
            right[2] * forward[0] - right[0] * forward[2],
            right[0] * forward[1] - right[1] * forward[0],
        };

        const float rel[3] = {
            world[0] - cameraPos[0],
            world[1] - cameraPos[1],
            world[2] - cameraPos[2]
        };

        const float camX = rel[0] * right[0] + rel[1] * right[1] + rel[2] * right[2];
        const float camY = rel[0] * up[0] + rel[1] * up[1] + rel[2] * up[2];
        const float camZ = rel[0] * forward[0] + rel[1] * forward[1] + rel[2] * forward[2];
        outDepth = camZ;

        const float aspect = std::max(1e-5f, static_cast<float>(viewportW) / std::max(1e-5f, static_cast<float>(viewportH)));
        float ndcX = 0.0f;
        float ndcY = 0.0f;

        if (renderSettings.cameraProjectionMode == CAMERA_PROJECTION_ORTHOGRAPHIC) {
            const float orthoScale = computeOrthoScale();
            ndcX = camX / std::max(1e-5f, orthoScale * aspect);
            ndcY = camY / std::max(1e-5f, orthoScale);
        } else {
            if (camZ <= 1e-5f) {
                return false;
            }
            const float tanHalfFov = std::tan((renderSettings.cameraFovDegrees * (3.14159265358979323846f / 180.0f)) * 0.5f);
            ndcX = camX / (camZ * std::max(1e-5f, tanHalfFov * aspect));
            ndcY = camY / (camZ * std::max(1e-5f, tanHalfFov));
        }

        if (ndcX < -1.0f || ndcX > 1.0f || ndcY < -1.0f || ndcY > 1.0f) {
            return false;
        }

        outScreen.x = static_cast<float>(viewportX) + (ndcX * 0.5f + 0.5f) * static_cast<float>(viewportW);
        outScreen.y = static_cast<float>(viewportY) + (1.0f - (ndcY * 0.5f + 0.5f)) * static_cast<float>(viewportH);
        return true;
    };

    auto computePickRay = [&](float outRayDir[3]) -> bool {
        if (!mouseInViewport) {
            return false;
        }

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        int winWidth, winHeight;
        glfwGetWindowSize(window, &winWidth, &winHeight);
        float scaleX = static_cast<float>(fbWidth) / std::max(1, winWidth);
        float scaleY = static_cast<float>(fbHeight) / std::max(1, winHeight);
        float mouseLocalX = static_cast<float>(mouseX - viewportX) * scaleX;
        float mouseLocalY = static_cast<float>(mouseY - viewportY) * scaleY;
        float viewportFbW = static_cast<float>(viewportW) * scaleX;
        float viewportFbH = static_cast<float>(viewportH) * scaleY;
        viewportFbW = std::max(1.0f, viewportFbW);
        viewportFbH = std::max(1.0f, viewportFbH);

        double ndcX = (mouseLocalX / viewportFbW) * 2.0 - 1.0;
        double ndcY = ((viewportFbH - mouseLocalY) / static_cast<double>(viewportFbH)) * 2.0 - 1.0;
        ndcX *= static_cast<double>(viewportFbW) / viewportFbH;

        float forward[3] = {
            cameraTarget[0] - cameraPos[0],
            cameraTarget[1] - cameraPos[1],
            cameraTarget[2] - cameraPos[2],
        };
        float fLen =
            std::sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
        if (fLen < 1e-6f) {
            return false;
        }
        forward[0] /= fLen;
        forward[1] /= fLen;
        forward[2] /= fLen;

        float upDefault[3] = {0.0f, 1.0f, 0.0f};
        float right[3] = {
            forward[1] * upDefault[2] - forward[2] * upDefault[1],
            forward[2] * upDefault[0] - forward[0] * upDefault[2],
            forward[0] * upDefault[1] - forward[1] * upDefault[0],
        };
        float rLen = std::sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
        if (rLen < 1e-6f) {
            return false;
        }
        right[0] /= rLen;
        right[1] /= rLen;
        right[2] /= rLen;

        float upVec[3] = {
            right[1] * forward[2] - right[2] * forward[1],
            right[2] * forward[0] - right[0] * forward[2],
            right[0] * forward[1] - right[1] * forward[0],
        };
        outRayDir[0] = forward[0] + static_cast<float>(ndcX) * right[0] + static_cast<float>(ndcY) * upVec[0];
        outRayDir[1] = forward[1] + static_cast<float>(ndcX) * right[1] + static_cast<float>(ndcY) * upVec[1];
        outRayDir[2] = forward[2] + static_cast<float>(ndcX) * right[2] + static_cast<float>(ndcY) * upVec[2];

        const float len = std::sqrt(outRayDir[0] * outRayDir[0] + outRayDir[1] * outRayDir[1] +
                                    outRayDir[2] * outRayDir[2]);
        if (len < 1e-6f) {
            return false;
        }
        outRayDir[0] /= len;
        outRayDir[1] /= len;
        outRayDir[2] /= len;
        return true;
    };

    auto estimateMirrorHelperExtent = [](const Shape& shape) -> float {
        float baseRadius = 0.5f;
        switch (shape.type) {
            case SHAPE_SPHERE:
                baseRadius = std::max(shape.param[0], 0.01f);
                break;
            case SHAPE_BOX:
                baseRadius = std::sqrt(shape.param[0] * shape.param[0] + shape.param[1] * shape.param[1] +
                                       shape.param[2] * shape.param[2]);
                break;
            case SHAPE_TORUS:
                baseRadius = std::max(shape.param[0] + shape.param[1], 0.01f);
                break;
            case SHAPE_CYLINDER:
                baseRadius = std::sqrt(shape.param[0] * shape.param[0] + shape.param[1] * shape.param[1]);
                break;
            case SHAPE_CONE:
                baseRadius = std::sqrt(shape.param[0] * shape.param[0] + shape.param[1] * shape.param[1]);
                break;
            case SHAPE_MANDELBULB:
                baseRadius = std::max(1.5f, std::max(shape.param[2], 2.0f) * 0.5f);
                break;
            case SHAPE_MENGER_SPONGE:
                baseRadius = std::sqrt(3.0f) * std::max(shape.param[0], 0.01f);
                break;
            case SHAPE_MESH_SDF:
                baseRadius = std::sqrt(3.0f) * std::max(shape.param[0], 0.01f);
                break;
            default:
                break;
        }
        const float maxScale = std::max(std::fabs(shape.scale[0]),
                                        std::max(std::fabs(shape.scale[1]), std::fabs(shape.scale[2])));
        baseRadius *= std::max(maxScale, 0.001f);
        const float ex = std::max(shape.elongation[0], 0.0f);
        const float ey = std::max(shape.elongation[1], 0.0f);
        const float ez = std::max(shape.elongation[2], 0.0f);
        baseRadius += std::sqrt(ex * ex + ey * ey + ez * ez);
        baseRadius += std::max(shape.extra, 0.0f);
        return std::max(0.5f, baseRadius * 1.5f);
    };

    auto trySelectMirrorHelper = [&]() -> bool {
        if (selectedShapes.empty()) {
            return false;
        }
        const int selIdx = selectedShapes.front();
        if (selIdx < 0 || selIdx >= static_cast<int>(shapes.size())) {
            return false;
        }

        Shape& selectedShape = shapes[static_cast<std::size_t>(selIdx)];
        if (!selectedShape.mirrorModifierEnabled) {
            return false;
        }
        if (!selectedShape.mirrorAxis[0] && !selectedShape.mirrorAxis[1] && !selectedShape.mirrorAxis[2]) {
            return false;
        }

        float rayDir[3];
        if (!computePickRay(rayDir)) {
            return false;
        }

        const float helperExtent = estimateMirrorHelperExtent(selectedShape);
        float bestT = std::numeric_limits<float>::max();
        int bestAxis = -1;

        for (int axis = 0; axis < 3; ++axis) {
            if (!selectedShape.mirrorAxis[axis]) {
                continue;
            }

            const float roAxis = cameraPos[axis];
            const float rdAxis = rayDir[axis];
            if (std::fabs(rdAxis) < 1e-6f) {
                continue;
            }

            const float offset = selectedShape.mirrorOffset[axis];
            const float t = (offset - roAxis) / rdAxis;
            if (t <= 0.0f || t >= bestT || t > 500.0f) {
                continue;
            }

            const float hitPoint[3] = {
                cameraPos[0] + rayDir[0] * t,
                cameraPos[1] + rayDir[1] * t,
                cameraPos[2] + rayDir[2] * t,
            };

            float du = 0.0f;
            float dv = 0.0f;
            if (axis == 0) {
                du = hitPoint[1] - selectedShape.center[1];
                dv = hitPoint[2] - selectedShape.center[2];
            } else if (axis == 1) {
                du = hitPoint[0] - selectedShape.center[0];
                dv = hitPoint[2] - selectedShape.center[2];
            } else {
                du = hitPoint[0] - selectedShape.center[0];
                dv = hitPoint[1] - selectedShape.center[1];
            }

            if (std::fabs(du) <= helperExtent && std::fabs(dv) <= helperExtent) {
                bestT = t;
                bestAxis = axis;
            }
        }

        if (bestAxis < 0) {
            return false;
        }

        ts.mirrorHelperSelected = true;
        ts.mirrorHelperShapeIndex = selIdx;
        ts.mirrorHelperAxis = bestAxis;
        ts.mirrorHelperMoveModeActive = false;
        ts.mirrorHelperMoveConstrained = false;
        ts.mirrorHelperMoveAxis = -1;
        return true;
    };

    auto trySelectSunHelper = [&]() -> bool {
        if (!ts.sunLightEnabled) {
            return false;
        }

        float sunDir[3] = { lightDir[0], lightDir[1], lightDir[2] };
        const float dirLen = std::sqrt(sunDir[0] * sunDir[0] + sunDir[1] * sunDir[1] + sunDir[2] * sunDir[2]);
        if (dirLen > 1e-6f) {
            sunDir[0] /= dirLen;
            sunDir[1] /= dirLen;
            sunDir[2] /= dirLen;
        } else {
            sunDir[0] = 0.0f;
            sunDir[1] = 1.0f;
            sunDir[2] = 0.0f;
        }

        const float helperCenter[3] = {
            ts.sunLightPosition[0],
            ts.sunLightPosition[1],
            ts.sunLightPosition[2]
        };
        const float visualDir[3] = {
            -sunDir[0],
            -sunDir[1],
            -sunDir[2]
        };
        const float handleCenter[3] = {
            ts.sunLightPosition[0] + visualDir[0] * ts.sunLightHandleDistance,
            ts.sunLightPosition[1] + visualDir[1] * ts.sunLightHandleDistance,
            ts.sunLightPosition[2] + visualDir[2] * ts.sunLightHandleDistance
        };

        ImVec2 helperScreen(0.0f, 0.0f);
        ImVec2 handleScreen(0.0f, 0.0f);
        float helperDepth = 0.0f;
        float handleDepth = 0.0f;
        const bool helperVisible = projectWorldToViewport(helperCenter, helperScreen, helperDepth);
        const bool handleVisible = projectWorldToViewport(handleCenter, handleScreen, handleDepth);
        if (!helperVisible && !handleVisible) {
            return false;
        }

        const float helperRadius = ts.sunLightSelected ? 13.0f : 11.0f;
        const float handleRadius = ts.sunLightHandleDragActive ? 10.0f : 9.0f;

        float helperDist2 = std::numeric_limits<float>::max();
        float handleDist2 = std::numeric_limits<float>::max();
        bool hitHelper = false;
        bool hitHandle = false;

        if (helperVisible) {
            const float dx = static_cast<float>(mouseX) - helperScreen.x;
            const float dy = static_cast<float>(mouseY) - helperScreen.y;
            helperDist2 = dx * dx + dy * dy;
            hitHelper = helperDist2 <= helperRadius * helperRadius;
        }
        if (handleVisible) {
            const float dx = static_cast<float>(mouseX) - handleScreen.x;
            const float dy = static_cast<float>(mouseY) - handleScreen.y;
            handleDist2 = dx * dx + dy * dy;
            hitHandle = handleDist2 <= handleRadius * handleRadius;
        }

        if (!hitHelper && !hitHandle) {
            return false;
        }

        ts.sunLightSelected = true;
        ts.sunLightMoveModeActive = false;
        ts.sunLightMoveConstrained = false;
        ts.sunLightMoveAxis = -1;
        ts.selectedPointLightIndex = -1;
        ts.pointLightMoveModeActive = false;
        ts.pointLightMoveConstrained = false;
        ts.pointLightMoveAxis = -1;
        ts.mirrorHelperSelected = false;
        ts.mirrorHelperShapeIndex = -1;
        ts.mirrorHelperAxis = -1;
        ts.mirrorHelperMoveModeActive = false;
        ts.mirrorHelperMoveConstrained = false;
        ts.mirrorHelperMoveAxis = -1;
        ts.curveNodeSelected = false;
        ts.curveNodeShapeIndex = -1;
        ts.curveNodeIndex = -1;
        selectedShapes.clear();

        if (hitHandle && (!hitHelper || handleDist2 <= helperDist2)) {
            ts.sunLightHandleDragActive = true;
            glfwGetCursorPos(window, &ts.sunLightHandleStartMouseX, &ts.sunLightHandleStartMouseY);
            ts.sunLightHandleStartDirection[0] = visualDir[0];
            ts.sunLightHandleStartDirection[1] = visualDir[1];
            ts.sunLightHandleStartDirection[2] = visualDir[2];
        } else {
            ts.sunLightHandleDragActive = false;
        }

        return true;
    };

    auto trySelectPointLightHelper = [&]() -> bool {
        if (ts.pointLights.empty()) {
            return false;
        }

        float bestDist2 = std::numeric_limits<float>::max();
        float bestDepth = std::numeric_limits<float>::max();
        int bestIndex = -1;

        for (int i = 0; i < static_cast<int>(ts.pointLights.size()); ++i) {
            const PointLightState& pointLight = ts.pointLights[static_cast<std::size_t>(i)];
            ImVec2 pointScreen(0.0f, 0.0f);
            float pointDepth = 0.0f;
            if (!projectWorldToViewport(pointLight.position, pointScreen, pointDepth)) {
                continue;
            }

            const bool selected = (ts.selectedPointLightIndex == i);
            const float hitRadius = selected ? 14.0f : 12.0f;
            const float dx = static_cast<float>(mouseX) - pointScreen.x;
            const float dy = static_cast<float>(mouseY) - pointScreen.y;
            const float dist2 = dx * dx + dy * dy;
            if (dist2 > hitRadius * hitRadius) {
                continue;
            }

            const bool betterDistance = dist2 + 0.01f < bestDist2;
            const bool equalDistance = std::fabs(dist2 - bestDist2) <= 0.01f;
            if (betterDistance || (equalDistance && pointDepth < bestDepth)) {
                bestDist2 = dist2;
                bestDepth = pointDepth;
                bestIndex = i;
            }
        }

        if (bestIndex < 0) {
            return false;
        }

        ts.selectedPointLightIndex = bestIndex;
        ts.pointLightMoveModeActive = false;
        ts.pointLightMoveConstrained = false;
        ts.pointLightMoveAxis = -1;
        ts.sunLightSelected = false;
        ts.sunLightMoveModeActive = false;
        ts.sunLightMoveConstrained = false;
        ts.sunLightMoveAxis = -1;
        ts.sunLightHandleDragActive = false;
        ts.mirrorHelperSelected = false;
        ts.mirrorHelperShapeIndex = -1;
        ts.mirrorHelperAxis = -1;
        ts.mirrorHelperMoveModeActive = false;
        ts.mirrorHelperMoveConstrained = false;
        ts.mirrorHelperMoveAxis = -1;
        ts.curveNodeSelected = false;
        ts.curveNodeShapeIndex = -1;
        ts.curveNodeIndex = -1;
        selectedShapes.clear();
        return true;
    };

    auto trySelectCurveNode = [&]() -> bool {
        if (!ts.curveEditMode || selectedShapes.size() != 1) {
            return false;
        }

        const int shapeIndex = selectedShapes[0];
        if (shapeIndex < 0 || shapeIndex >= static_cast<int>(shapes.size())) {
            return false;
        }

        const Shape& selectedShape = shapes[static_cast<std::size_t>(shapeIndex)];
        if (selectedShape.type != SHAPE_CURVE || selectedShape.curveNodes.empty()) {
            return false;
        }

        int bestNodeIndex = -1;
        float bestDist2 = std::numeric_limits<float>::max();
        float bestDepth = std::numeric_limits<float>::max();

        for (int nodeIndex = 0; nodeIndex < static_cast<int>(selectedShape.curveNodes.size()); ++nodeIndex) {
            const CurveNode& node = selectedShape.curveNodes[static_cast<std::size_t>(nodeIndex)];
            const float worldPos[3] = {
                selectedShape.center[0] + node.position[0],
                selectedShape.center[1] + node.position[1],
                selectedShape.center[2] + node.position[2]
            };

            ImVec2 nodeScreen(0.0f, 0.0f);
            float nodeDepth = 0.0f;
            if (!projectWorldToViewport(worldPos, nodeScreen, nodeDepth)) {
                continue;
            }

            const float hitRadius =
                (ts.curveNodeSelected && ts.curveNodeShapeIndex == shapeIndex && ts.curveNodeIndex == nodeIndex)
                ? 13.0f
                : 11.0f;

            const float dx = static_cast<float>(mouseX) - nodeScreen.x;
            const float dy = static_cast<float>(mouseY) - nodeScreen.y;
            const float dist2 = dx * dx + dy * dy;
            if (dist2 > hitRadius * hitRadius) {
                continue;
            }

            const bool betterDistance = dist2 + 0.01f < bestDist2;
            const bool equalDistance = std::fabs(dist2 - bestDist2) <= 0.01f;
            if (betterDistance || (equalDistance && nodeDepth < bestDepth)) {
                bestDist2 = dist2;
                bestDepth = nodeDepth;
                bestNodeIndex = nodeIndex;
            }
        }

        if (bestNodeIndex < 0) {
            return false;
        }

        ts.curveNodeSelected = true;
        ts.curveNodeShapeIndex = shapeIndex;
        ts.curveNodeIndex = bestNodeIndex;
        ts.curveNodeMoveModeActive = false;
        ts.curveNodeMoveConstrained = false;
        ts.curveNodeMoveAxis = -1;
        return true;
    };

    if (!blockByUiPanel && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (!mouseWasPressed) {
            if (!mouseInViewport) {
                return;
            }
            mouseWasPressed = true;
            lastMouseX = mouseX;
            lastMouseY = mouseY;

            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
                panningMode = true;
                cameraDragging = true;
            } else {
                panningMode = false;
                if (trySelectSunHelper()) {
                    cameraDragging = false;
                } else if (trySelectPointLightHelper()) {
                    cameraDragging = false;
                } else if (trySelectMirrorHelper()) {
                    ts.curveNodeSelected = false;
                    ts.curveNodeShapeIndex = -1;
                    ts.curveNodeIndex = -1;
                    cameraDragging = false;
                } else if (trySelectCurveNode()) {
                    cameraDragging = false;
                } else {
                    float rayDir[3];
                    if (computePickRay(rayDir)) {
                        float t;
                        int hitShape = pickRayMarchSDF(cameraPos, rayDir, shapes, t);
                        if (hitShape >= 0) {
                            if (ImGui::GetIO().KeyShift) {
                                auto it = std::find(selectedShapes.begin(), selectedShapes.end(), hitShape);
                                if (it != selectedShapes.end()) {
                                    selectedShapes.erase(it);
                                } else {
                                    selectedShapes.push_back(hitShape);
                                }
                            } else {
                                selectedShapes.clear();
                                selectedShapes.push_back(hitShape);
                            }
                            ts.mirrorHelperSelected = false;
                            ts.mirrorHelperShapeIndex = -1;
                            ts.mirrorHelperAxis = -1;
                            ts.mirrorHelperMoveModeActive = false;
                            ts.mirrorHelperMoveConstrained = false;
                            ts.mirrorHelperMoveAxis = -1;
                            ts.sunLightSelected = false;
                            ts.sunLightMoveModeActive = false;
                            ts.sunLightMoveConstrained = false;
                            ts.sunLightMoveAxis = -1;
                            ts.sunLightHandleDragActive = false;
                            ts.selectedPointLightIndex = -1;
                            ts.pointLightMoveModeActive = false;
                            ts.pointLightMoveConstrained = false;
                            ts.pointLightMoveAxis = -1;
                            if (!ts.curveEditMode || shapes[hitShape].type != SHAPE_CURVE) {
                                ts.curveNodeSelected = false;
                                ts.curveNodeShapeIndex = -1;
                                ts.curveNodeIndex = -1;
                            } else {
                                ts.curveNodeShapeIndex = hitShape;
                                if (ts.curveNodeIndex < 0 ||
                                    ts.curveNodeIndex >= static_cast<int>(shapes[hitShape].curveNodes.size())) {
                                    ts.curveNodeIndex = 0;
                                }
                                ts.curveNodeSelected = !shapes[hitShape].curveNodes.empty();
                            }
                        } else {
                            selectedShapes.clear();
                            ts.mirrorHelperSelected = false;
                            ts.mirrorHelperShapeIndex = -1;
                            ts.mirrorHelperAxis = -1;
                            ts.mirrorHelperMoveModeActive = false;
                            ts.mirrorHelperMoveConstrained = false;
                            ts.mirrorHelperMoveAxis = -1;
                            ts.sunLightSelected = false;
                            ts.sunLightMoveModeActive = false;
                            ts.sunLightMoveConstrained = false;
                            ts.sunLightMoveAxis = -1;
                            ts.sunLightHandleDragActive = false;
                            ts.selectedPointLightIndex = -1;
                            ts.pointLightMoveModeActive = false;
                            ts.pointLightMoveConstrained = false;
                            ts.pointLightMoveAxis = -1;
                            ts.curveNodeSelected = false;
                            ts.curveNodeShapeIndex = -1;
                            ts.curveNodeIndex = -1;
                            cameraDragging = true;
                        }
                    } else {
                        cameraDragging = true;
                    }
                }
            }
        } else if (cameraDragging) {
            double deltaX = mouseX - lastMouseX;
            double deltaY = mouseY - lastMouseY;
            lastMouseX = mouseX;
            lastMouseY = mouseY;

            if (panningMode) {
                float sensitivity = 0.01f;

                float forward[3] = {
                    cameraTarget[0] - cameraPos[0],
                    cameraTarget[1] - cameraPos[1],
                    cameraTarget[2] - cameraPos[2],
                };
                float fLen =
                    std::sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
                forward[0] /= fLen;
                forward[1] /= fLen;
                forward[2] /= fLen;

                float upDefault[3] = {0.0f, 1.0f, 0.0f};
                float right[3] = {
                    forward[1] * upDefault[2] - forward[2] * upDefault[1],
                    forward[2] * upDefault[0] - forward[0] * upDefault[2],
                    forward[0] * upDefault[1] - forward[1] * upDefault[0],
                };
                float rLen = std::sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
                right[0] /= rLen;
                right[1] /= rLen;
                right[2] /= rLen;

                float up[3] = {
                    right[1] * forward[2] - right[2] * forward[1],
                    right[2] * forward[0] - right[0] * forward[2],
                    right[0] * forward[1] - right[1] * forward[0],
                };

                float panX = static_cast<float>(-deltaX) * sensitivity;
                float panY = static_cast<float>(deltaY) * sensitivity;

                cameraTarget[0] += panX * right[0] + panY * up[0];
                cameraTarget[1] += panX * right[1] + panY * up[1];
                cameraTarget[2] += panX * right[2] + panY * up[2];

                cameraPos[0] += panX * right[0] + panY * up[0];
                cameraPos[1] += panX * right[1] + panY * up[1];
                cameraPos[2] += panX * right[2] + panY * up[2];
            } else {
                camTheta -= static_cast<float>(deltaX) * 0.008f;
                camPhi += static_cast<float>(deltaY) * 0.008f;
                if (camPhi > 1.57f) {
                    camPhi = 1.57f;
                }
                if (camPhi < -1.57f) {
                    camPhi = -1.57f;
                }
            }
        }
    } else {
        mouseWasPressed = false;
        cameraDragging = false;
        panningMode = false;
    }
}

} // namespace rmt
