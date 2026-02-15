#include "rmt/input/Input.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

#include "rmt/input/transform/MirrorHelperTool.h"
#include "rmt/input/transform/RotationTool.h"
#include "rmt/input/transform/ScaleTool.h"
#include "rmt/input/transform/TranslationTool.h"

namespace rmt {

namespace {

float constrainedAxisDragAmountForSun(int axis, double deltaX, double deltaY, float sensitivity) {
    if (axis == 1) {
        return -static_cast<float>(deltaY) * sensitivity;
    }
    return static_cast<float>(deltaX) * sensitivity;
}

void updateSunLightTool(GLFWwindow* window,
                        TransformationState& ts,
                        float lightDir[3],
                        const float cameraPos[3],
                        const float cameraTarget[3],
                        const ImVec2& viewportPos,
                        const ImVec2& viewportSize) {
    auto normalizeDir = [](float v[3]) {
        const float len = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (len > 1e-6f) {
            v[0] /= len;
            v[1] /= len;
            v[2] /= len;
        }
    };

    auto clearMoveState = [&]() {
        ts.sunLightMoveModeActive = false;
        ts.sunLightMoveConstrained = false;
        ts.sunLightMoveAxis = -1;
    };

    if (!ts.sunLightEnabled) {
        ts.sunLightSelected = false;
        ts.sunLightHandleDragActive = false;
        clearMoveState();
        return;
    }

    if (ts.sunLightHandleDragActive) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS) {
            ts.sunLightHandleDragActive = false;
        } else {
            double currentMouseX = 0.0;
            double currentMouseY = 0.0;
            glfwGetCursorPos(window, &currentMouseX, &currentMouseY);
            double viewportX = viewportPos.x;
            double viewportY = viewportPos.y;
            double viewportW = viewportSize.x;
            double viewportH = viewportSize.y;
            if (viewportW <= 1.0 || viewportH <= 1.0) {
                int winWidth = 1;
                int winHeight = 1;
                glfwGetWindowSize(window, &winWidth, &winHeight);
                viewportX = 0.0;
                viewportY = 0.0;
                viewportW = std::max(1, winWidth);
                viewportH = std::max(1, winHeight);
            }

            const bool mouseInViewport =
                currentMouseX >= viewportX && currentMouseX <= (viewportX + viewportW) &&
                currentMouseY >= viewportY && currentMouseY <= (viewportY + viewportH);

            if (mouseInViewport) {
                int fbWidth = 1;
                int fbHeight = 1;
                glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
                int winWidth = 1;
                int winHeight = 1;
                glfwGetWindowSize(window, &winWidth, &winHeight);
                const float scaleX = static_cast<float>(fbWidth) / std::max(1, winWidth);
                const float scaleY = static_cast<float>(fbHeight) / std::max(1, winHeight);

                float mouseLocalX = static_cast<float>(currentMouseX - viewportX) * scaleX;
                float mouseLocalY = static_cast<float>(currentMouseY - viewportY) * scaleY;
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
                    cameraTarget[2] - cameraPos[2]
                };
                normalizeDir(forward);

                float upDefault[3] = {0.0f, 1.0f, 0.0f};
                float right[3] = {
                    forward[1] * upDefault[2] - forward[2] * upDefault[1],
                    forward[2] * upDefault[0] - forward[0] * upDefault[2],
                    forward[0] * upDefault[1] - forward[1] * upDefault[0]
                };
                normalizeDir(right);

                float up[3] = {
                    right[1] * forward[2] - right[2] * forward[1],
                    right[2] * forward[0] - right[0] * forward[2],
                    right[0] * forward[1] - right[1] * forward[0]
                };
                normalizeDir(up);

                float rayDir[3] = {
                    forward[0] + static_cast<float>(ndcX) * right[0] + static_cast<float>(ndcY) * up[0],
                    forward[1] + static_cast<float>(ndcX) * right[1] + static_cast<float>(ndcY) * up[1],
                    forward[2] + static_cast<float>(ndcX) * right[2] + static_cast<float>(ndcY) * up[2]
                };
                normalizeDir(rayDir);

                const float sunPos[3] = {
                    ts.sunLightPosition[0],
                    ts.sunLightPosition[1],
                    ts.sunLightPosition[2]
                };

                const float planeNumerator =
                    (sunPos[0] - cameraPos[0]) * forward[0] +
                    (sunPos[1] - cameraPos[1]) * forward[1] +
                    (sunPos[2] - cameraPos[2]) * forward[2];
                const float planeDenominator =
                    rayDir[0] * forward[0] + rayDir[1] * forward[1] + rayDir[2] * forward[2];

                if (std::fabs(planeDenominator) > 1e-6f) {
                    const float t = planeNumerator / planeDenominator;
                    if (t > 1e-4f) {
                        const float targetPoint[3] = {
                            cameraPos[0] + rayDir[0] * t,
                            cameraPos[1] + rayDir[1] * t,
                            cameraPos[2] + rayDir[2] * t
                        };

                        float visualDir[3] = {
                            targetPoint[0] - sunPos[0],
                            targetPoint[1] - sunPos[1],
                            targetPoint[2] - sunPos[2]
                        };
                        const float visualLen = std::sqrt(visualDir[0] * visualDir[0] +
                                                          visualDir[1] * visualDir[1] +
                                                          visualDir[2] * visualDir[2]);
                        if (visualLen > 1e-4f) {
                            visualDir[0] /= visualLen;
                            visualDir[1] /= visualLen;
                            visualDir[2] /= visualLen;
                            ts.sunLightHandleDistance = std::max(0.25f, visualLen);

                            lightDir[0] = -visualDir[0];
                            lightDir[1] = -visualDir[1];
                            lightDir[2] = -visualDir[2];
                            normalizeDir(lightDir);
                        }
                    }
                }
            }
        }
    }

    if (!ts.sunLightMoveModeActive) {
        return;
    }

    double currentMouseX = 0.0;
    double currentMouseY = 0.0;
    glfwGetCursorPos(window, &currentMouseX, &currentMouseY);
    const double deltaX = currentMouseX - ts.sunLightMoveStartMouseX;
    const double deltaY = currentMouseY - ts.sunLightMoveStartMouseY;
    const float sensitivity = 0.01f;

    float deltaMove[3] = {0.0f, 0.0f, 0.0f};
    if (ts.sunLightMoveConstrained && ts.sunLightMoveAxis >= 0 && ts.sunLightMoveAxis < 3) {
        const float dragAmount = constrainedAxisDragAmountForSun(ts.sunLightMoveAxis, deltaX, deltaY, sensitivity);
        deltaMove[ts.sunLightMoveAxis] = dragAmount;
    } else {
        float forward[3] = { cameraTarget[0] - cameraPos[0],
                             cameraTarget[1] - cameraPos[1],
                             cameraTarget[2] - cameraPos[2] };
        float fLen = std::sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
        if (fLen > 1e-6f) {
            forward[0] /= fLen;
            forward[1] /= fLen;
            forward[2] /= fLen;
            const float upDefault[3] = {0.0f, 1.0f, 0.0f};
            float right[3] = {
                forward[1] * upDefault[2] - forward[2] * upDefault[1],
                forward[2] * upDefault[0] - forward[0] * upDefault[2],
                forward[0] * upDefault[1] - forward[1] * upDefault[0]
            };
            float rLen = std::sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
            if (rLen > 1e-6f) {
                right[0] /= rLen;
                right[1] /= rLen;
                right[2] /= rLen;
                const float up[3] = {
                    right[1] * forward[2] - right[2] * forward[1],
                    right[2] * forward[0] - right[0] * forward[2],
                    right[0] * forward[1] - right[1] * forward[0]
                };
                deltaMove[0] = static_cast<float>(deltaX) * sensitivity * right[0] -
                               static_cast<float>(deltaY) * sensitivity * up[0];
                deltaMove[1] = static_cast<float>(deltaX) * sensitivity * right[1] -
                               static_cast<float>(deltaY) * sensitivity * up[1];
                deltaMove[2] = static_cast<float>(deltaX) * sensitivity * right[2] -
                               static_cast<float>(deltaY) * sensitivity * up[2];
            }
        }
    }

    ts.sunLightPosition[0] = ts.sunLightMoveStartPosition[0] + deltaMove[0];
    ts.sunLightPosition[1] = ts.sunLightMoveStartPosition[1] + deltaMove[1];
    ts.sunLightPosition[2] = ts.sunLightMoveStartPosition[2] + deltaMove[2];

    if (!ts.sunLightMoveConstrained && !ImGui::GetIO().WantCaptureKeyboard) {
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
            ts.sunLightMoveConstrained = true;
            ts.sunLightMoveAxis = 0;
            glfwGetCursorPos(window, &ts.sunLightMoveStartMouseX, &ts.sunLightMoveStartMouseY);
            ts.sunLightMoveStartPosition[0] = ts.sunLightPosition[0];
            ts.sunLightMoveStartPosition[1] = ts.sunLightPosition[1];
            ts.sunLightMoveStartPosition[2] = ts.sunLightPosition[2];
        } else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
            ts.sunLightMoveConstrained = true;
            ts.sunLightMoveAxis = 1;
            glfwGetCursorPos(window, &ts.sunLightMoveStartMouseX, &ts.sunLightMoveStartMouseY);
            ts.sunLightMoveStartPosition[0] = ts.sunLightPosition[0];
            ts.sunLightMoveStartPosition[1] = ts.sunLightPosition[1];
            ts.sunLightMoveStartPosition[2] = ts.sunLightPosition[2];
        } else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
            ts.sunLightMoveConstrained = true;
            ts.sunLightMoveAxis = 2;
            glfwGetCursorPos(window, &ts.sunLightMoveStartMouseX, &ts.sunLightMoveStartMouseY);
            ts.sunLightMoveStartPosition[0] = ts.sunLightPosition[0];
            ts.sunLightMoveStartPosition[1] = ts.sunLightPosition[1];
            ts.sunLightMoveStartPosition[2] = ts.sunLightPosition[2];
        }
    }

    double viewportX = viewportPos.x;
    double viewportY = viewportPos.y;
    double viewportW = viewportSize.x;
    double viewportH = viewportSize.y;
    if (viewportW <= 1.0 || viewportH <= 1.0) {
        int winWidth = 1;
        int winHeight = 1;
        glfwGetWindowSize(window, &winWidth, &winHeight);
        viewportX = 0.0;
        viewportY = 0.0;
        viewportW = std::max(1, winWidth);
        viewportH = std::max(1, winHeight);
    }

    const bool clickAllowed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (clickAllowed) {
        double mouseX = 0.0;
        double mouseY = 0.0;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        const bool mouseInViewport =
            mouseX >= viewportX && mouseX <= (viewportX + viewportW) &&
            mouseY >= viewportY && mouseY <= (viewportY + viewportH);
        const bool blockByUiPanel = shouldBlockViewportInput(mouseX, mouseY);
        if (!blockByUiPanel && mouseInViewport) {
            clearMoveState();
        }
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        ts.sunLightPosition[0] = ts.sunLightMoveStartPosition[0];
        ts.sunLightPosition[1] = ts.sunLightMoveStartPosition[1];
        ts.sunLightPosition[2] = ts.sunLightMoveStartPosition[2];
        clearMoveState();
        ts.sunLightHandleDragActive = false;
    }
}

