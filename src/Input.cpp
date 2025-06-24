#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Input.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include "Texture.h"
#include <cmath>

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
            !ts.translationModeActive && !ts.rotationModeActive && !ts.scaleModeActive)
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
            ts.showAxisGuides = true; // Activer les guides d'axe
            ts.activeAxis = -1; // Pas d'axe spécifique encore
            
            // Calculer le centre des formes sélectionnées pour les guides
            if (!selectedShapes.empty()) {
                float centerX = 0.0f, centerY = 0.0f, centerZ = 0.0f;
                for (int idx : selectedShapes) {
                    centerX += shapes[idx].center[0];
                    centerY += shapes[idx].center[1];
                    centerZ += shapes[idx].center[2];
                }
                ts.guideCenter[0] = centerX / selectedShapes.size();
                ts.guideCenter[1] = centerY / selectedShapes.size();
                ts.guideCenter[2] = centerZ / selectedShapes.size();
            }
            
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
            ts.showAxisGuides = true; // Activer les guides d'axe pour la rotation
            ts.activeAxis = -1; // Pas d'axe spécifique encore
            glfwGetCursorPos(window, &ts.rotationStartMouseX, &ts.rotationStartMouseY);
            
            // Calculer le centre de rotation (centre des formes sélectionnées)
            if (!selectedShapes.empty()) {
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
            }
            
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
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && !ts.scaleKeyHandled)
        {
            ts.scaleModeActive = true;
            ts.scaleConstrained = false;
            ts.scaleAxis = -1;
            ts.scaleKeyHandled = true;
            ts.showAxisGuides = true; // Activer les guides d'axe pour la mise à l'échelle
            ts.activeAxis = -1; // Pas d'axe spécifique encore
            
            // Calculer le centre des formes sélectionnées pour les guides
            if (!selectedShapes.empty()) {
                float centerX = 0.0f, centerY = 0.0f, centerZ = 0.0f;
                for (int idx : selectedShapes) {
                    centerX += shapes[idx].center[0];
                    centerY += shapes[idx].center[1];
                    centerZ += shapes[idx].center[2];
                }
                ts.guideCenter[0] = centerX / selectedShapes.size();
                ts.guideCenter[1] = centerY / selectedShapes.size();
                ts.guideCenter[2] = centerZ / selectedShapes.size();
            }
            
            glfwGetCursorPos(window, &ts.scaleStartMouseX, &ts.scaleStartMouseY);
            ts.scaleOriginalParams.clear();
            for (int idx : selectedShapes) {
                std::array<float, 3> scale = { shapes[idx].scale[0], shapes[idx].scale[1], shapes[idx].scale[2] };
                ts.scaleOriginalParams.push_back(scale);
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
                ts.activeAxis = 0; // Axe X actif pour les guides
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
            
            if (ts.rotationAxis == 0) axis[0] = 1.0f;      // X axis
            else if (ts.rotationAxis == 1) axis[1] = 1.0f; // Y axis
            else if (ts.rotationAxis == 2) axis[2] = 1.0f; // Z axis
            
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
        if (!ImGui::GetIO().WantCaptureMouse && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
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
        float sensitivity = 0.005f;
        if (ts.scaleConstrained)
        {
            for (size_t i = 0; i < selectedShapes.size(); i++) {
                int idx = selectedShapes[i];
                float scaleFactor = 1.0f + static_cast<float>(deltaY) * sensitivity;
                
                // Restaurer les valeurs originales pour tous les axes
                shapes[idx].scale[0] = ts.scaleOriginalParams[i][0];
                shapes[idx].scale[1] = ts.scaleOriginalParams[i][1];
                shapes[idx].scale[2] = ts.scaleOriginalParams[i][2];
                
                // Appliquer la mise à l'échelle seulement sur l'axe sélectionné
                if (ts.scaleAxis == 0)
                    shapes[idx].scale[0] = ts.scaleOriginalParams[i][0] * scaleFactor;
                else if (ts.scaleAxis == 1)
                    shapes[idx].scale[1] = ts.scaleOriginalParams[i][1] * scaleFactor;
                else if (ts.scaleAxis == 2)
                    shapes[idx].scale[2] = ts.scaleOriginalParams[i][2] * scaleFactor;
            }
        }
        else
        {
            float scaleFactor = 1.0f + static_cast<float>(deltaY) * sensitivity;
            for (size_t i = 0; i < selectedShapes.size(); i++) {
                int idx = selectedShapes[i];
                shapes[idx].scale[0] = ts.scaleOriginalParams[i][0] * scaleFactor;
                shapes[idx].scale[1] = ts.scaleOriginalParams[i][1] * scaleFactor;
                shapes[idx].scale[2] = ts.scaleOriginalParams[i][2] * scaleFactor;
            }
        }
        if (!ts.scaleConstrained && !ImGui::GetIO().WantCaptureKeyboard)
        {
            if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
            {
                ts.scaleConstrained = true;
                ts.scaleAxis = 0;
                ts.activeAxis = 0; // Axe X actif pour les guides
                glfwGetCursorPos(window, &ts.scaleStartMouseX, &ts.scaleStartMouseY);
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.scaleOriginalParams[i][0] = shapes[idx].scale[0];
                    ts.scaleOriginalParams[i][1] = shapes[idx].scale[1];
                    ts.scaleOriginalParams[i][2] = shapes[idx].scale[2];
                }
            }
            else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
            {
                ts.scaleConstrained = true;
                ts.scaleAxis = 1;
                ts.activeAxis = 1; // Axe Y actif pour les guides
                glfwGetCursorPos(window, &ts.scaleStartMouseX, &ts.scaleStartMouseY);
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.scaleOriginalParams[i][0] = shapes[idx].scale[0];
                    ts.scaleOriginalParams[i][1] = shapes[idx].scale[1];
                    ts.scaleOriginalParams[i][2] = shapes[idx].scale[2];
                }
            }
            else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
            {
                ts.scaleConstrained = true;
                ts.scaleAxis = 2;
                ts.activeAxis = 2; // Axe Z actif pour les guides
                glfwGetCursorPos(window, &ts.scaleStartMouseX, &ts.scaleStartMouseY);
                for (size_t i = 0; i < selectedShapes.size(); i++) {
                    int idx = selectedShapes[i];
                    ts.scaleOriginalParams[i][0] = shapes[idx].scale[0];
                    ts.scaleOriginalParams[i][1] = shapes[idx].scale[1];
                    ts.scaleOriginalParams[i][2] = shapes[idx].scale[2];
                }
            }
        }
        if (!ImGui::GetIO().WantCaptureMouse && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
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
                shapes[idx].scale[0] = ts.scaleOriginalParams[i][0];
                shapes[idx].scale[1] = ts.scaleOriginalParams[i][1];
                shapes[idx].scale[2] = ts.scaleOriginalParams[i][2];
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
                                                      bool& mouseWasPressed)
{
    static bool panningMode = false;
    
    if (!ImGui::GetIO().WantCaptureMouse && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        if (!mouseWasPressed)
        {
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
                // Normal picking logic
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
