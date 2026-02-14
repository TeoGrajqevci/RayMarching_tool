#pragma once

struct GLFWwindow;

namespace rmt {

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

} // namespace rmt
