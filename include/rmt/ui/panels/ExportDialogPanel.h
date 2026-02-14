#pragma once

#include <vector>

#include "rmt/scene/Shape.h"
#include "rmt/ui/UiState.h"

namespace rmt {

void renderExportDialogPanel(std::vector<Shape>& shapes,
                             UiRuntimeState& uiState,
                             bool& showConsole,
                             int winWidth,
                             int winHeight);

} // namespace rmt
