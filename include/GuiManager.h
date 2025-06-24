#ifndef GUIMANAGER_H
#define GUIMANAGER_H

#include <vector>
#include <array>
#include <string>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "ImGuiLayer.h"
#include "imgui.h"
#include "Shapes.h"
#include "MeshExporter.h"

class GUIManager {
public:
    GUIManager();
    ~GUIManager();

    void newFrame();

    void renderGUI(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes,
                   float lightDir[3], float lightColor[3], float ambientColor[3],
                   bool& useGradientBackground,
                   bool& showGUI, bool& altRenderMode,
                   int& editingShapeIndex, char (&renameBuffer)[128],
                   bool& showHelpPopup, ImVec2& helpButtonPos);
};

#endif // GUIMANAGER_H
