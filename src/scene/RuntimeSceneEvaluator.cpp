#include "rmt/scene/RuntimeSceneEvaluator.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "rmt/scene/EvalStats.h"
#include "rmt/scene/ShapeEnums.h"
#include "rmt/scene/blending/BlendOps.h"
#include "rmt/scene/modifiers/ShapeModifiers.h"
#include "rmt/scene/primitives/PrimitiveDistance.h"

namespace rmt {

namespace {

const float kHugeDistance = 1e6f;

float evaluateRuntimeShapeDistanceSingle(const float p[3], const RuntimeShapeData& shapeData) {
    float local[3] = {
        p[0] - shapeData.center[0],
        p[1] - shapeData.center[1],
        p[2] - shapeData.center[2]
    };

    float rotated[3];
    if ((shapeData.flags & RUNTIME_SHAPE_HAS_ROTATION) != 0) {
        applyInverseRotationRows(shapeData.invRotationRows, local, rotated);
    } else {
        rotated[0] = local[0];
        rotated[1] = local[1];
        rotated[2] = local[2];
    }

    float twisted[3];
    if ((shapeData.flags & RUNTIME_SHAPE_HAS_TWIST) != 0) {
        applyTwistCombinedCPU(rotated, shapeData.twist, twisted);
    } else {
        twisted[0] = rotated[0];
        twisted[1] = rotated[1];
        twisted[2] = rotated[2];
    }

    float bent[3];
    if ((shapeData.flags & RUNTIME_SHAPE_HAS_BEND) != 0) {
        applyCheapBendCombinedCPU(twisted, shapeData.bend, bent);
    } else {
        bent[0] = twisted[0];
        bent[1] = twisted[1];
        bent[2] = twisted[2];
    }

    float q[3] = {
        bent[0],
        bent[1],
        bent[2]
    };
    float correction = 0.0f;

    if ((shapeData.flags & RUNTIME_SHAPE_HAS_ELONGATION) != 0) {
        const float raw[3] = {
            std::fabs(bent[0]) - shapeData.elongation[0],
            std::fabs(bent[1]) - shapeData.elongation[1],
            std::fabs(bent[2]) - shapeData.elongation[2]
        };

        q[0] = std::max(raw[0], 0.0f);
        q[1] = std::max(raw[1], 0.0f);
        q[2] = std::max(raw[2], 0.0f);
        correction = std::min(std::max(raw[0], std::max(raw[1], raw[2])), 0.0f);
    }

    float distance = primitiveDistanceScaledCPU(shapeData.type, q, shapeData) + correction;

    if ((shapeData.flags & RUNTIME_SHAPE_HAS_ROUNDING) != 0) {
        distance -= shapeData.roundness;
    }

    const float warpAmount = absSum3(shapeData.twist) + absSum3(shapeData.bend);
    const float safety = 1.0f + 0.35f * std::min(warpAmount, 8.0f);
    distance /= safety;

    return distance;
}

} // namespace

float evaluateRuntimeShapeDistance(const float p[3], const RuntimeShapeData& shapeData) {
    countShapeEvaluation();

    float bestDistance = evaluateRuntimeShapeDistanceSingle(p, shapeData);
    if ((shapeData.flags & RUNTIME_SHAPE_HAS_MIRROR) == 0) {
        return bestDistance;
    }

    const int maxX = shapeData.mirror[0] > 0.5f ? 1 : 0;
    const int maxY = shapeData.mirror[1] > 0.5f ? 1 : 0;
    const int maxZ = shapeData.mirror[2] > 0.5f ? 1 : 0;
    if (maxX == 0 && maxY == 0 && maxZ == 0) {
        return bestDistance;
    }

    const float mirrorK = std::max(shapeData.mirrorSmoothness, 0.0f);

    for (int ix = 0; ix <= maxX; ++ix) {
        for (int iy = 0; iy <= maxY; ++iy) {
            for (int iz = 0; iz <= maxZ; ++iz) {
                if (ix == 0 && iy == 0 && iz == 0) {
                    continue;
                }

                float mirroredWorld[3] = {
                    ix == 1 ? (2.0f * shapeData.mirrorOffset[0] - p[0]) : p[0],
                    iy == 1 ? (2.0f * shapeData.mirrorOffset[1] - p[1]) : p[1],
                    iz == 1 ? (2.0f * shapeData.mirrorOffset[2] - p[2]) : p[2]
                };
                const float candidateDist = evaluateRuntimeShapeDistanceSingle(mirroredWorld, shapeData);
                if (mirrorK > 1e-6f) {
                    bestDistance = smoothUnionDistance(candidateDist, bestDistance, mirrorK);
                } else {
                    bestDistance = std::min(bestDistance, candidateDist);
                }
            }
        }
    }

    return bestDistance;
}

float evaluateRuntimeSceneDistance(const float p[3], const std::vector<RuntimeShapeData>& shapeDataList) {
    countSceneEvaluation();

    if (shapeDataList.empty()) {
        return kHugeDistance;
    }

    float sceneDist = kHugeDistance;
    for (std::size_t i = 0; i < shapeDataList.size(); ++i) {
        const RuntimeShapeData& shapeData = shapeDataList[i];
        if (shouldSkipFromBounds(shapeData, p, sceneDist)) {
            continue;
        }

        const float candidateDist = evaluateRuntimeShapeDistance(p, shapeData);
        switch (shapeData.blendOp) {
            case BLEND_SMOOTH_UNION:
                sceneDist = smoothUnionDistance(candidateDist, sceneDist, shapeData.smoothness);
                break;
            case BLEND_SMOOTH_SUBTRACTION:
                sceneDist = smoothSubtractionDistance(candidateDist, sceneDist, shapeData.smoothness);
                break;
            case BLEND_SMOOTH_INTERSECTION:
                sceneDist = smoothIntersectionDistance(candidateDist, sceneDist, shapeData.smoothness);
                break;
            case BLEND_NONE:
            default:
                sceneDist = std::min(sceneDist, candidateDist);
                break;
        }
    }

    return sceneDist;
}

} // namespace rmt
