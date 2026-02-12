#ifndef INPUT_H
#define INPUT_H

#include <vector>
#include <array>
#include <string>
#include <cmath>
#include <algorithm>

struct GLFWwindow;

#include "Shapes.h"

#include "imgui.h"

struct TransformationState {
    bool useLocalSpace = false; // false: world, true: local

    bool translationModeActive = false;
    bool rotationModeActive = false;
    bool scaleModeActive = false;

    bool translationConstrained = false;
    int translationAxis = -1;
    double translationStartMouseX = 0.0, translationStartMouseY = 0.0;
    std::vector<std::array<float, 3>> translationOriginalCenters;

    bool rotationConstrained = false;
    int rotationAxis = -1;
    double rotationStartMouseX = 0.0, rotationStartMouseY = 0.0;
    std::vector<std::array<float, 3>> rotationOriginalRotations;
    std::vector<std::array<float, 3>> rotationOriginalCenters; // Centres originaux pour rotation mondiale
    float rotationCenter[3] = {0.0f, 0.0f, 0.0f}; // Centre de rotation mondiale

    bool scaleConstrained = false;
    int scaleAxis = -1;
    double scaleStartMouseX = 0.0, scaleStartMouseY = 0.0;
    std::vector<std::array<float, 3>> scaleOriginalScales;
    std::vector<std::array<float, 3>> scaleOriginalElongations;

    bool translationKeyHandled = false;
    bool rotationKeyHandled = false;
    bool scaleKeyHandled = false;
    
    // Nouvelles données pour les guides d'axe
    bool showAxisGuides = false;
    int activeAxis = -1; // 0=X, 1=Y, 2=Z
    float guideCenter[3] = {0.0f, 0.0f, 0.0f}; // Centre du guide (centre de la forme sélectionnée)
    float guideAxisDirection[3] = {1.0f, 0.0f, 0.0f}; // Direction monde de l'axe actif

    bool mirrorHelperSelected = false;
    int mirrorHelperShapeIndex = -1;
    int mirrorHelperAxis = -1; // Plane normal axis (X/Y/Z)
    bool mirrorHelperMoveModeActive = false;
    bool mirrorHelperMoveConstrained = false;
    int mirrorHelperMoveAxis = -1;
    double mirrorHelperMoveStartMouseX = 0.0;
    double mirrorHelperMoveStartMouseY = 0.0;
    float mirrorHelperMoveStartOffset[3] = {0.0f, 0.0f, 0.0f};
    float mirrorHelperMoveStartShapeCenter[3] = {0.0f, 0.0f, 0.0f};
};

class InputManager {
public:
    InputManager();
    ~InputManager();

    void processGeneralInput(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes, float cameraTarget[3], bool& showGUI, bool& altRenderMode, const TransformationState& ts);
    void processTransformationModeActivation(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes, TransformationState& ts);
    void processTransformationUpdates(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes,
                                      TransformationState& ts, const float cameraPos[3], const float cameraTarget[3],
                                      const ImVec2& viewportPos, const ImVec2& viewportSize);
    void processMousePickingAndCameraDrag(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes,
                                          float cameraPos[3], float cameraTarget[3],
                                          float& camTheta, float& camPhi,
                                          bool& cameraDragging, double& lastMouseX, double& lastMouseY,
                                          bool& mouseWasPressed,
                                          TransformationState& ts,
                                          const ImVec2& viewportPos, const ImVec2& viewportSize);
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

#endif
