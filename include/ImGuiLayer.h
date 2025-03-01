#ifndef IMGUI_LAYER_H
#define IMGUI_LAYER_H

#include <GLFW/glfw3.h>

namespace ImGuiLayer {
    void Init(GLFWwindow* window);
    void NewFrame();
    void Render();
    void Shutdown();
}

#endif