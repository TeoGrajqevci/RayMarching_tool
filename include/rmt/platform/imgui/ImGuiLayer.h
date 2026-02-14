#ifndef RMT_PLATFORM_IMGUI_LAYER_H
#define RMT_PLATFORM_IMGUI_LAYER_H

#include <GLFW/glfw3.h>

namespace rmt {
namespace ImGuiLayer {

void Init(GLFWwindow* window);
void NewFrame();
void Render();
void Shutdown();

} // namespace ImGuiLayer
} // namespace rmt

#endif // RMT_PLATFORM_IMGUI_LAYER_H
