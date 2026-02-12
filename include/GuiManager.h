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
#include "RenderSettings.h"
#include "MeshExporter.h"
#include "Input.h"

// Simple console class for logging
class Console {
public:
    struct LogEntry {
        std::string message;
        float timestamp;
        int level; // 0: info, 1: warning, 2: error
    };
    
    static Console& getInstance() {
        static Console instance;
        return instance;
    }
    
    void addLog(const std::string& message, int level = 0);
    void clear();
    void render(bool* p_open);
    
private:
    std::vector<LogEntry> logs;
    bool autoScroll = true;
    float startTime;
    
    Console();
    ImVec4 getLogColor(int level);
};

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
};

#endif // GUIMANAGER_H
