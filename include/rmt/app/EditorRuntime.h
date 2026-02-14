#pragma once

#include <vector>

#include "imgui.h"

#include "rmt/RenderSettings.h"
#include "rmt/input/Input.h"
#include "rmt/rendering/Renderer.h"
#include "rmt/scene/Shape.h"

struct GLFWwindow;

namespace rmt {

class GUIManager;
class InputManager;
class UndoRedoManager;

struct EditorRuntimeState {
    bool showGui;
    bool showHelpPopup;
    bool showConsole;
    int editingShapeIndex;
    char renameBuffer[128];

    EditorRuntimeState();
};

void resetTransformInteractionState(TransformationState& ts);
bool isInsideViewport(double x, double y, const ImVec2& viewportPos, const ImVec2& viewportSize);

int runEditorRuntimeLoop(GLFWwindow* window,
                         Renderer& renderer,
                         std::vector<Shape>& shapes,
                         std::vector<int>& selectedShapes,
                         float lightDir[3],
                         float lightColor[3],
                         float ambientColor[3],
                         float backgroundColor[3],
                         float& ambientIntensity,
                         float& directLightIntensity,
                         float cameraPos[3],
                         float cameraTarget[3],
                         float& camTheta,
                         float& camPhi,
                         bool& useGradientBackground,
                         bool& altRenderMode,
                         bool& usePathTracer,
                         RenderSettings& renderSettings,
                         InputManager& inputManager,
                         GUIManager& guiManager,
                         UndoRedoManager& undoRedoManager,
                         EditorRuntimeState& runtimeState);

} // namespace rmt
