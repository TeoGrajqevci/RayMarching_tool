#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Input.h"
#include "ImGuizmo.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include "Texture.h"
#include <cmath>
#include <limits>

extern float camDistance;
extern Texture hdrTexture;
extern bool hdrLoaded;

// Fonctions utilitaires pour la rotation 3D
void rotateAroundAxis(float point[3], float center[3], float axis[3], float angle) {
    // Translate point to origin
    float translated[3] = {
        point[0] - center[0],
        point[1] - center[1],
        point[2] - center[2]
    };
    
    // Rotation matrix using Rodrigues' formula
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);
    float oneMinusCosA = 1.0f - cosA;
    
    // Normalize axis
    float axisLength = std::sqrt(axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2]);
    if (axisLength < 1e-6f) {
        return;
    }
    float normalizedAxis[3] = {axis[0]/axisLength, axis[1]/axisLength, axis[2]/axisLength};
    
    float rotated[3];
    rotated[0] = translated[0] * (cosA + normalizedAxis[0]*normalizedAxis[0]*oneMinusCosA) +
                 translated[1] * (normalizedAxis[0]*normalizedAxis[1]*oneMinusCosA - normalizedAxis[2]*sinA) +
                 translated[2] * (normalizedAxis[0]*normalizedAxis[2]*oneMinusCosA + normalizedAxis[1]*sinA);
    
    rotated[1] = translated[0] * (normalizedAxis[1]*normalizedAxis[0]*oneMinusCosA + normalizedAxis[2]*sinA) +
                 translated[1] * (cosA + normalizedAxis[1]*normalizedAxis[1]*oneMinusCosA) +
                 translated[2] * (normalizedAxis[1]*normalizedAxis[2]*oneMinusCosA - normalizedAxis[0]*sinA);
    
    rotated[2] = translated[0] * (normalizedAxis[2]*normalizedAxis[0]*oneMinusCosA - normalizedAxis[1]*sinA) +
                 translated[1] * (normalizedAxis[2]*normalizedAxis[1]*oneMinusCosA + normalizedAxis[0]*sinA) +
                 translated[2] * (cosA + normalizedAxis[2]*normalizedAxis[2]*oneMinusCosA);
    
    // Translate back
    point[0] = rotated[0] + center[0];
    point[1] = rotated[1] + center[1];
    point[2] = rotated[2] + center[2];
}

void setCartesianAxis(int axis, float out[3]) {
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
    if (axis >= 0 && axis < 3) {
        out[axis] = 1.0f;
    }
}

void normalize3(float v[3]) {
    const float len = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len > 1e-6f) {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
}

void rotateX(const float in[3], float angle, float out[3]) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    out[0] = in[0];
    out[1] = c * in[1] - s * in[2];
    out[2] = s * in[1] + c * in[2];
}

void rotateY(const float in[3], float angle, float out[3]) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    out[0] = c * in[0] + s * in[2];
    out[1] = in[1];
    out[2] = -s * in[0] + c * in[2];
}

void rotateZ(const float in[3], float angle, float out[3]) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    out[0] = c * in[0] - s * in[1];
    out[1] = s * in[0] + c * in[1];
    out[2] = in[2];
}

void rotateWorld(const float in[3], const float angles[3], float out[3]) {
    float tmp[3] = { in[0], in[1], in[2] };
    float next[3];
    if (angles[0] != 0.0f) {
        rotateX(tmp, angles[0], next);
        tmp[0] = next[0]; tmp[1] = next[1]; tmp[2] = next[2];
    }
    if (angles[1] != 0.0f) {
        rotateY(tmp, angles[1], next);
        tmp[0] = next[0]; tmp[1] = next[1]; tmp[2] = next[2];
    }
    if (angles[2] != 0.0f) {
        rotateZ(tmp, angles[2], next);
        tmp[0] = next[0]; tmp[1] = next[1]; tmp[2] = next[2];
    }
    out[0] = tmp[0];
    out[1] = tmp[1];
    out[2] = tmp[2];
}

void getLocalAxisWorldDirection(const float rotation[3], int axis, float out[3]) {
    float localAxis[3] = { 0.0f, 0.0f, 0.0f };
    if (axis < 0 || axis > 2) {
        out[0] = 1.0f;
        out[1] = 0.0f;
        out[2] = 0.0f;
        return;
    }
    localAxis[axis] = 1.0f;
    rotateWorld(localAxis, rotation, out);
    normalize3(out);
}

void updateGuideAxisDirection(TransformationState& ts, const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes, int axis) {
    setCartesianAxis(axis, ts.guideAxisDirection);
    if (!ts.useLocalSpace || selectedShapes.empty() || axis < 0 || axis > 2) {
        return;
    }
    const int shapeIndex = selectedShapes.front();
    if (shapeIndex < 0 || shapeIndex >= static_cast<int>(shapes.size())) {
        return;
    }
    getLocalAxisWorldDirection(shapes[shapeIndex].rotation, axis, ts.guideAxisDirection);
}

float constrainedAxisDragAmount(int axis, double deltaX, double deltaY, float sensitivity) {
    if (axis == 1) {
        return -static_cast<float>(deltaY) * sensitivity;
    }
    return static_cast<float>(deltaX) * sensitivity;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
} 

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camDistance -= static_cast<float>(yoffset) * 0.5f;
    if (camDistance < 1.0f)
        camDistance = 1.0f;
    if (camDistance > 20.0f)
        camDistance = 20.0f;
}

