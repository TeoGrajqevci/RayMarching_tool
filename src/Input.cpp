#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Input.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include "Texture.h"

// These extern declarations assume that camDistance, hdrTexture, and hdrLoaded are defined in main.cpp
extern float camDistance;
extern Texture hdrTexture;
extern bool hdrLoaded;

// -------------------------------------------------------------------------
// Callback implementations
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

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    if (count > 0) {
        const char* path = paths[0];
        std::string filepath(path);
        std::string ext = filepath.substr(filepath.find_last_of(".") + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == "hdr" || ext == "exr") {
            if (hdrTexture.loadHDR(path)) {
                // The file path is stored in hdrTexture.filePath and hdrLoaded is updated accordingly.
            }
        }
    }
}

// -------------------------------------------------------------------------
// InputManager class method implementations.
InputManager::InputManager()
{
}

InputManager::~InputManager()
{
}

void InputManager::processGeneralInput(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes, float cameraTarget[3], bool& showGUI, bool& altRenderMode)
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
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
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
    if (!ImGui::GetIO().WantCaptureKeyboard && glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
    {
        if (!hKeyPressed)
        {
            showGUI = !showGUI;
            altRenderMode = !altRenderMode;
            hKeyPressed = true;
        }
    }
    else if (glfwGetKey(window, GLFW_KEY_H) == GLFW_RELEASE)
    {
        hKeyPressed = false;
    }
}

