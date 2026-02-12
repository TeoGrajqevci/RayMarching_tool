#ifndef RENDERSETTINGS_H
#define RENDERSETTINGS_H

enum CameraProjectionMode {
    CAMERA_PROJECTION_PERSPECTIVE = 0,
    CAMERA_PROJECTION_ORTHOGRAPHIC = 1
};

struct RenderSettings {
    float cameraFovDegrees = 90.0f;
    int cameraProjectionMode = CAMERA_PROJECTION_PERSPECTIVE;

    int pathTracerMaxBounces = 6;

    bool denoiserEnabled = true;
    int denoiseStartSample = 8;
    int denoiseInterval = 8;
};

#endif // RENDERSETTINGS_H
