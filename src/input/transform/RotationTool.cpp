#include "rmt/input/transform/RotationTool.h"

#include <cmath>

#include "TransformToolCommon.h"

namespace rmt {

void updateRotationTool(GLFWwindow* window,
                        std::vector<Shape>& shapes,
                        const std::vector<int>& selectedShapes,
                        TransformationState& ts,
                        const ImVec2& viewportPos,
                        const ImVec2& viewportSize) {
    if (!ts.rotationModeActive || selectedShapes.empty()) {
        return;
    }

    double currentMouseX = 0.0;
    double currentMouseY = 0.0;
    glfwGetCursorPos(window, &currentMouseX, &currentMouseY);
    const double deltaX = currentMouseX - ts.rotationStartMouseX;
    const double deltaY = currentMouseY - ts.rotationStartMouseY;
    const float sensitivity = 0.005f;

    if (ts.rotationConstrained) {
        float axis[3] = {0.0f, 0.0f, 0.0f};
        const float angle = static_cast<float>(deltaX) * sensitivity;
        if (ts.useLocalSpace && ts.rotationAxis >= 0 && ts.rotationAxis < 3) {
            axis[0] = ts.guideAxisDirection[0];
            axis[1] = ts.guideAxisDirection[1];
            axis[2] = ts.guideAxisDirection[2];
            normalize3(axis);
        } else {
            setCartesianAxis(ts.rotationAxis, axis);
        }

        for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
            const int idx = selectedShapes[i];

            shapes[idx].center[0] = ts.rotationOriginalCenters[i][0];
            shapes[idx].center[1] = ts.rotationOriginalCenters[i][1];
            shapes[idx].center[2] = ts.rotationOriginalCenters[i][2];

            shapes[idx].rotation[0] = ts.rotationOriginalRotations[i][0];
            shapes[idx].rotation[1] = ts.rotationOriginalRotations[i][1];
            shapes[idx].rotation[2] = ts.rotationOriginalRotations[i][2];

            rotateAroundAxis(shapes[idx].center, ts.rotationCenter, axis, angle);

            if (ts.rotationAxis == 0) {
                shapes[idx].rotation[0] = ts.rotationOriginalRotations[i][0] + angle;
                shapes[idx].rotation[1] = ts.rotationOriginalRotations[i][1];
                shapes[idx].rotation[2] = ts.rotationOriginalRotations[i][2];
            } else if (ts.rotationAxis == 1) {
                shapes[idx].rotation[0] = ts.rotationOriginalRotations[i][0];
                shapes[idx].rotation[1] = ts.rotationOriginalRotations[i][1] + angle;
                shapes[idx].rotation[2] = ts.rotationOriginalRotations[i][2];
            } else if (ts.rotationAxis == 2) {
                shapes[idx].rotation[0] = ts.rotationOriginalRotations[i][0];
                shapes[idx].rotation[1] = ts.rotationOriginalRotations[i][1];
                shapes[idx].rotation[2] = ts.rotationOriginalRotations[i][2] + angle;
            }
        }
    } else {
        const float angleX = static_cast<float>(deltaY) * sensitivity;
        const float angleY = static_cast<float>(deltaX) * sensitivity;

        for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
            const int idx = selectedShapes[i];

            shapes[idx].center[0] = ts.rotationOriginalCenters[i][0];
            shapes[idx].center[1] = ts.rotationOriginalCenters[i][1];
            shapes[idx].center[2] = ts.rotationOriginalCenters[i][2];

            shapes[idx].rotation[0] = ts.rotationOriginalRotations[i][0];
            shapes[idx].rotation[1] = ts.rotationOriginalRotations[i][1];
            shapes[idx].rotation[2] = ts.rotationOriginalRotations[i][2];

            float axisY[3] = {0.0f, 1.0f, 0.0f};
            float axisX[3] = {1.0f, 0.0f, 0.0f};
            rotateAroundAxis(shapes[idx].center, ts.rotationCenter, axisY, angleY);
            rotateAroundAxis(shapes[idx].center, ts.rotationCenter, axisX, angleX);

            shapes[idx].rotation[0] = ts.rotationOriginalRotations[i][0] + angleX;
            shapes[idx].rotation[1] = ts.rotationOriginalRotations[i][1] + angleY;
            shapes[idx].rotation[2] = ts.rotationOriginalRotations[i][2];
        }
    }

    if (!ts.rotationConstrained && !ImGui::GetIO().WantCaptureKeyboard) {
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
            ts.rotationConstrained = true;
            ts.rotationAxis = 0;
            ts.activeAxis = 0;
            updateGuideAxisDirection(ts, shapes, selectedShapes, ts.rotationAxis);
            glfwGetCursorPos(window, &ts.rotationStartMouseX, &ts.rotationStartMouseY);
            for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
                const int idx = selectedShapes[i];
                ts.rotationOriginalRotations[i][0] = shapes[idx].rotation[0];
                ts.rotationOriginalRotations[i][1] = shapes[idx].rotation[1];
                ts.rotationOriginalRotations[i][2] = shapes[idx].rotation[2];
            }
        } else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
            ts.rotationConstrained = true;
            ts.rotationAxis = 1;
            ts.activeAxis = 1;
            updateGuideAxisDirection(ts, shapes, selectedShapes, ts.rotationAxis);
            glfwGetCursorPos(window, &ts.rotationStartMouseX, &ts.rotationStartMouseY);
            for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
                const int idx = selectedShapes[i];
                ts.rotationOriginalRotations[i][0] = shapes[idx].rotation[0];
                ts.rotationOriginalRotations[i][1] = shapes[idx].rotation[1];
                ts.rotationOriginalRotations[i][2] = shapes[idx].rotation[2];
            }
        } else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
            ts.rotationConstrained = true;
            ts.rotationAxis = 2;
            ts.activeAxis = 2;
            updateGuideAxisDirection(ts, shapes, selectedShapes, ts.rotationAxis);
            glfwGetCursorPos(window, &ts.rotationStartMouseX, &ts.rotationStartMouseY);
            for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
                const int idx = selectedShapes[i];
                ts.rotationOriginalRotations[i][0] = shapes[idx].rotation[0];
                ts.rotationOriginalRotations[i][1] = shapes[idx].rotation[1];
                ts.rotationOriginalRotations[i][2] = shapes[idx].rotation[2];
            }
        }
    }

    if (isViewportClickAllowedForTransform(window, viewportPos, viewportSize)) {
        ts.rotationModeActive = false;
        ts.rotationConstrained = false;
        ts.showAxisGuides = false;
        ts.activeAxis = -1;
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
            const int idx = selectedShapes[i];
            shapes[idx].rotation[0] = ts.rotationOriginalRotations[i][0];
            shapes[idx].rotation[1] = ts.rotationOriginalRotations[i][1];
            shapes[idx].rotation[2] = ts.rotationOriginalRotations[i][2];
        }
        ts.rotationModeActive = false;
        ts.rotationConstrained = false;
        ts.showAxisGuides = false;
        ts.activeAxis = -1;
    }
}

} // namespace rmt
