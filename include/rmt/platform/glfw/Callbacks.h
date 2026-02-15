#pragma once

struct GLFWwindow;

namespace rmt {

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void updateViewportInputContext(float viewportX,
								float viewportY,
								float viewportW,
								float viewportH,
								bool viewportHovered,
								bool uiWantsMouseCapture);
bool shouldBlockViewportInput(double mouseX, double mouseY);

} // namespace rmt
