#include "rmt/platform/glfw/Callbacks.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace rmt {

extern float camDistance;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    (void)window;
    (void)xoffset;
    camDistance -= static_cast<float>(yoffset) * 0.5f;
    if (camDistance < 1.0f) {
        camDistance = 1.0f;
    }
    if (camDistance > 20.0f) {
        camDistance = 20.0f;
    }
}

} // namespace rmt
