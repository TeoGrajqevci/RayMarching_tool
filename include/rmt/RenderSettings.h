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

    bool denoiserEnabled = true;
    int denoiseStartSample = 8;
    int denoiseInterval = 8;

    int debugOutputMode = RENDER_DEBUG_OUTPUT_NONE;
};

} // namespace rmt

#endif // RMT_RENDER_SETTINGS_H
