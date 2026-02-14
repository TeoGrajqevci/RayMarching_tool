#ifndef RMT_INPUT_TRANSFORM_SCALE_TOOL_H
#define RMT_INPUT_TRANSFORM_SCALE_TOOL_H

#include <vector>

#include "imgui.h"

#include "rmt/input/Input.h"
#include "rmt/scene/Shape.h"

struct GLFWwindow;

namespace rmt {

void updateScaleTool(GLFWwindow* window,
                     std::vector<Shape>& shapes,
                     const std::vector<int>& selectedShapes,
                     TransformationState& ts,
                     const ImVec2& viewportPos,
                     const ImVec2& viewportSize);

} // namespace rmt

#endif // RMT_INPUT_TRANSFORM_SCALE_TOOL_H
