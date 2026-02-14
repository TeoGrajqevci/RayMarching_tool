#include "rmt/scene/blending/BlendOps.h"

#include <algorithm>
#include <cmath>

#include "rmt/scene/ShapeEnums.h"

namespace rmt {

namespace {

float clampf(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

float mixf(float a, float b, float t) {
    return a + (b - a) * t;
}

} // namespace

float smoothUnionDistance(float candidateDist, float sceneDist, float smoothness) {
    const float k = std::max(smoothness, 1e-4f);
    const float h = clampf(0.5f + 0.5f * (sceneDist - candidateDist) / k, 0.0f, 1.0f);
    return mixf(sceneDist, candidateDist, h) - k * h * (1.0f - h);
}

float smoothSubtractionDistance(float candidateDist, float sceneDist, float smoothness) {
    const float k = std::max(smoothness, 1e-4f);
    const float h = clampf(0.5f - 0.5f * (sceneDist + candidateDist) / k, 0.0f, 1.0f);
    return mixf(sceneDist, -candidateDist, h) + k * h * (1.0f - h);
}

float smoothIntersectionDistance(float candidateDist, float sceneDist, float smoothness) {
    const float k = std::max(smoothness, 1e-4f);
    const float h = clampf(0.5f - 0.5f * (sceneDist - candidateDist) / k, 0.0f, 1.0f);
    return mixf(sceneDist, candidateDist, h) + k * h * (1.0f - h);
}

bool shouldSkipFromBounds(const RuntimeShapeData& shapeData,
                          const float p[3],
                          float sceneDist) {
    const float dx = p[0] - shapeData.center[0];
    const float dy = p[1] - shapeData.center[1];
    const float dz = p[2] - shapeData.center[2];
    const float centerDist = std::sqrt(dx * dx + dy * dy + dz * dz);
    const float lb = centerDist - shapeData.influenceRadius;
    const float ub = centerDist + shapeData.influenceRadius;
    const float k = std::max(shapeData.smoothness, 1e-4f);

    switch (shapeData.blendOp) {
        case BLEND_NONE:
            return lb >= sceneDist;
        case BLEND_SMOOTH_UNION:
            return lb >= sceneDist + k;
        case BLEND_SMOOTH_SUBTRACTION:
            return sceneDist + lb >= k;
        case BLEND_SMOOTH_INTERSECTION:
            return ub <= sceneDist - k;
        default:
            return lb >= sceneDist;
    }
}

} // namespace rmt
