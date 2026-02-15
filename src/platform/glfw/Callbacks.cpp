#include "rmt/platform/glfw/Callbacks.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>

namespace rmt {

extern float camDistance;

namespace {

struct ViewportInputContext {
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;
    bool viewportHovered = false;
    bool uiWantsMouseCapture = false;
    bool valid = false;
};

ViewportInputContext gViewportInputContext;

} // namespace

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

void updateViewportInputContext(float viewportX,
                                float viewportY,
                                float viewportW,
                                float viewportH,
                                bool viewportHovered,
                                bool uiWantsMouseCapture) {
    gViewportInputContext.x = static_cast<double>(viewportX);
    gViewportInputContext.y = static_cast<double>(viewportY);
    gViewportInputContext.w = static_cast<double>(std::max(0.0f, viewportW));
    gViewportInputContext.h = static_cast<double>(std::max(0.0f, viewportH));
    gViewportInputContext.viewportHovered = viewportHovered;
    gViewportInputContext.uiWantsMouseCapture = uiWantsMouseCapture;
    gViewportInputContext.valid = gViewportInputContext.w > 0.0 && gViewportInputContext.h > 0.0;
}

bool shouldBlockViewportInput(double mouseX, double mouseY) {
    if (!gViewportInputContext.valid) {
        return false;
    }

    const bool mouseInViewport =
        mouseX >= gViewportInputContext.x && mouseX <= (gViewportInputContext.x + gViewportInputContext.w) &&
        mouseY >= gViewportInputContext.y && mouseY <= (gViewportInputContext.y + gViewportInputContext.h);

    if (!mouseInViewport) {
        return true;
    }

    return gViewportInputContext.uiWantsMouseCapture && !gViewportInputContext.viewportHovered;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    (void)xoffset;

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    if (shouldBlockViewportInput(mouseX, mouseY)) {
        return;
    }

    camDistance -= static_cast<float>(yoffset) * 0.5f;
    if (camDistance < 1.0f) {
        camDistance = 1.0f;
    }
    if (camDistance > 20.0f) {
        camDistance = 20.0f;
    }
}

} // namespace rmt
