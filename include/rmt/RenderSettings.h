#ifndef RMT_RENDER_SETTINGS_H
#define RMT_RENDER_SETTINGS_H

namespace rmt {

enum CameraProjectionMode {
    CAMERA_PROJECTION_PERSPECTIVE = 0,
    CAMERA_PROJECTION_ORTHOGRAPHIC = 1
};

enum RayMarchQualityLevel {
    RAYMARCH_QUALITY_ULTRA = 0,
    RAYMARCH_QUALITY_HIGH = 1,
    RAYMARCH_QUALITY_MEDIUM = 2,
    RAYMARCH_QUALITY_LOW = 3
};

enum RenderDebugOutputMode {
    RENDER_DEBUG_OUTPUT_NONE = 0,
    RENDER_DEBUG_OUTPUT_COUNTERS = 1
};

struct RenderSettings {
    float cameraFovDegrees = 90.0f;
    int cameraProjectionMode = CAMERA_PROJECTION_PERSPECTIVE;
    int rayMarchQuality = RAYMARCH_QUALITY_HIGH;
    bool optimizedTracingEnabled = true;

    int pathTracerMaxBounces = 6;
    bool pathTracerUseFixedSeed = false;
    int pathTracerFixedSeed = 1337;
    bool pathTracerGuidedSamplingEnabled = true;
    float pathTracerGuidedSamplingMix = 0.2f;
    float pathTracerMisPower = 1.0f;
    int pathTracerRussianRouletteStartBounce = 2;
    float pathTracerResolutionScale = 1.0f;
    bool pathTracerPhysicalCameraEnabled = false;
    float pathTracerCameraFocalLengthMm = 50.0f;
    float pathTracerCameraSensorWidthMm = 36.0f;
    float pathTracerCameraSensorHeightMm = 24.0f;
    float pathTracerCameraApertureFNumber = 8.0f;
    float pathTracerCameraFocusDistance = 5.0f;
    int pathTracerCameraBladeCount = 0;
    float pathTracerCameraBladeRotationDegrees = 0.0f;
    float pathTracerCameraAnamorphicRatio = 1.0f;
    float pathTracerCameraChromaticAberration = 0.0f;
    float pathTracerCameraShutterSeconds = 1.0f / 125.0f;
    float pathTracerCameraIso = 100.0f;
    float pathTracerCameraExposureCompensationEv = 0.0f;

    bool denoiserEnabled = true;
    int denoiseStartSample = 8;
    int denoiseInterval = 8;

    int debugOutputMode = RENDER_DEBUG_OUTPUT_NONE;
};

} // namespace rmt

#endif // RMT_RENDER_SETTINGS_H
