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
const int kShapeTexelStride = 20;
const int kMaxMaterialTextures = 8;
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

inline float safePathTracerGuidedSamplingMix(const RenderSettings& renderSettings) {
    return clampf(renderSettings.pathTracerGuidedSamplingMix, 0.0f, 0.95f);
}

inline float safePathTracerMisPower(const RenderSettings& renderSettings) {
    return clampf(renderSettings.pathTracerMisPower, 1.0f, 4.0f);
}

inline int safePathTracerRussianRouletteStartBounce(const RenderSettings& renderSettings) {
    return clampi(renderSettings.pathTracerRussianRouletteStartBounce, 1, 12);
}

inline float safePathTracerResolutionScale(const RenderSettings& renderSettings) {
    return clampf(renderSettings.pathTracerResolutionScale, 0.5f, 1.0f);
}

inline bool safePathTracerPhysicalCameraEnabled(const RenderSettings& renderSettings) {
    return renderSettings.pathTracerPhysicalCameraEnabled;
}

inline float safePathTracerCameraFocalLengthMm(const RenderSettings& renderSettings) {
    return clampf(renderSettings.pathTracerCameraFocalLengthMm, 8.0f, 400.0f);
}

inline float safePathTracerCameraSensorWidthMm(const RenderSettings& renderSettings) {
    return clampf(renderSettings.pathTracerCameraSensorWidthMm, 4.0f, 70.0f);
}

inline float safePathTracerCameraSensorHeightMm(const RenderSettings& renderSettings) {
    return clampf(renderSettings.pathTracerCameraSensorHeightMm, 3.0f, 70.0f);
}

inline float safePathTracerCameraApertureFNumber(const RenderSettings& renderSettings) {
    return clampf(renderSettings.pathTracerCameraApertureFNumber, 0.7f, 64.0f);
}

inline float safePathTracerCameraFocusDistance(const RenderSettings& renderSettings) {
    return clampf(renderSettings.pathTracerCameraFocusDistance, 0.05f, 5000.0f);
}

inline int safePathTracerCameraBladeCount(const RenderSettings& renderSettings) {
    return clampi(renderSettings.pathTracerCameraBladeCount, 0, 12);
}

inline float safePathTracerCameraBladeRotationDegrees(const RenderSettings& renderSettings) {
    return clampf(renderSettings.pathTracerCameraBladeRotationDegrees, -360.0f, 360.0f);
}

inline float safePathTracerCameraAnamorphicRatio(const RenderSettings& renderSettings) {
    return clampf(renderSettings.pathTracerCameraAnamorphicRatio, 0.25f, 4.0f);
}

inline float safePathTracerCameraChromaticAberration(const RenderSettings& renderSettings) {
    if (!safePathTracerPhysicalCameraEnabled(renderSettings) ||
        safeCameraProjectionMode(renderSettings) != CAMERA_PROJECTION_PERSPECTIVE) {
        return 0.0f;
    }
    return clampf(renderSettings.pathTracerCameraChromaticAberration, 0.0f, 8.0f);
}

inline float safePathTracerCameraShutterSeconds(const RenderSettings& renderSettings) {
    return clampf(renderSettings.pathTracerCameraShutterSeconds, 1.0f / 8000.0f, 30.0f);
}

inline float safePathTracerCameraIso(const RenderSettings& renderSettings) {
    return clampf(renderSettings.pathTracerCameraIso, 25.0f, 25600.0f);
}

inline float safePathTracerCameraExposureCompensationEv(const RenderSettings& renderSettings) {
    return clampf(renderSettings.pathTracerCameraExposureCompensationEv, -10.0f, 10.0f);
}

inline bool usePathTracerPhysicalCamera(const RenderSettings& renderSettings) {
    return safePathTracerPhysicalCameraEnabled(renderSettings) &&
           safeCameraProjectionMode(renderSettings) == CAMERA_PROJECTION_PERSPECTIVE;
}

inline float safePathTracerCameraLensRadius(const RenderSettings& renderSettings) {
    if (!usePathTracerPhysicalCamera(renderSettings)) {
        return 0.0f;
    }
    const float apertureFNumber = safePathTracerCameraApertureFNumber(renderSettings);
    return 0.5f / apertureFNumber;
}

inline float safePathTracerCameraBladeRotationRadians(const RenderSettings& renderSettings) {
    const float rotationDegrees = safePathTracerCameraBladeRotationDegrees(renderSettings);
    return rotationDegrees * (3.14159265358979323846f / 180.0f);
}

inline float safePathTracerExposureScale(const RenderSettings& renderSettings) {
    if (!usePathTracerPhysicalCamera(renderSettings)) {
        return 1.0f;
    }

    constexpr float kReferenceApertureFNumber = 8.0f;
    constexpr float kReferenceShutterSeconds = 1.0f / 125.0f;
    constexpr float kReferenceIso = 100.0f;

    const float apertureFNumber = safePathTracerCameraApertureFNumber(renderSettings);
    const float shutterSeconds = safePathTracerCameraShutterSeconds(renderSettings);
    const float iso = safePathTracerCameraIso(renderSettings);
    const float exposureCompensation = safePathTracerCameraExposureCompensationEv(renderSettings);

    const float relativeExposure = (shutterSeconds * iso) / std::max(apertureFNumber * apertureFNumber, 1e-6f);
    const float referenceExposure = (kReferenceShutterSeconds * kReferenceIso) /
                                    (kReferenceApertureFNumber * kReferenceApertureFNumber);
    const float compensationMultiplier = std::pow(2.0f, exposureCompensation);
    return std::max((relativeExposure / referenceExposure) * compensationMultiplier, 0.0f);
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