void updatePointLightTool(GLFWwindow* window,
                          TransformationState& ts,
                          const float cameraPos[3],
                          const float cameraTarget[3],
                          const ImVec2& viewportPos,
                          const ImVec2& viewportSize) {
    auto clearMoveState = [&]() {
        ts.pointLightMoveModeActive = false;
        ts.pointLightMoveConstrained = false;
        ts.pointLightMoveAxis = -1;
    };

    if (ts.selectedPointLightIndex < 0 ||
        ts.selectedPointLightIndex >= static_cast<int>(ts.pointLights.size())) {
        ts.selectedPointLightIndex = -1;
        clearMoveState();
        return;
    }

    PointLightState& pointLight = ts.pointLights[static_cast<std::size_t>(ts.selectedPointLightIndex)];

    if (!ts.pointLightMoveModeActive) {
        return;
    }

    double currentMouseX = 0.0;
    double currentMouseY = 0.0;
    glfwGetCursorPos(window, &currentMouseX, &currentMouseY);
    const double deltaX = currentMouseX - ts.pointLightMoveStartMouseX;
    const double deltaY = currentMouseY - ts.pointLightMoveStartMouseY;
    const float sensitivity = 0.01f;

    float deltaMove[3] = {0.0f, 0.0f, 0.0f};
    if (ts.pointLightMoveConstrained && ts.pointLightMoveAxis >= 0 && ts.pointLightMoveAxis < 3) {
        const float dragAmount = constrainedAxisDragAmountForSun(ts.pointLightMoveAxis, deltaX, deltaY, sensitivity);
        deltaMove[ts.pointLightMoveAxis] = dragAmount;
    } else {
        float forward[3] = { cameraTarget[0] - cameraPos[0],
                             cameraTarget[1] - cameraPos[1],
                             cameraTarget[2] - cameraPos[2] };
        float fLen = std::sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
        if (fLen > 1e-6f) {
            forward[0] /= fLen;
            forward[1] /= fLen;
            forward[2] /= fLen;
            const float upDefault[3] = {0.0f, 1.0f, 0.0f};
            float right[3] = {
                forward[1] * upDefault[2] - forward[2] * upDefault[1],
                forward[2] * upDefault[0] - forward[0] * upDefault[2],
                forward[0] * upDefault[1] - forward[1] * upDefault[0]
            };
            float rLen = std::sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
            if (rLen > 1e-6f) {
                right[0] /= rLen;
                right[1] /= rLen;
                right[2] /= rLen;
                const float up[3] = {
                    right[1] * forward[2] - right[2] * forward[1],
                    right[2] * forward[0] - right[0] * forward[2],
                    right[0] * forward[1] - right[1] * forward[0]
                };
                deltaMove[0] = static_cast<float>(deltaX) * sensitivity * right[0] -
                               static_cast<float>(deltaY) * sensitivity * up[0];
                deltaMove[1] = static_cast<float>(deltaX) * sensitivity * right[1] -
                               static_cast<float>(deltaY) * sensitivity * up[1];
                deltaMove[2] = static_cast<float>(deltaX) * sensitivity * right[2] -
                               static_cast<float>(deltaY) * sensitivity * up[2];
            }
        }
    }

    pointLight.position[0] = ts.pointLightMoveStartPosition[0] + deltaMove[0];
    pointLight.position[1] = ts.pointLightMoveStartPosition[1] + deltaMove[1];
    pointLight.position[2] = ts.pointLightMoveStartPosition[2] + deltaMove[2];

    if (!ts.pointLightMoveConstrained && !ImGui::GetIO().WantCaptureKeyboard) {
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
            ts.pointLightMoveConstrained = true;
            ts.pointLightMoveAxis = 0;
            glfwGetCursorPos(window, &ts.pointLightMoveStartMouseX, &ts.pointLightMoveStartMouseY);
            ts.pointLightMoveStartPosition[0] = pointLight.position[0];
            ts.pointLightMoveStartPosition[1] = pointLight.position[1];
            ts.pointLightMoveStartPosition[2] = pointLight.position[2];
        } else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
            ts.pointLightMoveConstrained = true;
            ts.pointLightMoveAxis = 1;
            glfwGetCursorPos(window, &ts.pointLightMoveStartMouseX, &ts.pointLightMoveStartMouseY);
            ts.pointLightMoveStartPosition[0] = pointLight.position[0];
            ts.pointLightMoveStartPosition[1] = pointLight.position[1];
            ts.pointLightMoveStartPosition[2] = pointLight.position[2];
        } else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
            ts.pointLightMoveConstrained = true;
            ts.pointLightMoveAxis = 2;
            glfwGetCursorPos(window, &ts.pointLightMoveStartMouseX, &ts.pointLightMoveStartMouseY);
            ts.pointLightMoveStartPosition[0] = pointLight.position[0];
            ts.pointLightMoveStartPosition[1] = pointLight.position[1];
            ts.pointLightMoveStartPosition[2] = pointLight.position[2];
        }
    }

    double viewportX = viewportPos.x;
    double viewportY = viewportPos.y;
    double viewportW = viewportSize.x;
    double viewportH = viewportSize.y;
    if (viewportW <= 1.0 || viewportH <= 1.0) {
        int winWidth = 1;
        int winHeight = 1;
        glfwGetWindowSize(window, &winWidth, &winHeight);
        viewportX = 0.0;
        viewportY = 0.0;
        viewportW = std::max(1, winWidth);
        viewportH = std::max(1, winHeight);
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        double mouseX = 0.0;
        double mouseY = 0.0;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        const bool mouseInViewport =
            mouseX >= viewportX && mouseX <= (viewportX + viewportW) &&
            mouseY >= viewportY && mouseY <= (viewportY + viewportH);
        const bool blockByUiPanel = shouldBlockViewportInput(mouseX, mouseY);
        if (!blockByUiPanel && mouseInViewport) {
            clearMoveState();
        }
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        pointLight.position[0] = ts.pointLightMoveStartPosition[0];
        pointLight.position[1] = ts.pointLightMoveStartPosition[1];
        pointLight.position[2] = ts.pointLightMoveStartPosition[2];
        clearMoveState();
    }
}

} // namespace

