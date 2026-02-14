#pragma once

#include <vector>

#include "rmt/input/Input.h"
#include "rmt/scene/Shape.h"
#include "rmt/ui/UiState.h"

namespace rmt {

void renderInspectorPanel(const UiWorkspaceGeometry& geometry,
                          std::vector<Shape>& shapes,
                          std::vector<int>& selectedShapes,
                          TransformationState& transformState,
                          int globalScaleMode);

} // namespace rmt
