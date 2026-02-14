#include "rmt/rendering/PerfTelemetry.h"

#include "rmt/rendering/Renderer.h"

#include <GLFW/glfw3.h>

#include <iostream>

namespace rmt {

namespace {

const int kTimerQueryCount = 4;

} // namespace

void Renderer::processPerfTimers() {
    auto updateTimer = [](GpuTimerState& timer) {
        if (timer.initializedCount <= 0) {
            return;
        }

        const int queryIndex = (timer.writeIndex + kTimerQueryCount - 1) % kTimerQueryCount;
        GLuint available = GL_FALSE;
        glGetQueryObjectuiv(timer.queries[queryIndex], GL_QUERY_RESULT_AVAILABLE, &available);
        if (available == GL_TRUE) {
            GLuint64 elapsedNs = 0;
            glGetQueryObjectui64v(timer.queries[queryIndex], GL_QUERY_RESULT, &elapsedNs);
            timer.lastMilliseconds = static_cast<double>(elapsedNs) * 1e-6;
        }
    };

    updateTimer(solidGpuTimer);
    updateTimer(pathGpuTimer);
    updateTimer(denoiseGpuTimer);
}

void Renderer::resolvePerfTimersBlocking() {
    auto updateTimerBlocking = [](GpuTimerState& timer) {
        if (timer.initializedCount <= 0) {
            return;
        }

        const int queryIndex = (timer.writeIndex + kTimerQueryCount - 1) % kTimerQueryCount;
        GLuint64 elapsedNs = 0;
        glGetQueryObjectui64v(timer.queries[queryIndex], GL_QUERY_RESULT, &elapsedNs);
        timer.lastMilliseconds = static_cast<double>(elapsedNs) * 1e-6;
    };

    updateTimerBlocking(solidGpuTimer);
    updateTimerBlocking(pathGpuTimer);
    updateTimerBlocking(denoiseGpuTimer);

    lastPerfSnapshot.gpuSolidMs = solidGpuTimer.lastMilliseconds;
    lastPerfSnapshot.gpuPathMs = pathGpuTimer.lastMilliseconds;
    lastPerfSnapshot.gpuDenoiseMs = denoiseGpuTimer.lastMilliseconds;
}

void Renderer::logPerfIfNeeded(const std::vector<Shape>& shapes) {
    if (!perfLoggingEnabled) {
        return;
    }

    const double now = glfwGetTime();
    if (lastPerfLogTime <= 0.0) {
        lastPerfLogTime = now;
        return;
    }

    const double elapsed = now - lastPerfLogTime;
    if (elapsed < 1.0) {
        return;
    }

    const double avgCpuMs = (cpuFrameAccumulatedCount > 0)
        ? (cpuFrameAccumulatedMs / static_cast<double>(cpuFrameAccumulatedCount))
        : 0.0;
    const double fps = (elapsed > 0.0)
        ? (static_cast<double>(cpuFrameAccumulatedCount) / elapsed)
        : 0.0;

    const ShapeEvalStats evalStats = getShapeEvalStats();
    const std::uint64_t deltaPrimitive = evalStats.primitiveEvaluations - lastPrimitiveEvalCount;
    const std::uint64_t deltaShape = evalStats.shapeEvaluations - lastShapeEvalCount;
    const std::uint64_t deltaScene = evalStats.sceneEvaluations - lastSceneEvalCount;

    std::cout << "[Perf] fps=" << fps
              << " cpu_ms=" << avgCpuMs
              << " cpu_stage(scene/mesh/setup/draw_ms)="
              << cpuSceneUploadMs << "/"
              << cpuMeshUploadMs << "/"
              << cpuSetupMs << "/"
              << cpuDrawSubmitMs
              << " gpu_solid_ms=" << solidGpuTimer.lastMilliseconds
              << " gpu_path_ms=" << pathGpuTimer.lastMilliseconds
              << " gpu_denoise_ms=" << denoiseGpuTimer.lastMilliseconds
              << " shapes=" << shapes.size()
              << " uploaded_shapes=" << uploadedShapeCount
              << " accel(enabled/cells/fallback/indexes/avg_candidates)="
              << (accelGridEnabled ? 1 : 0) << "/"
              << accelCellCount << "/"
              << accelFallbackCellCount << "/"
              << accelShapeIndexCount << "/"
              << accelAverageCandidates
              << " accel_dim=" << accelGridDim[0] << "x" << accelGridDim[1] << "x" << accelGridDim[2]
              << " shape_eval(delta_scene/delta_shape/delta_primitive)="
              << deltaScene << "/" << deltaShape << "/" << deltaPrimitive
              << std::endl;

    cpuFrameAccumulatedMs = 0.0;
    cpuFrameAccumulatedCount = 0;
    lastPerfLogTime = now;

    lastPrimitiveEvalCount = evalStats.primitiveEvaluations;
    lastShapeEvalCount = evalStats.shapeEvaluations;
    lastSceneEvalCount = evalStats.sceneEvaluations;
}

} // namespace rmt