InputManager::InputManager()
{
}

InputManager::~InputManager()
{
}

void InputManager::processGeneralInput(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes, float cameraTarget[3], bool& showGUI, bool& altRenderMode, const TransformationState& ts)
{
    static bool duplicationKeyHandled = false;
    if (!ImGui::GetIO().WantCaptureKeyboard) {
        if ((glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) &&
            (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) &&
            !duplicationKeyHandled)
        {
            duplicationKeyHandled = true;
            std::vector<int> newSelection;
            for (int idx : selectedShapes) {
                if (idx >= 0 && idx < static_cast<int>(shapes.size())) {
                    Shape duplicateShape = shapes[idx];
                    duplicateShape.name += " Cp";
                    // S'assurer que les échelles sont copiées correctement
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
    if (!ImGui::GetIO().WantCaptureKeyboard && !selectedShapes.empty())
    {
        // Only allow deletion if no transformation mode is active
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS && 
            !ts.translationModeActive && !ts.rotationModeActive && !ts.scaleModeActive &&
            !ts.mirrorHelperMoveModeActive)
        {
            std::sort(selectedShapes.begin(), selectedShapes.end(), std::greater<int>());
            for (int idx : selectedShapes) {
                if (idx >= 0 && idx < static_cast<int>(shapes.size()))
                    shapes.erase(shapes.begin() + idx);
            }
            selectedShapes.clear();
        }
    }
    if (!ImGui::GetIO().WantCaptureKeyboard)
    {
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        {
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
    static bool hKeyPressed = false;
    const bool hDown = glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS;
    const bool textInputActive = showGUI && ImGui::GetIO().WantTextInput;
    if (hDown)
    {
        if (!hKeyPressed && !textInputActive)
        {
            showGUI = !showGUI;
            altRenderMode = !altRenderMode;
            hKeyPressed = true;
        }
    }
    else
    {
        hKeyPressed = false;
    }
}

void InputManager::processTransformationModeActivation(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes, TransformationState& ts)
{
    auto clearMirrorHelperSelection = [&]() {
        ts.mirrorHelperSelected = false;
        ts.mirrorHelperShapeIndex = -1;
        ts.mirrorHelperAxis = -1;
        ts.mirrorHelperMoveModeActive = false;
        ts.mirrorHelperMoveConstrained = false;
        ts.mirrorHelperMoveAxis = -1;
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
        !ts.mirrorHelperMoveModeActive && !ImGui::GetIO().WantCaptureKeyboard)
    {
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && !ts.translationKeyHandled)
        {
            ts.translationKeyHandled = true;
            bool startedMirrorHelperMove = false;
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

            if (!startedMirrorHelperMove && !selectedShapes.empty()) {
                ts.translationModeActive = true;
                ts.translationConstrained = false;
                ts.translationAxis = -1;
                ts.showAxisGuides = true; // Activer les guides d'axe
                ts.activeAxis = -1; // Pas d'axe spécifique encore
                setCartesianAxis(0, ts.guideAxisDirection);
                
                // Calculer le centre des formes sélectionnées pour les guides
                float centerX = 0.0f, centerY = 0.0f, centerZ = 0.0f;
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
                    std::array<float, 3> center = { shapes[idx].center[0], shapes[idx].center[1], shapes[idx].center[2] };
                    ts.translationOriginalCenters.push_back(center);
                }
            }
        }
        if (!selectedShapes.empty() && glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !ts.rotationKeyHandled)
        {
            ts.rotationModeActive = true;
            ts.rotationConstrained = false;
            ts.rotationAxis = -1;
            ts.rotationKeyHandled = true;
            ts.showAxisGuides = true; // Activer les guides d'axe pour la rotation
            ts.activeAxis = -1; // Pas d'axe spécifique encore
            setCartesianAxis(0, ts.guideAxisDirection);
            glfwGetCursorPos(window, &ts.rotationStartMouseX, &ts.rotationStartMouseY);
            
            // Calculer le centre de rotation (centre des formes sélectionnées)
            float centerX = 0.0f, centerY = 0.0f, centerZ = 0.0f;
            for (int idx : selectedShapes) {
                centerX += shapes[idx].center[0];
                centerY += shapes[idx].center[1];
                centerZ += shapes[idx].center[2];
            }
            ts.rotationCenter[0] = centerX / selectedShapes.size();
            ts.rotationCenter[1] = centerY / selectedShapes.size();
            ts.rotationCenter[2] = centerZ / selectedShapes.size();
            
            // Utiliser le centre de rotation comme centre des guides
            ts.guideCenter[0] = ts.rotationCenter[0];
            ts.guideCenter[1] = ts.rotationCenter[1];
            ts.guideCenter[2] = ts.rotationCenter[2];
            
            // Sauvegarder les rotations et centres originaux
            ts.rotationOriginalRotations.clear();
            ts.rotationOriginalCenters.clear();
            for (int idx : selectedShapes) {
                std::array<float, 3> rot = { shapes[idx].rotation[0], shapes[idx].rotation[1], shapes[idx].rotation[2] };
                std::array<float, 3> center = { shapes[idx].center[0], shapes[idx].center[1], shapes[idx].center[2] };
                ts.rotationOriginalRotations.push_back(rot);
                ts.rotationOriginalCenters.push_back(center);
            }
        }
        if (!selectedShapes.empty() && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && !ts.scaleKeyHandled)
        {
            ts.scaleModeActive = true;
            ts.scaleConstrained = false;
            ts.scaleAxis = -1;
            ts.scaleKeyHandled = true;
            ts.showAxisGuides = true; // Activer les guides d'axe pour la mise à l'échelle
            ts.activeAxis = -1; // Pas d'axe spécifique encore
            setCartesianAxis(0, ts.guideAxisDirection);
            
            // Calculer le centre des formes sélectionnées pour les guides
            float centerX = 0.0f, centerY = 0.0f, centerZ = 0.0f;
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
                std::array<float, 3> scale = { shapes[idx].scale[0], shapes[idx].scale[1], shapes[idx].scale[2] };
                std::array<float, 3> elongation = { shapes[idx].elongation[0], shapes[idx].elongation[1], shapes[idx].elongation[2] };
                ts.scaleOriginalScales.push_back(scale);
                ts.scaleOriginalElongations.push_back(elongation);
            }
        }
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE)
        ts.translationKeyHandled = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE)
        ts.rotationKeyHandled = false;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE)
        ts.scaleKeyHandled = false;
}

