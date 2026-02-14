#include "rmt/input/transform/TranslationTool.h"

#include <algorithm>
#include <cmath>

#include "TransformToolCommon.h"

namespace rmt {

void updateTranslationTool(GLFWwindow* window,
                           std::vector<Shape>& shapes,
                           const std::vector<int>& selectedShapes,
                           TransformationState& ts,
                           const float cameraPos[3],
                           const float cameraTarget[3],
                           const ImVec2& viewportPos,
                           const ImVec2& viewportSize) {
    if (!ts.translationModeActive || selectedShapes.empty()) {
        return;
    }

    double currentMouseX = 0.0;
    double currentMouseY = 0.0;
    glfwGetCursorPos(window, &currentMouseX, &currentMouseY);
    const double deltaX = currentMouseX - ts.translationStartMouseX;
    const double deltaY = currentMouseY - ts.translationStartMouseY;
    const float sensitivity = 0.01f;

    if (ts.translationConstrained) {
        const float dragAmount = constrainedAxisDragAmount(ts.translationAxis, deltaX, deltaY, sensitivity);
        for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
            const int idx = selectedShapes[i];
            if (!ts.useLocalSpace || ts.translationAxis < 0 || ts.translationAxis > 2) {
                if (ts.translationAxis == 0) {
                    shapes[idx].center[0] = ts.translationOriginalCenters[i][0] + dragAmount;
                } else if (ts.translationAxis == 1) {
                    shapes[idx].center[1] = ts.translationOriginalCenters[i][1] + dragAmount;
                } else if (ts.translationAxis == 2) {
                    shapes[idx].center[2] = ts.translationOriginalCenters[i][2] + dragAmount;
                }
            } else {
                float axisDir[3];
                getLocalAxisWorldDirection(shapes[idx].rotation, ts.translationAxis, axisDir);
                shapes[idx].center[0] = ts.translationOriginalCenters[i][0] + axisDir[0] * dragAmount;
                shapes[idx].center[1] = ts.translationOriginalCenters[i][1] + axisDir[1] * dragAmount;
                shapes[idx].center[2] = ts.translationOriginalCenters[i][2] + axisDir[2] * dragAmount;
            }
        }
    } else {
        float forward[3] = {cameraTarget[0] - cameraPos[0],
                            cameraTarget[1] - cameraPos[1],
                            cameraTarget[2] - cameraPos[2]};
        const float fLen = std::sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
        if (fLen > 1e-6f) {
            forward[0] /= fLen;
            forward[1] /= fLen;
            forward[2] /= fLen;
        }
        const float upDefault[3] = {0.0f, 1.0f, 0.0f};
        float right[3] = {
            forward[1] * upDefault[2] - forward[2] * upDefault[1],
            forward[2] * upDefault[0] - forward[0] * upDefault[2],
            forward[0] * upDefault[1] - forward[1] * upDefault[0]
        };
        const float rLen = std::sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
        if (rLen > 1e-6f) {
            right[0] /= rLen;
            right[1] /= rLen;
            right[2] /= rLen;
        }
        const float up[3] = {
            right[1] * forward[2] - right[2] * forward[1],
            right[2] * forward[0] - right[0] * forward[2],
            right[0] * forward[1] - right[1] * forward[0]
        };
        for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
            const int idx = selectedShapes[i];
            shapes[idx].center[0] = ts.translationOriginalCenters[i][0] +
                                    static_cast<float>(deltaX) * sensitivity * right[0] -
                                    static_cast<float>(deltaY) * sensitivity * up[0];
            shapes[idx].center[1] = ts.translationOriginalCenters[i][1] +
                                    static_cast<float>(deltaX) * sensitivity * right[1] -
                                    static_cast<float>(deltaY) * sensitivity * up[1];
            shapes[idx].center[2] = ts.translationOriginalCenters[i][2] +
                                    static_cast<float>(deltaX) * sensitivity * right[2] -
                                    static_cast<float>(deltaY) * sensitivity * up[2];
        }
    }

    if (!ts.translationConstrained && !ImGui::GetIO().WantCaptureKeyboard) {
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
            ts.translationConstrained = true;
            ts.translationAxis = 0;
            ts.activeAxis = 0;
            updateGuideAxisDirection(ts, shapes, selectedShapes, ts.translationAxis);
            glfwGetCursorPos(window, &ts.translationStartMouseX, &ts.translationStartMouseY);
            for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
                const int idx = selectedShapes[i];
                ts.translationOriginalCenters[i][0] = shapes[idx].center[0];
                ts.translationOriginalCenters[i][1] = shapes[idx].center[1];
                ts.translationOriginalCenters[i][2] = shapes[idx].center[2];
            }
        } else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
            ts.translationConstrained = true;
            ts.translationAxis = 1;
            ts.activeAxis = 1;
            updateGuideAxisDirection(ts, shapes, selectedShapes, ts.translationAxis);
            glfwGetCursorPos(window, &ts.translationStartMouseX, &ts.translationStartMouseY);
            for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
                const int idx = selectedShapes[i];
                ts.translationOriginalCenters[i][0] = shapes[idx].center[0];
                ts.translationOriginalCenters[i][1] = shapes[idx].center[1];
                ts.translationOriginalCenters[i][2] = shapes[idx].center[2];
            }
        } else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
            ts.translationConstrained = true;
            ts.translationAxis = 2;
            ts.activeAxis = 2;
            updateGuideAxisDirection(ts, shapes, selectedShapes, ts.translationAxis);
            glfwGetCursorPos(window, &ts.translationStartMouseX, &ts.translationStartMouseY);
            for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
                const int idx = selectedShapes[i];
                ts.translationOriginalCenters[i][0] = shapes[idx].center[0];
                ts.translationOriginalCenters[i][1] = shapes[idx].center[1];
                ts.translationOriginalCenters[i][2] = shapes[idx].center[2];
            }
        }
    }

    if (isViewportClickAllowedForTransform(window, viewportPos, viewportSize)) {
        ts.translationModeActive = false;
        ts.translationConstrained = false;
        ts.showAxisGuides = false;
        ts.activeAxis = -1;
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
            const int idx = selectedShapes[i];
            shapes[idx].center[0] = ts.translationOriginalCenters[i][0];
            shapes[idx].center[1] = ts.translationOriginalCenters[i][1];
            shapes[idx].center[2] = ts.translationOriginalCenters[i][2];
        }
        ts.translationModeActive = false;
        ts.translationConstrained = false;
        ts.showAxisGuides = false;
        ts.activeAxis = -1;
    }
}

} // namespace rmt
