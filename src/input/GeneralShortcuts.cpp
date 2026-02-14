#include "rmt/input/Input.h"

#include <GLFW/glfw3.h>

#include <algorithm>

namespace rmt {

void InputManager::processGeneralInput(GLFWwindow* window,
                                       std::vector<Shape>& shapes,
                                       std::vector<int>& selectedShapes,
                                       float cameraTarget[3],
                                       bool& showGUI,
                                       bool& altRenderMode,
                                       TransformationState& ts) {
    static bool duplicationKeyHandled = false;
    static bool deleteKeyHandled = false;
    if (!ImGui::GetIO().WantCaptureKeyboard) {
        if ((glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) &&
            (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
             glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) &&
            !duplicationKeyHandled) {
            duplicationKeyHandled = true;
            std::vector<int> newSelection;
            for (int idx : selectedShapes) {
                if (idx >= 0 && idx < static_cast<int>(shapes.size())) {
                    Shape duplicateShape = shapes[idx];
                    duplicateShape.name += " Cp";
                    duplicateShape.scale[0] = shapes[idx].scale[0];
                    duplicateShape.scale[1] = shapes[idx].scale[1];
                    duplicateShape.scale[2] = shapes[idx].scale[2];
                    shapes.push_back(duplicateShape);
                    newSelection.push_back(static_cast<int>(shapes.size()) - 1);
                }
            }
            selectedShapes = newSelection;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE) {
            duplicationKeyHandled = false;
        }
    }

    if (!ImGui::GetIO().WantCaptureKeyboard && ts.sunLightEnabled && ts.sunLightSelected &&
        !ts.translationModeActive && !ts.rotationModeActive && !ts.scaleModeActive &&
        !ts.mirrorHelperMoveModeActive && !ts.sunLightMoveModeActive && !ts.sunLightHandleDragActive &&
        !ts.pointLightMoveModeActive &&
        glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS && !deleteKeyHandled) {
        deleteKeyHandled = true;
        ts.sunLightEnabled = false;
        ts.sunLightSelected = false;
        ts.sunLightMoveModeActive = false;
        ts.sunLightMoveConstrained = false;
        ts.sunLightMoveAxis = -1;
        ts.sunLightHandleDragActive = false;
    }

    if (!ImGui::GetIO().WantCaptureKeyboard &&
        ts.selectedPointLightIndex >= 0 &&
        ts.selectedPointLightIndex < static_cast<int>(ts.pointLights.size()) &&
        !ts.translationModeActive && !ts.rotationModeActive && !ts.scaleModeActive &&
        !ts.mirrorHelperMoveModeActive && !ts.sunLightMoveModeActive && !ts.sunLightHandleDragActive &&
        !ts.pointLightMoveModeActive &&
        glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS && !deleteKeyHandled) {
        deleteKeyHandled = true;
        ts.pointLights.erase(ts.pointLights.begin() + ts.selectedPointLightIndex);
        if (ts.pointLights.empty()) {
            ts.selectedPointLightIndex = -1;
        } else {
            ts.selectedPointLightIndex = std::min(ts.selectedPointLightIndex,
                                                  static_cast<int>(ts.pointLights.size()) - 1);
        }
        ts.pointLightMoveModeActive = false;
        ts.pointLightMoveConstrained = false;
        ts.pointLightMoveAxis = -1;
    }

    if (!ImGui::GetIO().WantCaptureKeyboard && !selectedShapes.empty()) {
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS && !ts.translationModeActive &&
            !ts.rotationModeActive && !ts.scaleModeActive && !ts.mirrorHelperMoveModeActive &&
            !ts.sunLightMoveModeActive && !ts.sunLightHandleDragActive && !ts.pointLightMoveModeActive &&
            !deleteKeyHandled) {
            deleteKeyHandled = true;
            std::sort(selectedShapes.begin(), selectedShapes.end(), std::greater<int>());
            for (int idx : selectedShapes) {
                if (idx >= 0 && idx < static_cast<int>(shapes.size())) {
                    shapes.erase(shapes.begin() + idx);
                }
            }
            selectedShapes.clear();
        }
    }

    if (!ImGui::GetIO().WantCaptureKeyboard) {
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
            if (!selectedShapes.empty()) {
                int lastIdx = selectedShapes.back();
                cameraTarget[0] = shapes[lastIdx].center[0];
                cameraTarget[1] = shapes[lastIdx].center[1];
                cameraTarget[2] = shapes[lastIdx].center[2];
            } else {
                cameraTarget[0] = cameraTarget[1] = cameraTarget[2] = 0.0f;
            }
        }
    }

    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_RELEASE) {
        deleteKeyHandled = false;
    }

    static bool hKeyPressed = false;
    const bool hDown = glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS;
    const bool textInputActive = showGUI && ImGui::GetIO().WantTextInput;
    if (hDown) {
        if (!hKeyPressed && !textInputActive) {
            showGUI = !showGUI;
            altRenderMode = !altRenderMode;
            hKeyPressed = true;
        }
    } else {
        hKeyPressed = false;
    }
}

} // namespace rmt
