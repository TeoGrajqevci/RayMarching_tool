#include "rmt/rendering/DebugCounterReader.h"

#include "rmt/rendering/Renderer.h"

namespace rmt {

bool Renderer::collectDebugCountersFromFramebuffer(int width, int height) {
    if (width <= 0 || height <= 0) {
        return false;
    }

    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) *
                                      static_cast<std::size_t>(height) * 4u, 0u);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (pixelCount == 0) {
        return false;
    }

    std::uint64_t sumMarch = 0;
    std::uint64_t sumShadow = 0;
    std::uint64_t sumTransmission = 0;
    int maxMarch = 0;
    int maxShadow = 0;
    int maxTransmission = 0;
    std::uint64_t hitThresholdCount = 0;
    std::uint64_t hitRefinedCount = 0;
    std::uint64_t missCount = 0;

    for (std::size_t i = 0; i < pixelCount; ++i) {
        const std::size_t base = i * 4u;
        const int marchSteps = static_cast<int>(pixels[base + 0]);
        const int shadowSteps = static_cast<int>(pixels[base + 1]);
        const int transmissionSteps = static_cast<int>(pixels[base + 2]);
        const int exitCode = static_cast<int>(pixels[base + 3]);

        sumMarch += static_cast<std::uint64_t>(marchSteps);
        sumShadow += static_cast<std::uint64_t>(shadowSteps);
        sumTransmission += static_cast<std::uint64_t>(transmissionSteps);
        maxMarch = std::max(maxMarch, marchSteps);
        maxShadow = std::max(maxShadow, shadowSteps);
        maxTransmission = std::max(maxTransmission, transmissionSteps);

        if (exitCode == 1) {
            ++hitThresholdCount;
        } else if (exitCode == 2) {
            ++hitRefinedCount;
        } else {
            ++missCount;
        }
    }

    ShaderPerfCounters counters;
    counters.avgMarchSteps = static_cast<double>(sumMarch) / static_cast<double>(pixelCount);
    counters.maxMarchSteps = static_cast<double>(maxMarch);
    counters.avgShadowSteps = static_cast<double>(sumShadow) / static_cast<double>(pixelCount);
    counters.maxShadowSteps = static_cast<double>(maxShadow);
    counters.avgTransmissionSteps = static_cast<double>(sumTransmission) / static_cast<double>(pixelCount);
    counters.maxTransmissionSteps = static_cast<double>(maxTransmission);
    counters.thresholdHitRate = static_cast<double>(hitThresholdCount) / static_cast<double>(pixelCount);
    counters.refinedHitRate = static_cast<double>(hitRefinedCount) / static_cast<double>(pixelCount);
    counters.hitRate = counters.thresholdHitRate + counters.refinedHitRate;
    counters.missRate = static_cast<double>(missCount) / static_cast<double>(pixelCount);
    lastPerfSnapshot.shaderCounters = counters;
    return true;
}

} // namespace rmt