InputManager::InputManager() {
}

InputManager::~InputManager() {
}

void InputManager::processTransformationUpdates(GLFWwindow* window,
                                                std::vector<Shape>& shapes,
                                                std::vector<int>& selectedShapes,
                                                TransformationState& ts,
                                                float lightDir[3],
                                                const float cameraPos[3],
                                                const float cameraTarget[3],
                                                const ImVec2& viewportPos,
                                                const ImVec2& viewportSize) {
    updateSunLightTool(window,
                       ts,
                       lightDir,
                       cameraPos,
                       cameraTarget,
                       viewportPos,
                       viewportSize);

    updatePointLightTool(window,
                         ts,
                         cameraPos,
                         cameraTarget,
                         viewportPos,
                         viewportSize);

    updateMirrorHelperTool(window,
                           shapes,
                           selectedShapes,
                           ts,
                           cameraPos,
                           cameraTarget,
                           viewportPos,
                           viewportSize);

    updateTranslationTool(window,
                          shapes,
                          selectedShapes,
                          ts,
                          cameraPos,
                          cameraTarget,
                          viewportPos,
                          viewportSize);

    updateRotationTool(window,
                       shapes,
                       selectedShapes,
                       ts,
                       viewportPos,
                       viewportSize);

    updateScaleTool(window,
                    shapes,
                    selectedShapes,
                    ts,
                    viewportPos,
                    viewportSize);
}

} // namespace rmt
