#pragma once

#include <vector>

#include "rmt/input/Input.h"
#include "rmt/scene/Shape.h"
#include "rmt/ui/UiState.h"

namespace rmt {

void renderToolbarPanel(std::vector<Shape>& shapes,
                        UiRuntimeState& uiState,
                        TransformationState& transformState,
                        bool& showGUI,
                        bool& altRenderMode,
                        bool& usePathTracer,
                        bool& showConsole,
                        bool& showHelpPopup,
                        int winWidth);

} // namespace rmt
