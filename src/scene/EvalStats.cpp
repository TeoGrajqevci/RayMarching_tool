#include "rmt/scene/EvalStats.h"

#include <atomic>

namespace rmt {

namespace {

#if !defined(NDEBUG)
std::atomic<std::uint64_t> gPrimitiveEvaluations(0);
std::atomic<std::uint64_t> gShapeEvaluations(0);
std::atomic<std::uint64_t> gSceneEvaluations(0);
#endif

} // namespace

void countPrimitiveEvaluation() {
#if !defined(NDEBUG)
    gPrimitiveEvaluations.fetch_add(1, std::memory_order_relaxed);
#endif
}

void countShapeEvaluation() {
#if !defined(NDEBUG)
    gShapeEvaluations.fetch_add(1, std::memory_order_relaxed);
#endif
}

void countSceneEvaluation() {
#if !defined(NDEBUG)
    gSceneEvaluations.fetch_add(1, std::memory_order_relaxed);
#endif
}

ShapeEvalStats getShapeEvalStats() {
    ShapeEvalStats stats;
#if !defined(NDEBUG)
    stats.primitiveEvaluations = gPrimitiveEvaluations.load(std::memory_order_relaxed);
    stats.shapeEvaluations = gShapeEvaluations.load(std::memory_order_relaxed);
    stats.sceneEvaluations = gSceneEvaluations.load(std::memory_order_relaxed);
#else
    stats.primitiveEvaluations = 0;
    stats.shapeEvaluations = 0;
    stats.sceneEvaluations = 0;
#endif
    return stats;
}

void resetShapeEvalStats() {
#if !defined(NDEBUG)
    gPrimitiveEvaluations.store(0, std::memory_order_relaxed);
    gShapeEvaluations.store(0, std::memory_order_relaxed);
    gSceneEvaluations.store(0, std::memory_order_relaxed);
#endif
}

} // namespace rmt
