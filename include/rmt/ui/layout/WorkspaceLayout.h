#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "rmt/ui/UiState.h"

namespace rmt {

void updateWorkspaceLayout(GLFWwindow* window,
                           bool showConsole,
                           bool showHelpPopup,
                           UiRuntimeState& uiState,
                           UiWorkspaceGeometry& geometry);

void drawWorkspaceSplitters(const UiWorkspaceGeometry& geometry,
                            bool showHelpPopup);

} // namespace rmt
