#ifndef INPUT_H
#define INPUT_H

#include <vector>
#include <array>
#include <string>
#include <cmath>
#include <algorithm>

// Forward declare GLFWwindow instead of including GLFW/glfw3.h
struct GLFWwindow;

// Include Shapes for the Shape type.
#include "Shapes.h"

// Include ImGui header (required for inline calls to ImGui functions)
#include "imgui.h"

// -------------------------------------------------------------------------
// Transformation state for handling translation, rotation and scale modes.
struct TransformationState {
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

    bool scaleConstrained = false;
    int scaleAxis = -1;
    double scaleStartMouseX = 0.0, scaleStartMouseY = 0.0;
    std::vector<std::array<float, 3>> scaleOriginalParams;

    bool translationKeyHandled = false;
    bool rotationKeyHandled = false;
    bool scaleKeyHandled = false;
};

// -------------------------------------------------------------------------
// InputManager class: handles key input, transformation mode activation/updates, mouse picking & camera drag.
class InputManager {
public:
    InputManager();
    ~InputManager();

    void processGeneralInput(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes, float cameraTarget[3], bool& showGUI, bool& altRenderMode);
    void processTransformationModeActivation(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes, TransformationState& ts);
    void processTransformationUpdates(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes,
                                      TransformationState& ts, const float cameraPos[3], const float cameraTarget[3]);
    void processMousePickingAndCameraDrag(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes,
                                          float cameraPos[3], float cameraTarget[3],
                                          float& camTheta, float& camPhi,
                                          bool& cameraDragging, double& lastMouseX, double& lastMouseY,
                                          bool& mouseWasPressed);
};

// -------------------------------------------------------------------------
// Callback function declarations.
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void drop_callback(GLFWwindow* window, int count, const char** paths);

#endif
