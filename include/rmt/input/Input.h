#ifndef RMT_INPUT_INPUT_H
#define RMT_INPUT_INPUT_H

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

#include "imgui.h"

#include "rmt/platform/glfw/Callbacks.h"
#include "rmt/scene/Shape.h"

struct GLFWwindow;

namespace rmt {

struct PointLightState {
    float position[3] = {0.0f, 1.5f, 0.0f};
    float color[3] = {1.0f, 1.0f, 1.0f};
    float intensity = 12.0f;
    float range = 8.0f;
    float radius = 0.05f;
};

struct TransformationState {
    bool useLocalSpace = false;

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
    std::vector<std::array<float, 3>> rotationOriginalCenters;
    float rotationCenter[3] = {0.0f, 0.0f, 0.0f};

    bool scaleConstrained = false;
    int scaleAxis = -1;
    double scaleStartMouseX = 0.0, scaleStartMouseY = 0.0;
    std::vector<std::array<float, 3>> scaleOriginalScales;
    std::vector<std::array<float, 3>> scaleOriginalElongations;

    bool translationKeyHandled = false;
    bool rotationKeyHandled = false;
    bool scaleKeyHandled = false;

    bool showAxisGuides = false;
    int activeAxis = -1;
    float guideCenter[3] = {0.0f, 0.0f, 0.0f};
    float guideAxisDirection[3] = {1.0f, 0.0f, 0.0f};

    bool mirrorHelperSelected = false;
    int mirrorHelperShapeIndex = -1;
    int mirrorHelperAxis = -1;
    bool mirrorHelperMoveModeActive = false;
    bool mirrorHelperMoveConstrained = false;
    int mirrorHelperMoveAxis = -1;
    double mirrorHelperMoveStartMouseX = 0.0;
    double mirrorHelperMoveStartMouseY = 0.0;
    float mirrorHelperMoveStartOffset[3] = {0.0f, 0.0f, 0.0f};
    float mirrorHelperMoveStartShapeCenter[3] = {0.0f, 0.0f, 0.0f};

    bool sunLightInitialized = false;
    bool sunLightEnabled = true;
    bool sunLightSelected = false;
    bool sunLightMoveModeActive = false;
    bool sunLightMoveConstrained = false;
    int sunLightMoveAxis = -1;
    double sunLightMoveStartMouseX = 0.0;
    double sunLightMoveStartMouseY = 0.0;
    float sunLightPosition[3] = {0.0f, 2.0f, 0.0f};
    float sunLightMoveStartPosition[3] = {0.0f, 2.0f, 0.0f};
    bool sunLightHandleDragActive = false;
    double sunLightHandleStartMouseX = 0.0;
    double sunLightHandleStartMouseY = 0.0;
    float sunLightHandleStartDirection[3] = {0.0f, -1.0f, 0.0f};
    float sunLightHandleDistance = 1.2f;

    std::vector<PointLightState> pointLights;
    int selectedPointLightIndex = -1;
    bool pointLightMoveModeActive = false;
    bool pointLightMoveConstrained = false;
    int pointLightMoveAxis = -1;
    double pointLightMoveStartMouseX = 0.0;
    double pointLightMoveStartMouseY = 0.0;
    float pointLightMoveStartPosition[3] = {0.0f, 1.5f, 0.0f};
};

class InputManager {
public:
    InputManager();
    ~InputManager();

    void processGeneralInput(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes,
                             float cameraTarget[3], bool& showGUI, bool& altRenderMode,
                             TransformationState& ts);
    void processTransformationModeActivation(GLFWwindow* window, std::vector<Shape>& shapes,
                                             std::vector<int>& selectedShapes, TransformationState& ts);
    void processTransformationUpdates(GLFWwindow* window, std::vector<Shape>& shapes,
                                      std::vector<int>& selectedShapes,
                                      TransformationState& ts,
                                      float lightDir[3],
                                      const float cameraPos[3], const float cameraTarget[3],
                                      const ImVec2& viewportPos, const ImVec2& viewportSize);
    void processMousePickingAndCameraDrag(GLFWwindow* window, std::vector<Shape>& shapes,
                                          std::vector<int>& selectedShapes,
                                          float cameraPos[3], float cameraTarget[3],
                                          float& camTheta, float& camPhi,
                                          bool& cameraDragging, double& lastMouseX, double& lastMouseY,
                                          bool& mouseWasPressed,
                                          TransformationState& ts,
                                          float lightDir[3],
                                          const ImVec2& viewportPos, const ImVec2& viewportSize);
};

} // namespace rmt

#endif // RMT_INPUT_INPUT_H
