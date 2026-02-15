#include "rmt/input/Input.h"

#include <GLFW/glfw3.h>

#include "ImGuizmo.h"

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

        float rayDir[3];
        if (!computePickRay(rayDir)) {
            return false;
        }

        auto raySphereT = [](const float ro[3], const float rd[3], const float center[3], float radius) -> float {
            const float ocX = ro[0] - center[0];
            const float ocY = ro[1] - center[1];
            const float ocZ = ro[2] - center[2];
            const float b = 2.0f * (ocX * rd[0] + ocY * rd[1] + ocZ * rd[2]);
            const float c = ocX * ocX + ocY * ocY + ocZ * ocZ - radius * radius;
            const float disc = b * b - 4.0f * c;
            if (disc < 0.0f) {
                return std::numeric_limits<float>::max();
            }
            const float sqrtDisc = std::sqrt(disc);
            const float t0 = (-b - sqrtDisc) * 0.5f;
            const float t1 = (-b + sqrtDisc) * 0.5f;
            if (t0 > 0.0f) {
                return t0;
            }
            if (t1 > 0.0f) {
                return t1;
            }
            return std::numeric_limits<float>::max();
        };

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

        const float tHelper = raySphereT(cameraPos, rayDir, helperCenter, 0.22f);
        const float tHandle = raySphereT(cameraPos, rayDir, handleCenter, 0.16f);
        const bool hitHelper = tHelper < std::numeric_limits<float>::max();
        const bool hitHandle = tHandle < std::numeric_limits<float>::max();

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
        selectedShapes.clear();

        if (hitHandle && (!hitHelper || tHandle <= tHelper)) {
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

        float rayDir[3];
        if (!computePickRay(rayDir)) {
            return false;
        }

        constexpr float radius = 0.2f;
        float bestT = std::numeric_limits<float>::max();
        int bestIndex = -1;

        for (int i = 0; i < static_cast<int>(ts.pointLights.size()); ++i) {
            const PointLightState& pointLight = ts.pointLights[static_cast<std::size_t>(i)];
            const float ocX = cameraPos[0] - pointLight.position[0];
            const float ocY = cameraPos[1] - pointLight.position[1];
            const float ocZ = cameraPos[2] - pointLight.position[2];
            const float b = 2.0f * (ocX * rayDir[0] + ocY * rayDir[1] + ocZ * rayDir[2]);
            const float c = ocX * ocX + ocY * ocY + ocZ * ocZ - radius * radius;
            const float disc = b * b - 4.0f * c;
            if (disc < 0.0f) {
                continue;
            }

            const float sqrtDisc = std::sqrt(disc);
            const float t0 = (-b - sqrtDisc) * 0.5f;
            const float t1 = (-b + sqrtDisc) * 0.5f;
            float tCandidate = std::numeric_limits<float>::max();
            if (t0 > 0.0f) {
                tCandidate = t0;
            } else if (t1 > 0.0f) {
                tCandidate = t1;
            }

            if (tCandidate < bestT) {
                bestT = tCandidate;
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
        selectedShapes.clear();
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
