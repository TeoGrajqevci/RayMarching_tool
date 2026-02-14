#include "rmt/input/transform/MirrorHelperTool.h"

#include <cmath>

#include "TransformToolCommon.h"

namespace rmt {

void updateMirrorHelperTool(GLFWwindow* window,
                            std::vector<Shape>& shapes,
                            const std::vector<int>& selectedShapes,
                            TransformationState& ts,
                            const float cameraPos[3],
                            const float cameraTarget[3],
                            const ImVec2& viewportPos,
                            const ImVec2& viewportSize) {
    auto clearMirrorHelperMoveState = [&]() {
        ts.mirrorHelperMoveModeActive = false;
        ts.mirrorHelperMoveConstrained = false;
        ts.mirrorHelperMoveAxis = -1;
    };

    if (!ts.mirrorHelperMoveModeActive) {
        return;
    }

    const int helperShapeIdx = ts.mirrorHelperShapeIndex;
    const bool hasMatchingSelection =
        !selectedShapes.empty() && selectedShapes.front() == helperShapeIdx;
    const bool helperShapeIndexValid =
        helperShapeIdx >= 0 && helperShapeIdx < static_cast<int>(shapes.size());
    const bool helperShapeEnabled = helperShapeIndexValid &&
        shapes[static_cast<std::size_t>(helperShapeIdx)].mirrorModifierEnabled &&
        (shapes[static_cast<std::size_t>(helperShapeIdx)].mirrorAxis[0] ||
         shapes[static_cast<std::size_t>(helperShapeIdx)].mirrorAxis[1] ||
         shapes[static_cast<std::size_t>(helperShapeIdx)].mirrorAxis[2]);

    if (!ts.mirrorHelperSelected || !hasMatchingSelection || !helperShapeEnabled) {
        clearMirrorHelperMoveState();
        return;
    }

    Shape& helperShape = shapes[static_cast<std::size_t>(helperShapeIdx)];
    double currentMouseX = 0.0;
    double currentMouseY = 0.0;
    glfwGetCursorPos(window, &currentMouseX, &currentMouseY);
    const double deltaX = currentMouseX - ts.mirrorHelperMoveStartMouseX;
    const double deltaY = currentMouseY - ts.mirrorHelperMoveStartMouseY;
    const float sensitivity = 0.01f;

    float deltaMove[3] = {0.0f, 0.0f, 0.0f};
    if (ts.mirrorHelperMoveConstrained && ts.mirrorHelperMoveAxis >= 0 && ts.mirrorHelperMoveAxis < 3) {
        const float dragAmount = constrainedAxisDragAmount(ts.mirrorHelperMoveAxis, deltaX, deltaY, sensitivity);
        deltaMove[ts.mirrorHelperMoveAxis] = dragAmount;
    } else {
        float forward[3] = { cameraTarget[0] - cameraPos[0],
                             cameraTarget[1] - cameraPos[1],
                             cameraTarget[2] - cameraPos[2] };
        const float fLen = std::sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
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
            const float rLen = std::sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
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

    for (int axis = 0; axis < 3; ++axis) {
        helperShape.center[axis] = ts.mirrorHelperMoveStartShapeCenter[axis] + deltaMove[axis];
        helperShape.mirrorOffset[axis] = ts.mirrorHelperMoveStartOffset[axis] + deltaMove[axis];
    }

    if (!ts.mirrorHelperMoveConstrained && !ImGui::GetIO().WantCaptureKeyboard) {
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
            ts.mirrorHelperMoveConstrained = true;
            ts.mirrorHelperMoveAxis = 0;
            glfwGetCursorPos(window, &ts.mirrorHelperMoveStartMouseX, &ts.mirrorHelperMoveStartMouseY);
            for (int axis = 0; axis < 3; ++axis) {
                ts.mirrorHelperMoveStartOffset[axis] = helperShape.mirrorOffset[axis];
                ts.mirrorHelperMoveStartShapeCenter[axis] = helperShape.center[axis];
            }
        } else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
            ts.mirrorHelperMoveConstrained = true;
            ts.mirrorHelperMoveAxis = 1;
            glfwGetCursorPos(window, &ts.mirrorHelperMoveStartMouseX, &ts.mirrorHelperMoveStartMouseY);
            for (int axis = 0; axis < 3; ++axis) {
                ts.mirrorHelperMoveStartOffset[axis] = helperShape.mirrorOffset[axis];
                ts.mirrorHelperMoveStartShapeCenter[axis] = helperShape.center[axis];
            }
        } else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
            ts.mirrorHelperMoveConstrained = true;
            ts.mirrorHelperMoveAxis = 2;
            glfwGetCursorPos(window, &ts.mirrorHelperMoveStartMouseX, &ts.mirrorHelperMoveStartMouseY);
            for (int axis = 0; axis < 3; ++axis) {
                ts.mirrorHelperMoveStartOffset[axis] = helperShape.mirrorOffset[axis];
                ts.mirrorHelperMoveStartShapeCenter[axis] = helperShape.center[axis];
            }
        }
    }

    if (isViewportClickAllowedForTransform(window, viewportPos, viewportSize)) {
        clearMirrorHelperMoveState();
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        for (int axis = 0; axis < 3; ++axis) {
            helperShape.center[axis] = ts.mirrorHelperMoveStartShapeCenter[axis];
            helperShape.mirrorOffset[axis] = ts.mirrorHelperMoveStartOffset[axis];
        }
        clearMirrorHelperMoveState();
    }
}

} // namespace rmt