void InputManager::processTransformationUpdates(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes,
                                                  TransformationState& ts, const float cameraPos[3], const float cameraTarget[3],
                                                  const ImVec2& viewportPos, const ImVec2& viewportSize)
{
    auto isViewportClickAllowed = [&]() {
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
        const bool mouseInViewport =
            mouseX >= viewportX && mouseX <= (viewportX + viewportW) &&
            mouseY >= viewportY && mouseY <= (viewportY + viewportH);
        const bool blockByUiPanel = ImGui::GetIO().WantCaptureMouse && !mouseInViewport;

        return !blockByUiPanel && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    };

    auto clearMirrorHelperMoveState = [&]() {
        ts.mirrorHelperMoveModeActive = false;
        ts.mirrorHelperMoveConstrained = false;
        ts.mirrorHelperMoveAxis = -1;
    };

    if (ts.mirrorHelperMoveModeActive)
    {
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
        } else {
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
                float fLen = std::sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
                if (fLen > 1e-6f) {
                    forward[0] /= fLen; forward[1] /= fLen; forward[2] /= fLen;
                    float upDefault[3] = {0.0f, 1.0f, 0.0f};
                    float right[3] = {
                        forward[1] * upDefault[2] - forward[2] * upDefault[1],
                        forward[2] * upDefault[0] - forward[0] * upDefault[2],
                        forward[0] * upDefault[1] - forward[1] * upDefault[0]
                    };
                    float rLen = std::sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
                    if (rLen > 1e-6f) {
                        right[0] /= rLen; right[1] /= rLen; right[2] /= rLen;
                        float up[3] = {
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

            if (!ts.mirrorHelperMoveConstrained && !ImGui::GetIO().WantCaptureKeyboard)
            {
                if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
                {
                    ts.mirrorHelperMoveConstrained = true;
                    ts.mirrorHelperMoveAxis = 0;
                    glfwGetCursorPos(window, &ts.mirrorHelperMoveStartMouseX, &ts.mirrorHelperMoveStartMouseY);
                    for (int axis = 0; axis < 3; ++axis) {
                        ts.mirrorHelperMoveStartOffset[axis] = helperShape.mirrorOffset[axis];
                        ts.mirrorHelperMoveStartShapeCenter[axis] = helperShape.center[axis];
                    }
                }
                else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
                {
                    ts.mirrorHelperMoveConstrained = true;
                    ts.mirrorHelperMoveAxis = 1;
                    glfwGetCursorPos(window, &ts.mirrorHelperMoveStartMouseX, &ts.mirrorHelperMoveStartMouseY);
                    for (int axis = 0; axis < 3; ++axis) {
                        ts.mirrorHelperMoveStartOffset[axis] = helperShape.mirrorOffset[axis];
                        ts.mirrorHelperMoveStartShapeCenter[axis] = helperShape.center[axis];
                    }
                }
                else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
                {
                    ts.mirrorHelperMoveConstrained = true;
                    ts.mirrorHelperMoveAxis = 2;
                    glfwGetCursorPos(window, &ts.mirrorHelperMoveStartMouseX, &ts.mirrorHelperMoveStartMouseY);
                    for (int axis = 0; axis < 3; ++axis) {
                        ts.mirrorHelperMoveStartOffset[axis] = helperShape.mirrorOffset[axis];
                        ts.mirrorHelperMoveStartShapeCenter[axis] = helperShape.center[axis];
                    }
                }
            }

            if (isViewportClickAllowed())
            {
                clearMirrorHelperMoveState();
            }
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            {
                for (int axis = 0; axis < 3; ++axis) {
                    helperShape.center[axis] = ts.mirrorHelperMoveStartShapeCenter[axis];
                    helperShape.mirrorOffset[axis] = ts.mirrorHelperMoveStartOffset[axis];
                }
                clearMirrorHelperMoveState();
            }
        }
    }

    // Translation mode update
    if (ts.translationModeActive && !selectedShapes.empty())
    {
        double currentMouseX, currentMouseY;
        glfwGetCursorPos(window, &currentMouseX, &currentMouseY);
        double deltaX = currentMouseX - ts.translationStartMouseX;
        double deltaY = currentMouseY - ts.translationStartMouseY;
        float sensitivity = 0.01f;
        if (ts.translationConstrained)
        {
            const float dragAmount = constrainedAxisDragAmount(ts.translationAxis, deltaX, deltaY, sensitivity);
            for (size_t i = 0; i < selectedShapes.size(); i++) {
                int idx = selectedShapes[i];
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
        }
        else
        {
            float forward[3] = { cameraTarget[0] - cameraPos[0],
                                 cameraTarget[1] - cameraPos[1],
                                 cameraTarget[2] - cameraPos[2] };
            float fLen = std::sqrt(forward[0]*forward[0] + forward[1]*forward[1] + forward[2]*forward[2]);
            forward[0] /= fLen; forward[1] /= fLen; forward[2] /= fLen;
            float upDefault[3] = { 0.0f, 1.0f, 0.0f };
            float right[3] = {
                forward[1]*upDefault[2] - forward[2]*upDefault[1],
                forward[2]*upDefault[0] - forward[0]*upDefault[2],
                forward[0]*upDefault[1] - forward[1]*upDefault[0]
            };
            float rLen = std::sqrt(right[0]*right[0] + right[1]*right[1] + right[2]*right[2]);
            right[0] /= rLen; right[1] /= rLen; right[2] /= rLen;
            float up[3] = {
                right[1]*forward[2] - right[2]*forward[1],
                right[2]*forward[0] - right[0]*forward[2],
                right[0]*forward[1] - right[1]*forward[0]
            };
            for (size_t i = 0; i < selectedShapes.size(); i++) {
                int idx = selectedShapes[i];
                shapes[idx].center[0] = ts.translationOriginalCenters[i][0] + static_cast<float>(deltaX)*sensitivity*right[0] - static_cast<float>(deltaY)*sensitivity*up[0];
                shapes[idx].center[1] = ts.translationOriginalCenters[i][1] + static_cast<float>(deltaX)*sensitivity*right[1] - static_cast<float>(deltaY)*sensitivity*up[1];
                shapes[idx].center[2] = ts.translationOriginalCenters[i][2] + static_cast<float>(deltaX)*sensitivity*right[2] - static_cast<float>(deltaY)*sensitivity*up[2];
            }
        }
        if (!ts.translationConstrained && !ImGui::GetIO().WantCaptureKeyboard)
        {
            if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
            {
                ts.translationConstrained = true;
                ts.translationAxis = 0;
                ts.activeAxis = 0; // Axe X actif pour les guides
                updateGuideAxisDirection(ts, shapes, selectedShapes, ts.translationAxis);
                glfwGetCursorPos(window, &ts.translationStartMouseX, &ts.translationStartMouseY);
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.translationOriginalCenters[i][0] = shapes[idx].center[0];
                    ts.translationOriginalCenters[i][1] = shapes[idx].center[1];
                    ts.translationOriginalCenters[i][2] = shapes[idx].center[2];
                }
            }
            else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
            {
                ts.translationConstrained = true;
                ts.translationAxis = 1;
                ts.activeAxis = 1; // Axe Y actif pour les guides
                updateGuideAxisDirection(ts, shapes, selectedShapes, ts.translationAxis);
                glfwGetCursorPos(window, &ts.translationStartMouseX, &ts.translationStartMouseY);
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.translationOriginalCenters[i][0] = shapes[idx].center[0];
                    ts.translationOriginalCenters[i][1] = shapes[idx].center[1];
                    ts.translationOriginalCenters[i][2] = shapes[idx].center[2];
                }
            }
            else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
            {
                ts.translationConstrained = true;
                ts.translationAxis = 2;
                ts.activeAxis = 2; // Axe Z actif pour les guides
                updateGuideAxisDirection(ts, shapes, selectedShapes, ts.translationAxis);
                glfwGetCursorPos(window, &ts.translationStartMouseX, &ts.translationStartMouseY);
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.translationOriginalCenters[i][0] = shapes[idx].center[0];
                    ts.translationOriginalCenters[i][1] = shapes[idx].center[1];
                    ts.translationOriginalCenters[i][2] = shapes[idx].center[2];
                }
            }
        }
        if (isViewportClickAllowed())
        {
            ts.translationModeActive = false;
            ts.translationConstrained = false;
            ts.showAxisGuides = false; // Désactiver les guides
            ts.activeAxis = -1;
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            for (size_t i = 0; i < selectedShapes.size(); i++) {
                int idx = selectedShapes[i];
                shapes[idx].center[0] = ts.translationOriginalCenters[i][0];
                shapes[idx].center[1] = ts.translationOriginalCenters[i][1];
                shapes[idx].center[2] = ts.translationOriginalCenters[i][2];
            }
            ts.translationModeActive = false;
            ts.translationConstrained = false;
            ts.showAxisGuides = false; // Désactiver les guides
            ts.activeAxis = -1;
        }
    }
    // Rotation mode update
    if (ts.rotationModeActive && !selectedShapes.empty())
    {
        double currentMouseX, currentMouseY;
        glfwGetCursorPos(window, &currentMouseX, &currentMouseY);
        double deltaX = currentMouseX - ts.rotationStartMouseX;
        double deltaY = currentMouseY - ts.rotationStartMouseY;
        float sensitivity = 0.005f;
        
        if (ts.rotationConstrained)
        {
            // Rotation contrainte autour d'un axe spécifique
            float axis[3] = {0.0f, 0.0f, 0.0f};
            float angle = static_cast<float>(deltaX) * sensitivity;
            if (ts.useLocalSpace && ts.rotationAxis >= 0 && ts.rotationAxis < 3) {
                axis[0] = ts.guideAxisDirection[0];
                axis[1] = ts.guideAxisDirection[1];
                axis[2] = ts.guideAxisDirection[2];
                normalize3(axis);
            } else {
                setCartesianAxis(ts.rotationAxis, axis);
            }
            
            for (size_t i = 0; i < selectedShapes.size(); i++) {
                int idx = selectedShapes[i];
                
                // Restaurer position originale
                shapes[idx].center[0] = ts.rotationOriginalCenters[i][0];
                shapes[idx].center[1] = ts.rotationOriginalCenters[i][1];
                shapes[idx].center[2] = ts.rotationOriginalCenters[i][2];
                
                // Restaurer rotation originale
                shapes[idx].rotation[0] = ts.rotationOriginalRotations[i][0];
                shapes[idx].rotation[1] = ts.rotationOriginalRotations[i][1];
                shapes[idx].rotation[2] = ts.rotationOriginalRotations[i][2];
                
                // Appliquer la rotation autour du centre de rotation mondial
                rotateAroundAxis(shapes[idx].center, ts.rotationCenter, axis, angle);
                
                // Pour une rotation d'espace monde, accumuler les rotations
                // en gardant les autres axes et en ajoutant la nouvelle rotation
                if (ts.rotationAxis == 0) {
                    shapes[idx].rotation[0] = ts.rotationOriginalRotations[i][0] + angle;  // Accumuler X
                    shapes[idx].rotation[1] = ts.rotationOriginalRotations[i][1]; // Garder Y
                    shapes[idx].rotation[2] = ts.rotationOriginalRotations[i][2]; // Garder Z
                } else if (ts.rotationAxis == 1) {
                    shapes[idx].rotation[0] = ts.rotationOriginalRotations[i][0]; // Garder X
                    shapes[idx].rotation[1] = ts.rotationOriginalRotations[i][1] + angle;  // Accumuler Y
                    shapes[idx].rotation[2] = ts.rotationOriginalRotations[i][2]; // Garder Z
                } else if (ts.rotationAxis == 2) {
                    shapes[idx].rotation[0] = ts.rotationOriginalRotations[i][0]; // Garder X
                    shapes[idx].rotation[1] = ts.rotationOriginalRotations[i][1]; // Garder Y
                    shapes[idx].rotation[2] = ts.rotationOriginalRotations[i][2] + angle;  // Accumuler Z
                }
            }
        }
        else
        {
            // Rotation libre (combinaison des axes X et Y)
            float angleX = static_cast<float>(deltaY) * sensitivity;
            float angleY = static_cast<float>(deltaX) * sensitivity;
            
            for (size_t i = 0; i < selectedShapes.size(); i++) {
                int idx = selectedShapes[i];
                
                // Restaurer position originale
                shapes[idx].center[0] = ts.rotationOriginalCenters[i][0];
                shapes[idx].center[1] = ts.rotationOriginalCenters[i][1];
                shapes[idx].center[2] = ts.rotationOriginalCenters[i][2];
                
                // Restaurer rotation originale
                shapes[idx].rotation[0] = ts.rotationOriginalRotations[i][0];
                shapes[idx].rotation[1] = ts.rotationOriginalRotations[i][1];
                shapes[idx].rotation[2] = ts.rotationOriginalRotations[i][2];
                
                // Appliquer rotation Y puis X
                float axisY[3] = {0.0f, 1.0f, 0.0f};
                float axisX[3] = {1.0f, 0.0f, 0.0f};
                rotateAroundAxis(shapes[idx].center, ts.rotationCenter, axisY, angleY);
                rotateAroundAxis(shapes[idx].center, ts.rotationCenter, axisX, angleX);
                
                // Pour une rotation d'espace monde, accumuler les rotations X et Y
                shapes[idx].rotation[0] = ts.rotationOriginalRotations[i][0] + angleX;  // Accumuler X
                shapes[idx].rotation[1] = ts.rotationOriginalRotations[i][1] + angleY;  // Accumuler Y
                shapes[idx].rotation[2] = ts.rotationOriginalRotations[i][2];     // Garder Z
            }
        }
        if (!ts.rotationConstrained && !ImGui::GetIO().WantCaptureKeyboard)
        {
            if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
            {
                ts.rotationConstrained = true;
                ts.rotationAxis = 0;
                ts.activeAxis = 0; // Axe X actif pour les guides
                updateGuideAxisDirection(ts, shapes, selectedShapes, ts.rotationAxis);
                glfwGetCursorPos(window, &ts.rotationStartMouseX, &ts.rotationStartMouseY);
                // Sauvegarder toutes les rotations, pas seulement l'axe concerné
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.rotationOriginalRotations[i][0] = shapes[idx].rotation[0];
                    ts.rotationOriginalRotations[i][1] = shapes[idx].rotation[1];
                    ts.rotationOriginalRotations[i][2] = shapes[idx].rotation[2];
                }
            }
            else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
            {
                ts.rotationConstrained = true;
                ts.rotationAxis = 1;
                ts.activeAxis = 1; // Axe Y actif pour les guides
                updateGuideAxisDirection(ts, shapes, selectedShapes, ts.rotationAxis);
                glfwGetCursorPos(window, &ts.rotationStartMouseX, &ts.rotationStartMouseY);
                // Sauvegarder toutes les rotations, pas seulement l'axe concerné
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.rotationOriginalRotations[i][0] = shapes[idx].rotation[0];
                    ts.rotationOriginalRotations[i][1] = shapes[idx].rotation[1];
                    ts.rotationOriginalRotations[i][2] = shapes[idx].rotation[2];
                }
            }
            else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
            {
                ts.rotationConstrained = true;
                ts.rotationAxis = 2;
                ts.activeAxis = 2; // Axe Z actif pour les guides
                updateGuideAxisDirection(ts, shapes, selectedShapes, ts.rotationAxis);
                glfwGetCursorPos(window, &ts.rotationStartMouseX, &ts.rotationStartMouseY);
                // Sauvegarder toutes les rotations, pas seulement l'axe concerné
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.rotationOriginalRotations[i][0] = shapes[idx].rotation[0];
                    ts.rotationOriginalRotations[i][1] = shapes[idx].rotation[1];
                    ts.rotationOriginalRotations[i][2] = shapes[idx].rotation[2];
                }
            }
        }
        if (isViewportClickAllowed())
        {
            ts.rotationModeActive = false;
            ts.rotationConstrained = false;
            ts.showAxisGuides = false; // Désactiver les guides
            ts.activeAxis = -1;
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            for (size_t i = 0; i < selectedShapes.size(); i++) {
                int idx = selectedShapes[i];
                shapes[idx].rotation[0] = ts.rotationOriginalRotations[i][0];
                shapes[idx].rotation[1] = ts.rotationOriginalRotations[i][1];
                shapes[idx].rotation[2] = ts.rotationOriginalRotations[i][2];
            }
            ts.rotationModeActive = false;
            ts.rotationConstrained = false;
            ts.showAxisGuides = false; // Désactiver les guides
            ts.activeAxis = -1;
        }
    }
    // Scale mode update
    if (ts.scaleModeActive && !selectedShapes.empty())
    {
        double currentMouseX, currentMouseY;
        glfwGetCursorPos(window, &currentMouseX, &currentMouseY);
        double deltaY = currentMouseY - ts.scaleStartMouseY;
        float deformSensitivity = 0.005f;
        float elongationSensitivity = 0.01f;
        float scaleFactor = std::max(0.01f, 1.0f + static_cast<float>(deltaY) * deformSensitivity);
        float elongationDelta = static_cast<float>(deltaY) * elongationSensitivity;

        for (size_t i = 0; i < selectedShapes.size(); i++) {
            int idx = selectedShapes[i];

            // Restore the full original transform first, then apply the current drag delta.
            shapes[idx].scale[0] = ts.scaleOriginalScales[i][0];
            shapes[idx].scale[1] = ts.scaleOriginalScales[i][1];
            shapes[idx].scale[2] = ts.scaleOriginalScales[i][2];

            shapes[idx].elongation[0] = ts.scaleOriginalElongations[i][0];
            shapes[idx].elongation[1] = ts.scaleOriginalElongations[i][1];
            shapes[idx].elongation[2] = ts.scaleOriginalElongations[i][2];

            if (shapes[idx].scaleMode == SCALE_MODE_ELONGATE) {
                if (ts.scaleConstrained && ts.scaleAxis >= 0 && ts.scaleAxis < 3) {
                    shapes[idx].elongation[ts.scaleAxis] =
                        ts.scaleOriginalElongations[i][ts.scaleAxis] + elongationDelta;
                } else {
                    shapes[idx].elongation[0] = ts.scaleOriginalElongations[i][0] + elongationDelta;
                    shapes[idx].elongation[1] = ts.scaleOriginalElongations[i][1] + elongationDelta;
                    shapes[idx].elongation[2] = ts.scaleOriginalElongations[i][2] + elongationDelta;
                }
            } else {
                if (ts.scaleConstrained && ts.scaleAxis >= 0 && ts.scaleAxis < 3) {
                    shapes[idx].scale[ts.scaleAxis] =
                        std::max(0.01f, ts.scaleOriginalScales[i][ts.scaleAxis] * scaleFactor);
                } else {
                    shapes[idx].scale[0] = std::max(0.01f, ts.scaleOriginalScales[i][0] * scaleFactor);
                    shapes[idx].scale[1] = std::max(0.01f, ts.scaleOriginalScales[i][1] * scaleFactor);
                    shapes[idx].scale[2] = std::max(0.01f, ts.scaleOriginalScales[i][2] * scaleFactor);
                }
            }
        }
        if (!ts.scaleConstrained && !ImGui::GetIO().WantCaptureKeyboard)
        {
            if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
            {
                ts.scaleConstrained = true;
                ts.scaleAxis = 0;
                ts.activeAxis = 0; // Axe X actif pour les guides
                updateGuideAxisDirection(ts, shapes, selectedShapes, ts.scaleAxis);
                glfwGetCursorPos(window, &ts.scaleStartMouseX, &ts.scaleStartMouseY);
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.scaleOriginalScales[i][0] = shapes[idx].scale[0];
                    ts.scaleOriginalScales[i][1] = shapes[idx].scale[1];
                    ts.scaleOriginalScales[i][2] = shapes[idx].scale[2];
                    ts.scaleOriginalElongations[i][0] = shapes[idx].elongation[0];
                    ts.scaleOriginalElongations[i][1] = shapes[idx].elongation[1];
                    ts.scaleOriginalElongations[i][2] = shapes[idx].elongation[2];
                }
            }
            else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
            {
                ts.scaleConstrained = true;
                ts.scaleAxis = 1;
                ts.activeAxis = 1; // Axe Y actif pour les guides
                updateGuideAxisDirection(ts, shapes, selectedShapes, ts.scaleAxis);
                glfwGetCursorPos(window, &ts.scaleStartMouseX, &ts.scaleStartMouseY);
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.scaleOriginalScales[i][0] = shapes[idx].scale[0];
                    ts.scaleOriginalScales[i][1] = shapes[idx].scale[1];
                    ts.scaleOriginalScales[i][2] = shapes[idx].scale[2];
                    ts.scaleOriginalElongations[i][0] = shapes[idx].elongation[0];
                    ts.scaleOriginalElongations[i][1] = shapes[idx].elongation[1];
                    ts.scaleOriginalElongations[i][2] = shapes[idx].elongation[2];
                }
            }
            else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
            {
                ts.scaleConstrained = true;
                ts.scaleAxis = 2;
                ts.activeAxis = 2; // Axe Z actif pour les guides
                updateGuideAxisDirection(ts, shapes, selectedShapes, ts.scaleAxis);
                glfwGetCursorPos(window, &ts.scaleStartMouseX, &ts.scaleStartMouseY);
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.scaleOriginalScales[i][0] = shapes[idx].scale[0];
                    ts.scaleOriginalScales[i][1] = shapes[idx].scale[1];
                    ts.scaleOriginalScales[i][2] = shapes[idx].scale[2];
                    ts.scaleOriginalElongations[i][0] = shapes[idx].elongation[0];
                    ts.scaleOriginalElongations[i][1] = shapes[idx].elongation[1];
                    ts.scaleOriginalElongations[i][2] = shapes[idx].elongation[2];
                }
            }
        }
        if (isViewportClickAllowed())
        {
            ts.scaleModeActive = false;
            ts.scaleConstrained = false;
            ts.showAxisGuides = false; // Désactiver les guides
            ts.activeAxis = -1;
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            for (size_t i = 0; i < selectedShapes.size(); i++) {
                int idx = selectedShapes[i];
                shapes[idx].scale[0] = ts.scaleOriginalScales[i][0];
                shapes[idx].scale[1] = ts.scaleOriginalScales[i][1];
                shapes[idx].scale[2] = ts.scaleOriginalScales[i][2];
                shapes[idx].elongation[0] = ts.scaleOriginalElongations[i][0];
                shapes[idx].elongation[1] = ts.scaleOriginalElongations[i][1];
                shapes[idx].elongation[2] = ts.scaleOriginalElongations[i][2];
            }
            ts.scaleModeActive = false;
            ts.scaleConstrained = false;
            ts.showAxisGuides = false; // Désactiver les guides
            ts.activeAxis = -1;
        }
    }
}

