#pragma once

#include "imgui.h"

#include "rmt/ui/UiState.h"

namespace rmt {

ImDrawList* renderViewportPanel(const UiWorkspaceGeometry& geometry,
                                ImVec2& viewportPos,
                                ImVec2& viewportSize,
                                ImVec2& helpButtonPos);

} // namespace rmt
