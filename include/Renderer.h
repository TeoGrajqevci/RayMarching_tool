#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>
#include <array>
#include "Shader.h"
#include "Shapes.h"
#include "RenderSettings.h"
#include "Input.h" // Pour TransformationState
#ifdef RMT_HAS_OIDN
#include <OpenImageDenoise/oidn.hpp>
#endif

class Renderer {
public:
    explicit Renderer(GLuint vao);
    ~Renderer();

    void renderScene(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes,
                     const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                     const float backgroundColor[3],
                     float ambientIntensity, float directLightIntensity,
                     const float cameraPos[3], const float cameraTarget[3],
                     int display_w, int display_h,
                     bool altRenderMode, bool useGradientBackground,
                     bool usePathTracer,
                     const RenderSettings& renderSettings,
                     const TransformationState& transformState);

    bool isDenoiserAvailable() const { return denoiserAvailable; }
    bool isDenoiserUsingGPU() const { return denoiserUsingGPU; }
    int getPathSampleCount() const { return pathSampleCount; }

private:
    void uploadCommonUniforms(const Shader& activeShader,
                              const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                              const float backgroundColor[3],
                              float ambientIntensity, float directLightIntensity,
                              const float cameraPos[3], const float cameraTarget[3],
                              int display_w, int display_h,
                              bool useGradientBackground,
                              const RenderSettings& renderSettings) const;
    void ensureSceneBuffer(const std::vector<Shape>& shapes);
    void buildAccelerationGrid(const std::vector<RuntimeShapeData>& runtimeShapes, int shapeCount);
    void bindSceneBuffer(const Shader& activeShader, int textureUnit) const;
    void renderSolid(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes,
                     const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                     const float backgroundColor[3],
                     float ambientIntensity, float directLightIntensity,
                     const float cameraPos[3], const float cameraTarget[3],
                     int display_w, int display_h,
                     bool altRenderMode, bool useGradientBackground,
                     const RenderSettings& renderSettings,
                     const TransformationState& transformState);
    void renderPathTracer(const std::vector<Shape>& shapes,
                          const std::vector<int>& selectedShapes,
                          const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                          const float backgroundColor[3],
                          float ambientIntensity, float directLightIntensity,
                          const float cameraPos[3], const float cameraTarget[3],
                          int display_w, int display_h,
                          bool useGradientBackground,
                          const RenderSettings& renderSettings,
                          const TransformationState& transformState);

    void ensurePathTracerResources(int display_w, int display_h);
    void resetPathTracerAccumulation();
    std::uint64_t computeSceneHash(const std::vector<Shape>& shapes,
                                   const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                                   const float backgroundColor[3],
                                   float ambientIntensity, float directLightIntensity,
                                   const float cameraPos[3], const float cameraTarget[3],
                                   int display_w, int display_h,
                                   bool useGradientBackground,
                                   const RenderSettings& renderSettings) const;
    std::uint64_t computeShapeBufferHash(const std::vector<Shape>& shapes) const;
    void processPerfTimers();
    void logPerfIfNeeded(const std::vector<Shape>& shapes);

    GLuint quadVAO;
    std::unique_ptr<Shader> solidShader;
    std::unique_ptr<Shader> pathTracerShader;
    std::unique_ptr<Shader> pathTracerDisplayShader;

    GLuint sceneBufferObject;
    GLuint sceneBufferTexture;
    GLuint accelCellRangeBufferObject;
    GLuint accelCellRangeTexture;
    GLuint accelShapeIndexBufferObject;
    GLuint accelShapeIndexTexture;
    std::vector<float> packedSceneTexels;
    std::vector<float> accelCellRangeTexels;
    std::vector<float> accelShapeIndexTexels;
    int sceneTexelStride;
    int maxTextureBufferTexels;
    int maxSceneShapes;
    int uploadedShapeCount;
    std::array<int, 3> accelGridDim;
    std::array<float, 3> accelGridMin;
    std::array<float, 3> accelGridInvCellSize;
    bool accelGridEnabled;
    int accelCellCount;
    int accelFallbackCellCount;
    int accelShapeIndexCount;
    float accelAverageCandidates;
    std::array<float, 3> sceneBoundsCenter;
    float sceneBoundsRadius;
    std::uint64_t sceneBufferHash;
    bool sceneBufferReady;
    int lastCapacityWarningShapeCount;

    GLuint accumulationFBO;
    GLuint accumulationTextures[2];
    GLuint denoisedTexture;
    GLuint denoiseReadbackPBOs[2];
    bool denoiseReadbackReady[2];
    int denoiseReadbackIndex;
    bool hasDenoisedFrame;
    int accumulationWidth;
    int accumulationHeight;
    int accumulationWriteIndex;
    int pathSampleCount;
    bool accumulationInitialized;
    std::uint64_t lastSceneHash;

    std::vector<float> denoiseInput;
    std::vector<float> denoiseOutput;
    bool denoiserAvailable;
    bool denoiserUsingGPU;

    struct GpuTimerState {
        GLuint queries[4];
        int writeIndex;
        int initializedCount;
        double lastMilliseconds;
    };

    GpuTimerState solidGpuTimer;
    GpuTimerState pathGpuTimer;
    GpuTimerState denoiseGpuTimer;
    double cpuFrameAccumulatedMs;
    int cpuFrameAccumulatedCount;
    double lastPerfLogTime;
    std::uint64_t lastPrimitiveEvalCount;
    std::uint64_t lastShapeEvalCount;
    std::uint64_t lastSceneEvalCount;
#ifdef RMT_HAS_OIDN
    oidn::DeviceRef oidnDevice;
    oidn::FilterRef oidnFilter;
    oidn::BufferRef oidnColorBuffer;
    oidn::BufferRef oidnOutputBuffer;
    std::size_t oidnBufferByteSize;
#endif
};

#endif // RENDERER_H