void InputManager::processMousePickingAndCameraDrag(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes,
                                                      float cameraPos[3], float cameraTarget[3],
                                                      float& camTheta, float& camPhi,
                                                      bool& cameraDragging, double& lastMouseX, double& lastMouseY,
                                                      bool& mouseWasPressed,
                                                      TransformationState& ts,
                                                      const ImVec2& viewportPos, const ImVec2& viewportSize)
{
    static bool panningMode = false;

    // Prevent viewport picking/camera controls from fighting the transform gizmo
    // only when there is an active selection.
    if (!selectedShapes.empty() && (ImGuizmo::IsUsing() || ImGuizmo::IsOver())) {
        cameraDragging = false;
        mouseWasPressed = false;
        panningMode = false;
        return;
    }

    if (ts.mirrorHelperMoveModeActive) {
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
    const bool mouseInViewport =
        mouseX >= viewportX && mouseX <= (viewportX + viewportW) &&
        mouseY >= viewportY && mouseY <= (viewportY + viewportH);
    const bool blockByUiPanel = ImGui::GetIO().WantCaptureMouse && !mouseInViewport;

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
            cameraTarget[2] - cameraPos[2]
        };
        float fLen = std::sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
        if (fLen < 1e-6f) {
            return false;
        }
        forward[0] /= fLen; forward[1] /= fLen; forward[2] /= fLen;

        float upDefault[3] = {0.0f, 1.0f, 0.0f};
        float right[3] = {
            forward[1] * upDefault[2] - forward[2] * upDefault[1],
            forward[2] * upDefault[0] - forward[0] * upDefault[2],
            forward[0] * upDefault[1] - forward[1] * upDefault[0]
        };
        float rLen = std::sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
        if (rLen < 1e-6f) {
            return false;
        }
        right[0] /= rLen; right[1] /= rLen; right[2] /= rLen;

        float upVec[3] = {
            right[1] * forward[2] - right[2] * forward[1],
            right[2] * forward[0] - right[0] * forward[2],
            right[0] * forward[1] - right[1] * forward[0]
        };
        outRayDir[0] = forward[0] + static_cast<float>(ndcX) * right[0] + static_cast<float>(ndcY) * upVec[0];
        outRayDir[1] = forward[1] + static_cast<float>(ndcX) * right[1] + static_cast<float>(ndcY) * upVec[1];
        outRayDir[2] = forward[2] + static_cast<float>(ndcX) * right[2] + static_cast<float>(ndcY) * upVec[2];

        const float len = std::sqrt(outRayDir[0] * outRayDir[0] +
                                    outRayDir[1] * outRayDir[1] +
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
                baseRadius = std::sqrt(shape.param[0] * shape.param[0] +
                                       shape.param[1] * shape.param[1] +
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
                cameraPos[2] + rayDir[2] * t
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

    if (!blockByUiPanel && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        if (!mouseWasPressed)
        {
            if (!mouseInViewport) {
                return;
            }
            mouseWasPressed = true;
            lastMouseX = mouseX;
            lastMouseY = mouseY;
            
            // Check if Shift is pressed for panning mode
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
                glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
            {
                panningMode = true;
                cameraDragging = true; // Start camera dragging for panning
            }
            else
            {
                panningMode = false;
                if (trySelectMirrorHelper()) {
                    cameraDragging = false;
                } else {
                    float rayDir[3];
                    if (computePickRay(rayDir)) {
                        float t;
                        int hitShape = pickRayMarchSDF(cameraPos, rayDir, shapes, t);
                        if (hitShape >= 0) {
                            if (ImGui::GetIO().KeyShift)
                            {
                                auto it = std::find(selectedShapes.begin(), selectedShapes.end(), hitShape);
                                if (it != selectedShapes.end())
                                    selectedShapes.erase(it);
                                else
                                    selectedShapes.push_back(hitShape);
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
                        } else {
                            selectedShapes.clear();
                            ts.mirrorHelperSelected = false;
                            ts.mirrorHelperShapeIndex = -1;
                            ts.mirrorHelperAxis = -1;
                            ts.mirrorHelperMoveModeActive = false;
                            ts.mirrorHelperMoveConstrained = false;
                            ts.mirrorHelperMoveAxis = -1;
                            cameraDragging = true;
                        }
                    } else {
                        cameraDragging = true;
                    }
                }
            }
        }
        else if (cameraDragging)
        {
            double deltaX = mouseX - lastMouseX;
            double deltaY = mouseY - lastMouseY;
            lastMouseX = mouseX;
            lastMouseY = mouseY;
            
            if (panningMode)
            {
                // Pan the camera target instead of rotating
                float sensitivity = 0.01f;
                
                // Calculate camera's right and up vectors
                float forward[3] = { cameraTarget[0] - cameraPos[0],
                                     cameraTarget[1] - cameraPos[1],
                                     cameraTarget[2] - cameraPos[2] };
                float fLen = std::sqrt(forward[0]*forward[0] + forward[1]*forward[1] + forward[2]*forward[2]);
                forward[0] /= fLen; forward[1] /= fLen; forward[2] /= fLen;
                
                float upDefault[3] = { 0.0f, 1.0f, 0.0f };
                float right[3] = {
                    forward[1]*upDefault[2] - forward[2]*upDefault[1],
                    forward[2]*upDefault[0] - forward[0]*upDefault[2],
                    forward[0]*upDefault[1] - forward[1]*upDefault[0]
                };
                float rLen = std::sqrt(right[0]*right[0] + right[1]*right[1] + right[2]*right[2]);
                right[0] /= rLen; right[1] /= rLen; right[2] /= rLen;
                
                float up[3] = {
                    right[1]*forward[2] - right[2]*forward[1],
                    right[2]*forward[0] - right[0]*forward[2],
                    right[0]*forward[1] - right[1]*forward[0]
                };
                
                // Pan both camera position and target
                float panX = static_cast<float>(-deltaX) * sensitivity;
                float panY = static_cast<float>(deltaY) * sensitivity;
                
                cameraTarget[0] += panX * right[0] + panY * up[0];
                cameraTarget[1] += panX * right[1] + panY * up[1];
                cameraTarget[2] += panX * right[2] + panY * up[2];
                
                cameraPos[0] += panX * right[0] + panY * up[0];
                cameraPos[1] += panX * right[1] + panY * up[1];
                cameraPos[2] += panX * right[2] + panY * up[2];
            }
            else
            {
                // Normal rotation
                camTheta -= static_cast<float>(deltaX) * 0.008f;
                camPhi   += static_cast<float>(deltaY) * 0.008f;
                if(camPhi > 1.57f) camPhi = 1.57f;
                if(camPhi < -1.57f) camPhi = -1.57f;
            }
        }
    }
    else
    {
        mouseWasPressed = false;
        cameraDragging = false;
        panningMode = false;
    }
}
