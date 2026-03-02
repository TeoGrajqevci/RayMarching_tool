#pragma once

#include <vector>

#include "rmt/input/Input.h"
#include "rmt/platform/glfw/FileDropQueue.h"
#include "rmt/scene/Shape.h"
#include "rmt/ui/UiState.h"

namespace rmt {

void renderInspectorPanel(const ImVec2& viewportPos,
                          const ImVec2& viewportSize,
                          std::vector<Shape>& shapes,
                          std::vector<int>& selectedShapes,
                          TransformationState& transformState,
                          int globalScaleMode,
                          const std::vector<DroppedFileEvent>& droppedFiles,
                          std::vector<char>& consumedDropFlags);

} // namespace rmt
