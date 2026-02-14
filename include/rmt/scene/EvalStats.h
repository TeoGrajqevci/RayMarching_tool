#pragma once

#include <cstdint>

namespace rmt {

struct ShapeEvalStats {
    std::uint64_t primitiveEvaluations;
    std::uint64_t shapeEvaluations;
    std::uint64_t sceneEvaluations;
};

ShapeEvalStats getShapeEvalStats();
void resetShapeEvalStats();

void countPrimitiveEvaluation();
void countShapeEvaluation();
void countSceneEvaluation();

} // namespace rmt
