#include "rmt/input/Input.h"

#include <GLFW/glfw3.h>

namespace rmt {

namespace {

void setCartesianAxis(int axis, float out[3]) {
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
    if (axis >= 0 && axis < 3) {
        out[axis] = 1.0f;
    }
}

} // namespace

void InputManager::processTransformationModeActivation(GLFWwindow* window,
                                                       std::vector<Shape>& shapes,
                                                       std::vector<int>& selectedShapes,
                                                       TransformationState& ts) {
    auto clearMirrorHelperSelection = [&]() {
        ts.mirrorHelperSelected = false;
        ts.mirrorHelperShapeIndex = -1;
        ts.mirrorHelperAxis = -1;
        ts.mirrorHelperMoveModeActive = false;
        ts.mirrorHelperMoveConstrained = false;
        ts.mirrorHelperMoveAxis = -1;
    };

    auto clearSunLightSelection = [&]() {
        ts.sunLightSelected = false;
        ts.sunLightMoveModeActive = false;
        ts.sunLightMoveConstrained = false;
        ts.sunLightMoveAxis = -1;
        ts.sunLightHandleDragActive = false;
    };

    auto clearPointLightSelection = [&]() {
        ts.selectedPointLightIndex = -1;
        ts.pointLightMoveModeActive = false;
        ts.pointLightMoveConstrained = false;
        ts.pointLightMoveAxis = -1;
    };

    auto mirrorHelperSelectionValid = [&]() -> bool {
        if (!ts.mirrorHelperSelected) {
            return false;
        }
        if (ts.mirrorHelperShapeIndex < 0 || ts.mirrorHelperShapeIndex >= static_cast<int>(shapes.size())) {
            return false;
        }
        if (selectedShapes.empty() || selectedShapes.front() != ts.mirrorHelperShapeIndex) {
            return false;
        }
        const Shape& helperShape = shapes[static_cast<std::size_t>(ts.mirrorHelperShapeIndex)];
        if (!helperShape.mirrorModifierEnabled) {
            return false;
        }
        if (!helperShape.mirrorAxis[0] && !helperShape.mirrorAxis[1] && !helperShape.mirrorAxis[2]) {
            return false;
        }
        if (ts.mirrorHelperAxis < 0 || ts.mirrorHelperAxis > 2 || !helperShape.mirrorAxis[ts.mirrorHelperAxis]) {
            ts.mirrorHelperAxis = -1;
            for (int axis = 0; axis < 3; ++axis) {
                if (helperShape.mirrorAxis[axis]) {
                    ts.mirrorHelperAxis = axis;
                    break;
                }
            }
        }
        return ts.mirrorHelperAxis >= 0;
    };

    if (ts.mirrorHelperSelected && !mirrorHelperSelectionValid()) {
        clearMirrorHelperSelection();
    }

    if (!ts.translationModeActive && !ts.rotationModeActive && !ts.scaleModeActive &&
        !ts.mirrorHelperMoveModeActive && !ts.sunLightMoveModeActive &&
        !ts.sunLightHandleDragActive && !ts.pointLightMoveModeActive &&
        !ImGui::GetIO().WantCaptureKeyboard) {
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && !ts.translationKeyHandled) {
            ts.translationKeyHandled = true;
            bool startedMirrorHelperMove = false;
            bool startedSunLightMove = false;
            bool startedPointLightMove = false;
            if (mirrorHelperSelectionValid()) {
                const Shape& helperShape = shapes[static_cast<std::size_t>(ts.mirrorHelperShapeIndex)];
                ts.mirrorHelperMoveModeActive = true;
                ts.mirrorHelperMoveConstrained = false;
                ts.mirrorHelperMoveAxis = -1;
                glfwGetCursorPos(window, &ts.mirrorHelperMoveStartMouseX, &ts.mirrorHelperMoveStartMouseY);
                for (int axis = 0; axis < 3; ++axis) {
                    ts.mirrorHelperMoveStartOffset[axis] = helperShape.mirrorOffset[axis];
                    ts.mirrorHelperMoveStartShapeCenter[axis] = helperShape.center[axis];
                }
                ts.showAxisGuides = false;
                ts.activeAxis = -1;
                startedMirrorHelperMove = true;
            }

            if (!startedMirrorHelperMove && ts.sunLightEnabled && ts.sunLightSelected) {
                ts.sunLightMoveModeActive = true;
                ts.sunLightMoveConstrained = false;
                ts.sunLightMoveAxis = -1;
                glfwGetCursorPos(window, &ts.sunLightMoveStartMouseX, &ts.sunLightMoveStartMouseY);
                ts.sunLightMoveStartPosition[0] = ts.sunLightPosition[0];
                ts.sunLightMoveStartPosition[1] = ts.sunLightPosition[1];
                ts.sunLightMoveStartPosition[2] = ts.sunLightPosition[2];
                ts.showAxisGuides = false;
                ts.activeAxis = -1;
                startedSunLightMove = true;
            }

            if (!startedMirrorHelperMove && !startedSunLightMove &&
                ts.selectedPointLightIndex >= 0 &&
                ts.selectedPointLightIndex < static_cast<int>(ts.pointLights.size())) {
                ts.pointLightMoveModeActive = true;
                ts.pointLightMoveConstrained = false;
                ts.pointLightMoveAxis = -1;
                glfwGetCursorPos(window, &ts.pointLightMoveStartMouseX, &ts.pointLightMoveStartMouseY);
                const PointLightState& pointLight = ts.pointLights[static_cast<std::size_t>(ts.selectedPointLightIndex)];
                ts.pointLightMoveStartPosition[0] = pointLight.position[0];
                ts.pointLightMoveStartPosition[1] = pointLight.position[1];
                ts.pointLightMoveStartPosition[2] = pointLight.position[2];
                ts.showAxisGuides = false;
                ts.activeAxis = -1;
                startedPointLightMove = true;
            }

            if (!startedMirrorHelperMove && !startedSunLightMove && !startedPointLightMove && !selectedShapes.empty()) {
                ts.translationModeActive = true;
                ts.translationConstrained = false;
                ts.translationAxis = -1;
                ts.showAxisGuides = true;
                ts.activeAxis = -1;
                setCartesianAxis(0, ts.guideAxisDirection);

                float centerX = 0.0f;
                float centerY = 0.0f;
                float centerZ = 0.0f;
                for (int idx : selectedShapes) {
                    centerX += shapes[idx].center[0];
                    centerY += shapes[idx].center[1];
                    centerZ += shapes[idx].center[2];
                }
                ts.guideCenter[0] = centerX / selectedShapes.size();
                ts.guideCenter[1] = centerY / selectedShapes.size();
                ts.guideCenter[2] = centerZ / selectedShapes.size();

                glfwGetCursorPos(window, &ts.translationStartMouseX, &ts.translationStartMouseY);
                ts.translationOriginalCenters.clear();
                for (int idx : selectedShapes) {
                    std::array<float, 3> center = {
                        shapes[idx].center[0],
                        shapes[idx].center[1],
                        shapes[idx].center[2],
                    };
                    ts.translationOriginalCenters.push_back(center);
                }
            }
        }

        if (!selectedShapes.empty() && glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS &&
            !ts.rotationKeyHandled) {
            clearSunLightSelection();
            clearPointLightSelection();
            ts.rotationModeActive = true;
            ts.rotationConstrained = false;
            ts.rotationAxis = -1;
            ts.rotationKeyHandled = true;
            ts.showAxisGuides = true;
            ts.activeAxis = -1;
            setCartesianAxis(0, ts.guideAxisDirection);
            glfwGetCursorPos(window, &ts.rotationStartMouseX, &ts.rotationStartMouseY);

            float centerX = 0.0f;
            float centerY = 0.0f;
            float centerZ = 0.0f;
            for (int idx : selectedShapes) {
                centerX += shapes[idx].center[0];
                centerY += shapes[idx].center[1];
                centerZ += shapes[idx].center[2];
            }
            ts.rotationCenter[0] = centerX / selectedShapes.size();
            ts.rotationCenter[1] = centerY / selectedShapes.size();
            ts.rotationCenter[2] = centerZ / selectedShapes.size();

            ts.guideCenter[0] = ts.rotationCenter[0];
            ts.guideCenter[1] = ts.rotationCenter[1];
            ts.guideCenter[2] = ts.rotationCenter[2];

            ts.rotationOriginalRotations.clear();
            ts.rotationOriginalCenters.clear();
            for (int idx : selectedShapes) {
                std::array<float, 3> rot = {
                    shapes[idx].rotation[0],
                    shapes[idx].rotation[1],
                    shapes[idx].rotation[2],
                };
                std::array<float, 3> center = {
                    shapes[idx].center[0],
                    shapes[idx].center[1],
                    shapes[idx].center[2],
                };
                ts.rotationOriginalRotations.push_back(rot);
                ts.rotationOriginalCenters.push_back(center);
            }
        }

        if (!selectedShapes.empty() && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && !ts.scaleKeyHandled) {
            clearSunLightSelection();
            clearPointLightSelection();
            ts.scaleModeActive = true;
            ts.scaleConstrained = false;
            ts.scaleAxis = -1;
            ts.scaleKeyHandled = true;
            ts.showAxisGuides = true;
            ts.activeAxis = -1;
            setCartesianAxis(0, ts.guideAxisDirection);

            float centerX = 0.0f;
            float centerY = 0.0f;
            float centerZ = 0.0f;
            for (int idx : selectedShapes) {
                centerX += shapes[idx].center[0];
                centerY += shapes[idx].center[1];
                centerZ += shapes[idx].center[2];
            }
            ts.guideCenter[0] = centerX / selectedShapes.size();
            ts.guideCenter[1] = centerY / selectedShapes.size();
            ts.guideCenter[2] = centerZ / selectedShapes.size();

            glfwGetCursorPos(window, &ts.scaleStartMouseX, &ts.scaleStartMouseY);
            ts.scaleOriginalScales.clear();
            ts.scaleOriginalElongations.clear();
            for (int idx : selectedShapes) {
                std::array<float, 3> scale = {
                    shapes[idx].scale[0],
                    shapes[idx].scale[1],
                    shapes[idx].scale[2],
                };
                std::array<float, 3> elongation = {
                    shapes[idx].elongation[0],
                    shapes[idx].elongation[1],
                    shapes[idx].elongation[2],
                };
                ts.scaleOriginalScales.push_back(scale);
                ts.scaleOriginalElongations.push_back(elongation);
            }
        }
    }

    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE) {
        ts.translationKeyHandled = false;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
        ts.rotationKeyHandled = false;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE) {
        ts.scaleKeyHandled = false;
    }
}

} // namespace rmt
