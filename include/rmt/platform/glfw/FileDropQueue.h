#pragma once

#include <string>
#include <vector>

struct GLFWwindow;

namespace rmt {

struct DroppedFileEvent {
    std::string path;
    double mouseX;
    double mouseY;
};

void fileDropCallback(GLFWwindow* window, int count, const char** paths);
std::vector<DroppedFileEvent> consumeDroppedFileEvents();

} // namespace rmt
