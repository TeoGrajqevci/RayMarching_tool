#pragma once

#include <vector>

#include "rmt/scene/RuntimeShapeData.h"

namespace rmt {

float evaluateRuntimeShapeDistance(const float p[3], const RuntimeShapeData& shapeData);
float evaluateRuntimeSceneDistance(const float p[3], const std::vector<RuntimeShapeData>& shapeDataList);

} // namespace rmt
