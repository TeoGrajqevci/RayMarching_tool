#pragma once

#include <vector>

#include "rmt/input/Input.h"
#include "rmt/scene/Shape.h"
#include "rmt/ui/UiState.h"

namespace rmt {

void renderScenePanel(const UiWorkspaceGeometry& geometry,
                      std::vector<Shape>& shapes,
                      std::vector<int>& selectedShapes,
                      int& editingShapeIndex,
                      char (&renameBuffer)[128],
                      TransformationState& transformState);

} // namespace rmt
