#ifndef RMT_UI_UI_FACADE_H
#define RMT_UI_UI_FACADE_H

#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "imgui.h"

#include "rmt/input/Input.h"
#include "rmt/io/mesh/export/MeshExporter.h"
#include "rmt/platform/imgui/ImGuiLayer.h"
#include "rmt/RenderSettings.h"
#include "rmt/scene/Shape.h"
#include "rmt/ui/console/Console.h"
#include "rmt/ui/UiState.h"

namespace rmt {

class GUIManager {
public:
    GUIManager();
    ~GUIManager();

    void newFrame();
    ImVec2 getViewportPos() const;
    ImVec2 getViewportSize() const;

    void renderGUI(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes,
                   float lightDir[3], float lightColor[3], float ambientColor[3], float backgroundColor[3],
                   float& ambientIntensity, float& directLightIntensity,
                   bool& useGradientBackground,
                   bool& showGUI, bool& altRenderMode, bool& usePathTracer,
                   int& editingShapeIndex, char (&renameBuffer)[128],
                   bool& showHelpPopup, ImVec2& helpButtonPos,
                   bool& showConsole, TransformationState& transformState,
                   const float cameraPos[3], const float cameraTarget[3],
                   RenderSettings& renderSettings,
                   bool denoiserAvailable, bool denoiserUsingGPU,
                   int pathSampleCount);

private:
    ImVec2 viewportPos;
    ImVec2 viewportSize;
    UiRuntimeState uiState;
};

} // namespace rmt

#endif // RMT_UI_UI_FACADE_H