void InputManager::processTransformationModeActivation(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes, TransformationState& ts)
{
    if (!ts.translationModeActive && !ts.rotationModeActive && !ts.scaleModeActive && !ImGui::GetIO().WantCaptureKeyboard && !selectedShapes.empty())
    {
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && !ts.translationKeyHandled)
        {
            ts.translationModeActive = true;
            ts.translationConstrained = false;
            ts.translationAxis = -1;
            ts.translationKeyHandled = true;
            glfwGetCursorPos(window, &ts.translationStartMouseX, &ts.translationStartMouseY);
            ts.translationOriginalCenters.clear();
            for (int idx : selectedShapes) {
                std::array<float, 3> center = { shapes[idx].center[0], shapes[idx].center[1], shapes[idx].center[2] };
                ts.translationOriginalCenters.push_back(center);
            }
        }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !ts.rotationKeyHandled)
        {
            ts.rotationModeActive = true;
            ts.rotationConstrained = false;
            ts.rotationAxis = -1;
            ts.rotationKeyHandled = true;
            glfwGetCursorPos(window, &ts.rotationStartMouseX, &ts.rotationStartMouseY);
            ts.rotationOriginalRotations.clear();
            for (int idx : selectedShapes) {
                std::array<float, 3> rot = { shapes[idx].rotation[0], shapes[idx].rotation[1], shapes[idx].rotation[2] };
                ts.rotationOriginalRotations.push_back(rot);
            }
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && !ts.scaleKeyHandled)
        {
            ts.scaleModeActive = true;
            ts.scaleConstrained = false;
            ts.scaleAxis = -1;
            ts.scaleKeyHandled = true;
            glfwGetCursorPos(window, &ts.scaleStartMouseX, &ts.scaleStartMouseY);
            ts.scaleOriginalParams.clear();
            for (int idx : selectedShapes) {
                std::array<float, 3> param = { shapes[idx].param[0], shapes[idx].param[1], shapes[idx].param[2] };
                ts.scaleOriginalParams.push_back(param);
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
                                                  TransformationState& ts, const float cameraPos[3], const float cameraTarget[3])
{
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
            for (size_t i = 0; i < selectedShapes.size(); i++) {
                int idx = selectedShapes[i];
                if (ts.translationAxis == 0)
                    shapes[idx].center[0] = ts.translationOriginalCenters[i][0] + static_cast<float>(deltaX) * sensitivity;
                else if (ts.translationAxis == 1)
                    shapes[idx].center[1] = ts.translationOriginalCenters[i][1] - static_cast<float>(deltaY) * sensitivity;
                else if (ts.translationAxis == 2)
                    shapes[idx].center[2] = ts.translationOriginalCenters[i][2] + static_cast<float>(deltaX) * sensitivity;
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
                glfwGetCursorPos(window, &ts.translationStartMouseX, &ts.translationStartMouseY);
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.translationOriginalCenters[i][0] = shapes[idx].center[0];
                    ts.translationOriginalCenters[i][1] = shapes[idx].center[1];
                    ts.translationOriginalCenters[i][2] = shapes[idx].center[2];
                }
            }
        }
        if (!ImGui::GetIO().WantCaptureMouse && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            ts.translationModeActive = false;
            ts.translationConstrained = false;
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
            for (size_t i = 0; i < selectedShapes.size(); i++) {
                int idx = selectedShapes[i];
                if (ts.rotationAxis == 0)
                    shapes[idx].rotation[0] = ts.rotationOriginalRotations[i][0] + static_cast<float>(deltaX) * sensitivity;
                else if (ts.rotationAxis == 1)
                    shapes[idx].rotation[1] = ts.rotationOriginalRotations[i][1] + static_cast<float>(deltaX) * sensitivity;
                else if (ts.rotationAxis == 2)
                    shapes[idx].rotation[2] = ts.rotationOriginalRotations[i][2] + static_cast<float>(deltaX) * sensitivity;
            }
        }
        else
        {
            for (size_t i = 0; i < selectedShapes.size(); i++) {
                int idx = selectedShapes[i];
                shapes[idx].rotation[0] = ts.rotationOriginalRotations[i][0] + static_cast<float>(deltaY) * sensitivity;
                shapes[idx].rotation[1] = ts.rotationOriginalRotations[i][1] + static_cast<float>(deltaX) * sensitivity;
                shapes[idx].rotation[2] = ts.rotationOriginalRotations[i][2];
            }
        }
        if (!ts.rotationConstrained && !ImGui::GetIO().WantCaptureKeyboard)
        {
            if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
            {
                ts.rotationConstrained = true;
                ts.rotationAxis = 0;
                glfwGetCursorPos(window, &ts.rotationStartMouseX, &ts.rotationStartMouseY);
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.rotationOriginalRotations[i][0] = shapes[idx].rotation[0];
                }
            }
            else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
            {
                ts.rotationConstrained = true;
                ts.rotationAxis = 1;
                glfwGetCursorPos(window, &ts.rotationStartMouseX, &ts.rotationStartMouseY);
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.rotationOriginalRotations[i][1] = shapes[idx].rotation[1];
                }
            }
            else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
            {
                ts.rotationConstrained = true;
                ts.rotationAxis = 2;
                glfwGetCursorPos(window, &ts.rotationStartMouseX, &ts.rotationStartMouseY);
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.rotationOriginalRotations[i][2] = shapes[idx].rotation[2];
                }
            }
        }
        if (!ImGui::GetIO().WantCaptureMouse && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            ts.rotationModeActive = false;
            ts.rotationConstrained = false;
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
        }
    }
    // Scale mode update
    if (ts.scaleModeActive && !selectedShapes.empty())
    {
        double currentMouseX, currentMouseY;
        glfwGetCursorPos(window, &currentMouseX, &currentMouseY);
        double deltaY = currentMouseY - ts.scaleStartMouseY;
        float sensitivity = 0.005f;
        if (ts.scaleConstrained)
        {
            for (size_t i = 0; i < selectedShapes.size(); i++) {
                int idx = selectedShapes[i];
                if (ts.scaleAxis == 0)
                    shapes[idx].param[0] = ts.scaleOriginalParams[i][0] * (1.0f + static_cast<float>(deltaY) * sensitivity);
                else if (ts.scaleAxis == 1)
                    shapes[idx].param[1] = ts.scaleOriginalParams[i][1] * (1.0f + static_cast<float>(deltaY) * sensitivity);
                else if (ts.scaleAxis == 2)
                    shapes[idx].param[2] = ts.scaleOriginalParams[i][2] * (1.0f + static_cast<float>(deltaY) * sensitivity);
            }
        }
        else
        {
            float scaleFactor = 1.0f + static_cast<float>(deltaY) * sensitivity;
            for (size_t i = 0; i < selectedShapes.size(); i++) {
                int idx = selectedShapes[i];
                shapes[idx].param[0] = ts.scaleOriginalParams[i][0] * scaleFactor;
                shapes[idx].param[1] = ts.scaleOriginalParams[i][1] * scaleFactor;
                shapes[idx].param[2] = ts.scaleOriginalParams[i][2] * scaleFactor;
            }
        }
        if (!ts.scaleConstrained && !ImGui::GetIO().WantCaptureKeyboard)
        {
            if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
            {
                ts.scaleConstrained = true;
                ts.scaleAxis = 0;
                glfwGetCursorPos(window, &ts.scaleStartMouseX, &ts.scaleStartMouseY);
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.scaleOriginalParams[i][0] = shapes[idx].param[0];
                }
            }
            else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
            {
                ts.scaleConstrained = true;
                ts.scaleAxis = 1;
                glfwGetCursorPos(window, &ts.scaleStartMouseX, &ts.scaleStartMouseY);
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.scaleOriginalParams[i][1] = shapes[idx].param[1];
                }
            }
            else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
            {
                ts.scaleConstrained = true;
                ts.scaleAxis = 2;
                glfwGetCursorPos(window, &ts.scaleStartMouseX, &ts.scaleStartMouseY);
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.scaleOriginalParams[i][2] = shapes[idx].param[2];
                }
            }
        }
        if (!ImGui::GetIO().WantCaptureMouse && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            ts.scaleModeActive = false;
            ts.scaleConstrained = false;
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            for (size_t i = 0; i < selectedShapes.size(); i++) {
                int idx = selectedShapes[i];
                shapes[idx].param[0] = ts.scaleOriginalParams[i][0];
                shapes[idx].param[1] = ts.scaleOriginalParams[i][1];
                shapes[idx].param[2] = ts.scaleOriginalParams[i][2];
            }
            ts.scaleModeActive = false;
            ts.scaleConstrained = false;
        }
    }
}

