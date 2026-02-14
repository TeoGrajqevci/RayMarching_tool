#ifndef RMT_RENDERING_RENDERER_INTERNAL_H
#define RMT_RENDERING_RENDERER_INTERNAL_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "rmt/RenderSettings.h"

namespace rmt {
namespace renderer_internal {

const int kMaxSelected = 128;
const int kShapeTexelStride = 14;
const int kTimerQueryCount = 4;
const int kAccelMinDim = 4;
const int kAccelMaxDim = 96;
const int kAccelMaxCells = 131072;
const int kAccelFallbackCellCandidateCount = 512;

inline void hashRaw(std::uint64_t& hash, const void* data, std::size_t size) {
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= static_cast<std::uint64_t>(bytes[i]);
        hash *= 1099511628211ULL;
    }
}

template <typename T>
inline void hashValue(std::uint64_t& hash, const T& value) {
    hashRaw(hash, &value, sizeof(T));
}

inline std::size_t shapeTexelBaseOffset(int shapeIndex, int texelSlot) {
    return static_cast<std::size_t>(shapeIndex * kShapeTexelStride + texelSlot) * 4u;
}

inline void writePackedTexel(std::vector<float>& buffer,
                             int shapeIndex,
                             int texelSlot,
                             float x,
                             float y,
                             float z,
                             float w) {
    const std::size_t base = shapeTexelBaseOffset(shapeIndex, texelSlot);
    buffer[base + 0] = x;
    buffer[base + 1] = y;
    buffer[base + 2] = z;
    buffer[base + 3] = w;
}

inline float clampf(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

inline int clampi(int value, int minValue, int maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

inline float safeCameraFovDegrees(const RenderSettings& renderSettings) {
    return clampf(renderSettings.cameraFovDegrees, 20.0f, 120.0f);
}

inline int safeCameraProjectionMode(const RenderSettings& renderSettings) {
    if (renderSettings.cameraProjectionMode == CAMERA_PROJECTION_ORTHOGRAPHIC) {
        return CAMERA_PROJECTION_ORTHOGRAPHIC;
    }
    return CAMERA_PROJECTION_PERSPECTIVE;
}

inline int safePathTracerMaxBounces(const RenderSettings& renderSettings) {
    return clampi(renderSettings.pathTracerMaxBounces, 1, 12);
}

inline int safeDenoiseStartSample(const RenderSettings& renderSettings) {
    return clampi(renderSettings.denoiseStartSample, 1, 4096);
}

inline int safeDenoiseInterval(const RenderSettings& renderSettings) {
    return clampi(renderSettings.denoiseInterval, 1, 1024);
}

inline float computeCameraOrthoScale(const float cameraPos[3], const float cameraTarget[3], float fovDegrees) {
    const float dx = cameraPos[0] - cameraTarget[0];
    const float dy = cameraPos[1] - cameraTarget[1];
    const float dz = cameraPos[2] - cameraTarget[2];
    const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    const float halfFovRadians = fovDegrees * 0.5f * (3.14159265358979323846f / 180.0f);
    return std::max(0.05f, distance * std::tan(halfFovRadians));
}

} // namespace renderer_internal
} // namespace rmt

#endif // RMT_RENDERING_RENDERER_INTERNAL_H
