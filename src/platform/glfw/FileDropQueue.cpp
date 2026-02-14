#include "rmt/platform/glfw/FileDropQueue.h"

#include <GLFW/glfw3.h>

namespace rmt {

namespace {

std::vector<DroppedFileEvent> gDroppedFileEvents;

} // namespace

void fileDropCallback(GLFWwindow* window, int count, const char** paths) {
    if (window == nullptr || count <= 0 || paths == nullptr) {
        return;
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    for (int i = 0; i < count; ++i) {
        if (paths[i] == nullptr) {
            continue;
        }

        DroppedFileEvent event;
        event.path = paths[i];
        event.mouseX = mouseX;
        event.mouseY = mouseY;
        gDroppedFileEvents.push_back(event);
    }
}

std::vector<DroppedFileEvent> consumeDroppedFileEvents() {
    std::vector<DroppedFileEvent> events;
    events.swap(gDroppedFileEvents);
    return events;
}

} // namespace rmt