void InputManager::processMousePickingAndCameraDrag(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes,
                                                      float cameraPos[3], float cameraTarget[3],
                                                      float& camTheta, float& camPhi,
                                                      bool& cameraDragging, double& lastMouseX, double& lastMouseY,
                                                      bool& mouseWasPressed)
{
    if (!ImGui::GetIO().WantCaptureMouse && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        if (!mouseWasPressed)
        {
            mouseWasPressed = true;
            lastMouseX = mouseX;
            lastMouseY = mouseY;
            int fbWidth, fbHeight;
            glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
            int winWidth, winHeight;
            glfwGetWindowSize(window, &winWidth, &winHeight);
            float scaleX = (float)fbWidth / winWidth;
            float scaleY = (float)fbHeight / winHeight;
            mouseX *= scaleX;
            mouseY *= scaleY;
            double ndcX = (mouseX / fbWidth) * 2.0 - 1.0;
            double ndcY = ((fbHeight - mouseY) / (double)fbHeight) * 2.0 - 1.0;
            ndcX *= (double)fbWidth / fbHeight;
            float forward[3] = { cameraTarget[0] - cameraPos[0],
                                 cameraTarget[1] - cameraPos[1],
                                 cameraTarget[2] - cameraPos[2] };
            float fLen = std::sqrt(forward[0]*forward[0] + forward[1]*forward[1] + forward[2]*forward[2]);
            forward[0] /= fLen; forward[1] /= fLen; forward[2] /= fLen;
            float upDefault[3] = {0.0f, 1.0f, 0.0f};
            float right[3] = {
                forward[1]*upDefault[2] - forward[2]*upDefault[1],
                forward[2]*upDefault[0] - forward[0]*upDefault[2],
                forward[0]*upDefault[1] - forward[1]*upDefault[0]
            };
            float rLen = std::sqrt(right[0]*right[0] + right[1]*right[1] + right[2]*right[2]);
            right[0] /= rLen; right[1] /= rLen; right[2] /= rLen;
            float upVec[3] = {
                right[1]*forward[2] - right[2]*forward[1],
                right[2]*forward[0] - right[0]*forward[2],
                right[0]*forward[1] - right[1]*forward[0]
            };
            float rayDir[3] = {
                forward[0] + static_cast<float>(ndcX) * right[0] + static_cast<float>(ndcY) * upVec[0],
                forward[1] + static_cast<float>(ndcX) * right[1] + static_cast<float>(ndcY) * upVec[1],
                forward[2] + static_cast<float>(ndcX) * right[2] + static_cast<float>(ndcY) * upVec[2]
            };
            float len = std::sqrt(rayDir[0]*rayDir[0] + rayDir[1]*rayDir[1] + rayDir[2]*rayDir[2]);
            rayDir[0] /= len; rayDir[1] /= len; rayDir[2] /= len;
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
            } else {
                selectedShapes.clear();
                cameraDragging = true;
            }
        }
        else if (cameraDragging)
        {
            double deltaX = mouseX - lastMouseX;
            double deltaY = mouseY - lastMouseY;
            lastMouseX = mouseX;
            lastMouseY = mouseY;
            camTheta += static_cast<float>(deltaX) * 0.008f;
            camPhi   += static_cast<float>(deltaY) * 0.008f;
            if(camPhi > 1.57f) camPhi = 1.57f;
            if(camPhi < -1.57f) camPhi = -1.57f;
        }
    }
    else
    {
        mouseWasPressed = false;
        cameraDragging = false;
    }
}
