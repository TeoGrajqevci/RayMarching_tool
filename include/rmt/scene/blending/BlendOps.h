#pragma once

#include "rmt/scene/RuntimeShapeData.h"

namespace rmt {

float smoothUnionDistance(float candidateDist, float sceneDist, float smoothness);
float smoothSubtractionDistance(float candidateDist, float sceneDist, float smoothness);
float smoothIntersectionDistance(float candidateDist, float sceneDist, float smoothness);

bool shouldSkipFromBounds(const RuntimeShapeData& shapeData,
                          const float p[3],
                          float sceneDist);

} // namespace rmt
