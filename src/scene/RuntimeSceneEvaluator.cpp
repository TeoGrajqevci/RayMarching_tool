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

int clampInt(int value, int minValue, int maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

float evaluateRuntimeShapeDistanceFromLocal(const float local[3], const RuntimeShapeData& shapeData) {
    float q[3] = {
        local[0],
        local[1],
        local[2]
    };
    float correction = 0.0f;

    if ((shapeData.flags & RUNTIME_SHAPE_HAS_ELONGATION) != 0) {
        const float raw[3] = {
            std::fabs(local[0]) - shapeData.elongation[0],
            std::fabs(local[1]) - shapeData.elongation[1],
            std::fabs(local[2]) - shapeData.elongation[2]
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

void buildArrayCandidateIndices(float axisCoord,
                                float spacing,
                                float repeatCount,
                                float smoothness,
                                int outIndices[3],
                                int& outCount) {
    const int repeats = clampInt(static_cast<int>(std::floor(repeatCount + 0.5f)), 1, 128);
    const float center = 0.5f * static_cast<float>(repeats - 1);
    const float safeSpacing = std::max(std::fabs(spacing), 1e-4f);
    const float coord = axisCoord / safeSpacing + center;
    const int nearest = clampInt(static_cast<int>(std::floor(coord + 0.5f)), 0, repeats - 1);

    outIndices[0] = nearest;
    outIndices[1] = nearest;
    outIndices[2] = nearest;
    outCount = 1;

    if (smoothness <= 1e-6f || repeats <= 1) {
        return;
    }

    int values[3] = {
        clampInt(nearest - 1, 0, repeats - 1),
        nearest,
        clampInt(nearest + 1, 0, repeats - 1)
    };

    int uniqueCount = 0;
    for (int i = 0; i < 3; ++i) {
        bool seen = false;
        for (int j = 0; j < uniqueCount; ++j) {
            if (outIndices[j] == values[i]) {
                seen = true;
                break;
            }
        }
        if (!seen) {
            outIndices[uniqueCount++] = values[i];
        }
    }

    if (uniqueCount <= 0) {
        outIndices[0] = nearest;
        outCount = 1;
        return;
    }

    outCount = uniqueCount;
    for (int i = uniqueCount; i < 3; ++i) {
        outIndices[i] = outIndices[uniqueCount - 1];
    }
}

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

    float candidates[27][3];
    int candidateCount = 1;
    candidates[0][0] = rotated[0];
    candidates[0][1] = rotated[1];
    candidates[0][2] = rotated[2];

    bool usedArrayModifier = false;
    const float arraySmoothness = std::max(shapeData.arraySmoothness, 0.0f);

    for (int stackIndex = 0; stackIndex < 3; ++stackIndex) {
        const int modifierId = clampInt(static_cast<int>(std::floor(shapeData.modifierStack[stackIndex] + 0.5f)),
                                        SHAPE_MODIFIER_BEND,
                                        SHAPE_MODIFIER_ARRAY);

        if (modifierId == SHAPE_MODIFIER_TWIST && (shapeData.flags & RUNTIME_SHAPE_HAS_TWIST) != 0) {
            for (int i = 0; i < candidateCount; ++i) {
                float out[3];
                applyTwistCombinedCPU(candidates[i], shapeData.twist, out);
                candidates[i][0] = out[0];
                candidates[i][1] = out[1];
                candidates[i][2] = out[2];
            }
        } else if (modifierId == SHAPE_MODIFIER_BEND && (shapeData.flags & RUNTIME_SHAPE_HAS_BEND) != 0) {
            for (int i = 0; i < candidateCount; ++i) {
                float out[3];
                applyCheapBendCombinedCPU(candidates[i], shapeData.bend, out);
                candidates[i][0] = out[0];
                candidates[i][1] = out[1];
                candidates[i][2] = out[2];
            }
        } else if (modifierId == SHAPE_MODIFIER_ARRAY && (shapeData.flags & RUNTIME_SHAPE_HAS_ARRAY) != 0) {
            float expanded[27][3];
            int expandedCount = 0;

            const int repeatX = clampInt(static_cast<int>(std::floor(shapeData.arrayRepeatCount[0] + 0.5f)), 1, 128);
            const int repeatY = clampInt(static_cast<int>(std::floor(shapeData.arrayRepeatCount[1] + 0.5f)), 1, 128);
            const int repeatZ = clampInt(static_cast<int>(std::floor(shapeData.arrayRepeatCount[2] + 0.5f)), 1, 128);
            const float centerX = 0.5f * static_cast<float>(repeatX - 1);
            const float centerY = 0.5f * static_cast<float>(repeatY - 1);
            const float centerZ = 0.5f * static_cast<float>(repeatZ - 1);

            for (int baseIndex = 0; baseIndex < candidateCount; ++baseIndex) {
                int xIndices[3] = { 0, 0, 0 };
                int yIndices[3] = { 0, 0, 0 };
                int zIndices[3] = { 0, 0, 0 };
                int xCount = 1;
                int yCount = 1;
                int zCount = 1;

                if (shapeData.arrayAxis[0] > 0.5f) {
                    buildArrayCandidateIndices(candidates[baseIndex][0], shapeData.arraySpacing[0], shapeData.arrayRepeatCount[0], arraySmoothness, xIndices, xCount);
                }
                if (shapeData.arrayAxis[1] > 0.5f) {
                    buildArrayCandidateIndices(candidates[baseIndex][1], shapeData.arraySpacing[1], shapeData.arrayRepeatCount[1], arraySmoothness, yIndices, yCount);
                }
                if (shapeData.arrayAxis[2] > 0.5f) {
                    buildArrayCandidateIndices(candidates[baseIndex][2], shapeData.arraySpacing[2], shapeData.arrayRepeatCount[2], arraySmoothness, zIndices, zCount);
                }

                for (int xi = 0; xi < xCount; ++xi) {
                    for (int yi = 0; yi < yCount; ++yi) {
                        for (int zi = 0; zi < zCount; ++zi) {
                            if (expandedCount >= 27) {
                                continue;
                            }

                            expanded[expandedCount][0] = candidates[baseIndex][0];
                            expanded[expandedCount][1] = candidates[baseIndex][1];
                            expanded[expandedCount][2] = candidates[baseIndex][2];

                            if (shapeData.arrayAxis[0] > 0.5f) {
                                const float shift = (static_cast<float>(xIndices[xi]) - centerX) * shapeData.arraySpacing[0];
                                expanded[expandedCount][0] -= shift;
                            }
                            if (shapeData.arrayAxis[1] > 0.5f) {
                                const float shift = (static_cast<float>(yIndices[yi]) - centerY) * shapeData.arraySpacing[1];
                                expanded[expandedCount][1] -= shift;
                            }
                            if (shapeData.arrayAxis[2] > 0.5f) {
                                const float shift = (static_cast<float>(zIndices[zi]) - centerZ) * shapeData.arraySpacing[2];
                                expanded[expandedCount][2] -= shift;
                            }

                            ++expandedCount;
                        }
                    }
                }
            }

            if (expandedCount > 0) {
                candidateCount = expandedCount;
                for (int i = 0; i < candidateCount; ++i) {
                    candidates[i][0] = expanded[i][0];
                    candidates[i][1] = expanded[i][1];
                    candidates[i][2] = expanded[i][2];
                }
                usedArrayModifier = true;
            }
        }
    }

    float bestDistance = kHugeDistance;
    for (int i = 0; i < candidateCount; ++i) {
        const float candidate = evaluateRuntimeShapeDistanceFromLocal(candidates[i], shapeData);
        if (i == 0) {
            bestDistance = candidate;
        } else if (usedArrayModifier && arraySmoothness > 1e-6f) {
            bestDistance = smoothUnionDistance(candidate, bestDistance, arraySmoothness);
        } else {
            bestDistance = std::min(bestDistance, candidate);
        }
    }

    return bestDistance;
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
