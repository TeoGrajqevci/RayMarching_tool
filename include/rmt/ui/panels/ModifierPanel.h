#pragma once

#include <vector>

#include "rmt/RenderSettings.h"
#include "rmt/input/Input.h"
#include "rmt/scene/Shape.h"
#include "rmt/ui/UiState.h"

namespace rmt {

void renderModifierPanel(const UiWorkspaceGeometry& geometry,
                         std::vector<Shape>& shapes,
                         std::vector<int>& selectedShapes,
                         TransformationState& transformState,
                         const float cameraPos[3],
                         const float cameraTarget[3],
                         float lightDir[3],
                         float lightColor[3],
                         float ambientColor[3],
                         float backgroundColor[3],
                         float& ambientIntensity,
                         float& directLightIntensity,
                         RenderSettings& renderSettings,
                         bool denoiserAvailable,
                         bool denoiserUsingGPU,
                         int pathSampleCount);

} // namespace rmt
