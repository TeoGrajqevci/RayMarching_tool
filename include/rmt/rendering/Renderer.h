#ifndef RMT_RENDERING_RENDERER_H
#define RMT_RENDERING_RENDERER_H

#include <glad/glad.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "rmt/input/Input.h"
#include "rmt/RenderSettings.h"
#include "rmt/rendering/ShaderProgram.h"
#include "rmt/scene/Shape.h"

#ifdef RMT_HAS_OIDN
#include <OpenImageDenoise/oidn.hpp>
#endif

namespace rmt {

struct ShaderPerfCounters {
    double avgMarchSteps = 0.0;
    double maxMarchSteps = 0.0;
    double avgShadowSteps = 0.0;
    double maxShadowSteps = 0.0;
    double avgTransmissionSteps = 0.0;
    double maxTransmissionSteps = 0.0;
    double hitRate = 0.0;
    double thresholdHitRate = 0.0;
    double refinedHitRate = 0.0;
    double missRate = 0.0;
};

struct RendererPerfSnapshot {
    double cpuFrameMs = 0.0;
    double cpuSceneUploadMs = 0.0;
    double cpuMeshUploadMs = 0.0;
    double cpuSetupMs = 0.0;
    double cpuDrawSubmitMs = 0.0;
    double gpuSolidMs = 0.0;
    double gpuPathMs = 0.0;
    double gpuDenoiseMs = 0.0;
    ShaderPerfCounters shaderCounters;
};

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
    const RendererPerfSnapshot& getLastPerfSnapshot() const { return lastPerfSnapshot; }
    void setPerfLoggingEnabled(bool enabled) { perfLoggingEnabled = enabled; }
    bool getPerfLoggingEnabled() const { return perfLoggingEnabled; }
    bool collectDebugCountersFromFramebuffer(int width, int height);
    void resolvePerfTimersBlocking();

private:
    void uploadCommonUniforms(const Shader& activeShader,
                              const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                              const float backgroundColor[3],
                              float ambientIntensity, float directLightIntensity,
                              const float cameraPos[3], const float cameraTarget[3],
                              int display_w, int display_h,
                              bool useGradientBackground,
                              const RenderSettings& renderSettings,
                              const TransformationState& transformState) const;
    void ensureSceneBuffer(const std::vector<Shape>& shapes);
    void ensureMeshSdfBuffer();
    void buildAccelerationGrid(const std::vector<RuntimeShapeData>& runtimeShapes, int shapeCount);
    void bindSceneBuffer(const Shader& activeShader, int textureUnit) const;
    void bindMaterialTextures(const Shader& activeShader, int textureUnit) const;
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
                                   const RenderSettings& renderSettings,
                                   const TransformationState& transformState) const;
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
    GLuint meshSdfBufferObject;
    GLuint meshSdfBufferTexture;
    GLuint curveNodeBufferObject;
    GLuint curveNodeBufferTexture;
    int materialTextureCount;
    GLuint materialTextureIds[8];
    bool materialTextureOverflowWarningIssued;
    std::vector<float> packedSceneTexels;
    std::vector<float> packedMeshSdfTexels;
    std::vector<float> packedCurveNodeTexels;
    std::vector<float> accelCellRangeTexels;
    std::vector<float> accelShapeIndexTexels;
    int sceneTexelStride;
    int meshSdfResolution;
    int meshSdfCount;
    int curveNodeCount;
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
    std::uint64_t meshSdfBufferVersion;
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
    RendererPerfSnapshot lastPerfSnapshot;
    double cpuFrameAccumulatedMs;
    int cpuFrameAccumulatedCount;
    double lastPerfLogTime;
    bool perfLoggingEnabled;
    double cpuSceneUploadMs;
    double cpuMeshUploadMs;
    double cpuSetupMs;
    double cpuDrawSubmitMs;
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

} // namespace rmt

#endif // RMT_RENDERING_RENDERER_H
