#pragma once

#include <vector>

#include "imgui.h"

#include "rmt/RenderSettings.h"
#include "rmt/input/Input.h"
#include "rmt/scene/Shape.h"
#include "rmt/ui/UiState.h"

namespace rmt {

void renderViewportGizmo(ImDrawList* viewportDrawList,
                         const ImVec2& viewportPos,
                         const ImVec2& viewportSize,
                         std::vector<Shape>& shapes,
                         const std::vector<int>& selectedShapes,
                         TransformationState& transformState,
                         float lightDir[3],
                         const float cameraPos[3],
                         const float cameraTarget[3],
                         float& camTheta,
                         float& camPhi,
                         const RenderSettings& renderSettings,
                         UiRuntimeState& uiState);

} // namespace rmt
