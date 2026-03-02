#include "rmt/rendering/Renderer.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "RendererInternal.h"
#include "rmt/rendering/Texture2D.h"

namespace rmt {

using namespace renderer_internal;

namespace {

#ifdef RMT_HAS_OIDN
const char* deviceTypeName(oidn::DeviceType type) {
    switch (type) {
        case oidn::DeviceType::CPU:   return "CPU";
        case oidn::DeviceType::SYCL:  return "SYCL";
        case oidn::DeviceType::CUDA:  return "CUDA";
        case oidn::DeviceType::HIP:   return "HIP";
        case oidn::DeviceType::Metal: return "Metal";
        default: return "Default";
    }
}
#endif

} // namespace

Renderer::Renderer(GLuint vao)
    : quadVAO(vao),
      solidShader(new Shader("shaders/vertex.glsl", "shaders/Solid_renderer.glsl")),
      pathTracerShader(new Shader("shaders/vertex.glsl", "shaders/Pathtracer.glsl")),
      pathTracerDisplayShader(new Shader("shaders/vertex.glsl", "shaders/Display_texture.glsl")),
      sceneBufferObject(0),
      sceneBufferTexture(0),
      accelCellRangeBufferObject(0),
      accelCellRangeTexture(0),
      accelShapeIndexBufferObject(0),
      accelShapeIndexTexture(0),
      meshSdfBufferObject(0),
      meshSdfBufferTexture(0),
      curveNodeBufferObject(0),
      curveNodeBufferTexture(0),
      materialTextureCount(0),
      materialTextureIds{0},
      materialTextureOverflowWarningIssued(false),
      packedSceneTexels(),
      packedMeshSdfTexels(),
      packedCurveNodeTexels(),
      accelCellRangeTexels(),
      accelShapeIndexTexels(),
      sceneTexelStride(kShapeTexelStride),
      meshSdfResolution(0),
      meshSdfCount(0),
      curveNodeCount(0),
      maxTextureBufferTexels(0),
      maxSceneShapes(0),
      uploadedShapeCount(0),
      accelGridDim{1, 1, 1},
      accelGridMin{0.0f, 0.0f, 0.0f},
      accelGridInvCellSize{1.0f, 1.0f, 1.0f},
      accelGridEnabled(false),
      accelCellCount(1),
      accelFallbackCellCount(0),
      accelShapeIndexCount(0),
      accelAverageCandidates(0.0f),
      sceneBoundsCenter{0.0f, 0.0f, 0.0f},
      sceneBoundsRadius(0.0f),
      sceneBufferHash(0),
      meshSdfBufferVersion(0),
      sceneBufferReady(false),
      lastCapacityWarningShapeCount(-1),
      accumulationFBO(0),
      accumulationTextures{0, 0},
      denoisedTexture(0),
      denoiseReadbackPBOs{0, 0},
      denoiseReadbackReady{false, false},
      denoiseReadbackIndex(0),
      hasDenoisedFrame(false),
      accumulationWidth(0),
      accumulationHeight(0),
      accumulationWriteIndex(0),
      pathSampleCount(0),
      accumulationInitialized(false),
      lastSceneHash(0),
      denoiserAvailable(false),
      denoiserUsingGPU(false),
      solidGpuTimer{{0, 0, 0, 0}, 0, 0, 0.0},
      pathGpuTimer{{0, 0, 0, 0}, 0, 0, 0.0},
      denoiseGpuTimer{{0, 0, 0, 0}, 0, 0, 0.0},
      lastPerfSnapshot(),
      cpuFrameAccumulatedMs(0.0),
      cpuFrameAccumulatedCount(0),
      lastPerfLogTime(0.0),
      perfLoggingEnabled(true),
      cpuSceneUploadMs(0.0),
      cpuMeshUploadMs(0.0),
      cpuSetupMs(0.0),
      cpuDrawSubmitMs(0.0),
      lastPrimitiveEvalCount(0),
      lastShapeEvalCount(0),
      lastSceneEvalCount(0) {
    glGenFramebuffers(1, &accumulationFBO);

    glGenBuffers(1, &sceneBufferObject);
    glGenTextures(1, &sceneBufferTexture);
    glBindTexture(GL_TEXTURE_BUFFER, sceneBufferTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, sceneBufferObject);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glGenBuffers(1, &accelCellRangeBufferObject);
    glGenTextures(1, &accelCellRangeTexture);
    glBindTexture(GL_TEXTURE_BUFFER, accelCellRangeTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, accelCellRangeBufferObject);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glGenBuffers(1, &accelShapeIndexBufferObject);
    glGenTextures(1, &accelShapeIndexTexture);
    glBindTexture(GL_TEXTURE_BUFFER, accelShapeIndexTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, accelShapeIndexBufferObject);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glGenBuffers(1, &meshSdfBufferObject);
    glGenTextures(1, &meshSdfBufferTexture);
    glBindTexture(GL_TEXTURE_BUFFER, meshSdfBufferTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, meshSdfBufferObject);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glGenBuffers(1, &curveNodeBufferObject);
    glGenTextures(1, &curveNodeBufferTexture);
    glBindTexture(GL_TEXTURE_BUFFER, curveNodeBufferTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, curveNodeBufferObject);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &maxTextureBufferTexels);
    maxSceneShapes = std::max(1, maxTextureBufferTexels / sceneTexelStride);

    glGenQueries(kTimerQueryCount, solidGpuTimer.queries);
    glGenQueries(kTimerQueryCount, pathGpuTimer.queries);
    glGenQueries(kTimerQueryCount, denoiseGpuTimer.queries);

#ifdef RMT_HAS_OIDN
    oidnBufferByteSize = 0;
    try {
        oidn::DeviceType selectedType = oidn::DeviceType::Default;
        std::string selectedName;
        std::ostringstream gpuInitReport;
        bool gpuCandidateDetected = false;

        auto commitGpuCandidate = [&](oidn::DeviceRef candidate,
                                      oidn::DeviceType requestedType,
                                      const std::string& candidateName,
                                      const std::string& sourceLabel) -> bool {
            if (!candidate) {
                gpuInitReport << " - " << sourceLabel << " [" << deviceTypeName(requestedType) << "]"
                              << ": device creation failed\n";
                return false;
            }

            candidate.commit();
            const char* candidateError = nullptr;
            if (candidate.getError(candidateError) != oidn::Error::None) {
                gpuInitReport << " - " << sourceLabel << " [" << deviceTypeName(requestedType) << "]";
                if (!candidateName.empty()) {
                    gpuInitReport << " \"" << candidateName << "\"";
                }
                gpuInitReport << ": "
                              << (candidateError != nullptr ? candidateError : "unknown device error")
                              << "\n";
                return false;
            }

            oidn::DeviceType actualType = requestedType;
            try {
                actualType = candidate.get<oidn::DeviceType>("type");
            } catch (...) {
            }

            if (actualType == oidn::DeviceType::CPU) {
                gpuInitReport << " - " << sourceLabel << " [" << deviceTypeName(requestedType) << "]";
                if (!candidateName.empty()) {
                    gpuInitReport << " \"" << candidateName << "\"";
                }
                gpuInitReport << ": resolved to CPU device (rejected)\n";
                return false;
            }

            selectedType = actualType;
            selectedName = candidateName;
            oidnDevice = candidate;
            return true;
        };

        const oidn::DeviceType explicitGpuTypes[] = {
            oidn::DeviceType::Metal,
            oidn::DeviceType::CUDA,
            oidn::DeviceType::HIP,
            oidn::DeviceType::SYCL,
        };

        for (const oidn::DeviceType gpuType : explicitGpuTypes) {
            oidn::DeviceRef explicitCandidate = oidn::newDevice(gpuType);
            if (commitGpuCandidate(explicitCandidate, gpuType, std::string(), "explicit backend")) {
                gpuCandidateDetected = true;
                break;
            }
        }

        if (!oidnDevice) {
            const int physicalDeviceCount = oidn::getNumPhysicalDevices();
            for (int i = 0; i < physicalDeviceCount; ++i) {
                oidn::PhysicalDeviceRef physicalDevice(i);
                const oidn::DeviceType type = physicalDevice.get<oidn::DeviceType>("type");
                if (type == oidn::DeviceType::CPU) {
                    continue;
                }

                gpuCandidateDetected = true;

                std::string candidateName;
                try {
                    candidateName = physicalDevice.get<std::string>("name");
                } catch (...) {
                }

                oidn::DeviceRef candidate = physicalDevice.newDevice();
                if (commitGpuCandidate(candidate, type, candidateName, "physical device")) {
                    break;
                }
            }
        }

        if (!oidnDevice) {
            std::ostringstream error;
            error << "OIDN GPU init failed: no usable GPU device found.";
            if (!gpuCandidateDetected) {
                error << " No non-CPU OIDN physical devices were detected.";
            }
            if (!gpuInitReport.str().empty()) {
                error << "\nOIDN GPU probe details:\n" << gpuInitReport.str();
            }
#if defined(__APPLE__)
            error << "\nHint: installed OIDN appears CPU-only. Install/build OpenImageDenoise with the Metal backend (libOpenImageDenoise_device_metal*.dylib).";
#endif
            throw std::runtime_error(error.str());
        }

        denoiserUsingGPU = true;

        oidnFilter = oidnDevice.newFilter("RT");
        denoiserAvailable = true;

        std::cout << "OIDN initialized on device [" << deviceTypeName(selectedType) << "]";
        if (!selectedName.empty()) {
            std::cout << " \"" << selectedName << "\"";
        }
        std::cout << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "OIDN init failed: " << e.what() << std::endl;
        denoiserAvailable = false;
        denoiserUsingGPU = false;
    }
#endif
}

Renderer::~Renderer() {
    if (solidGpuTimer.queries[0] != 0) {
        glDeleteQueries(kTimerQueryCount, solidGpuTimer.queries);
    }
    if (pathGpuTimer.queries[0] != 0) {
        glDeleteQueries(kTimerQueryCount, pathGpuTimer.queries);
    }
    if (denoiseGpuTimer.queries[0] != 0) {
        glDeleteQueries(kTimerQueryCount, denoiseGpuTimer.queries);
    }

    if (denoiseReadbackPBOs[0] != 0 || denoiseReadbackPBOs[1] != 0) {
        glDeleteBuffers(2, denoiseReadbackPBOs);
        denoiseReadbackPBOs[0] = 0;
        denoiseReadbackPBOs[1] = 0;
    }

    if (sceneBufferTexture != 0) {
        glDeleteTextures(1, &sceneBufferTexture);
        sceneBufferTexture = 0;
    }
    if (accelCellRangeTexture != 0) {
        glDeleteTextures(1, &accelCellRangeTexture);
        accelCellRangeTexture = 0;
    }
    if (accelShapeIndexTexture != 0) {
        glDeleteTextures(1, &accelShapeIndexTexture);
        accelShapeIndexTexture = 0;
    }
    if (meshSdfBufferTexture != 0) {
        glDeleteTextures(1, &meshSdfBufferTexture);
        meshSdfBufferTexture = 0;
    }
    if (curveNodeBufferTexture != 0) {
        glDeleteTextures(1, &curveNodeBufferTexture);
        curveNodeBufferTexture = 0;
    }

    if (sceneBufferObject != 0) {
        glDeleteBuffers(1, &sceneBufferObject);
        sceneBufferObject = 0;
    }
    if (accelCellRangeBufferObject != 0) {
        glDeleteBuffers(1, &accelCellRangeBufferObject);
        accelCellRangeBufferObject = 0;
    }
    if (accelShapeIndexBufferObject != 0) {
        glDeleteBuffers(1, &accelShapeIndexBufferObject);
        accelShapeIndexBufferObject = 0;
    }
    if (meshSdfBufferObject != 0) {
        glDeleteBuffers(1, &meshSdfBufferObject);
        meshSdfBufferObject = 0;
    }
    if (curveNodeBufferObject != 0) {
        glDeleteBuffers(1, &curveNodeBufferObject);
        curveNodeBufferObject = 0;
    }

    if (accumulationTextures[0] != 0 || accumulationTextures[1] != 0) {
        glDeleteTextures(2, accumulationTextures);
        accumulationTextures[0] = 0;
        accumulationTextures[1] = 0;
    }

    if (denoisedTexture != 0) {
        glDeleteTextures(1, &denoisedTexture);
        denoisedTexture = 0;
    }

    if (accumulationFBO != 0) {
        glDeleteFramebuffers(1, &accumulationFBO);
        accumulationFBO = 0;
    }

    releaseMaterialTextureCache();
}

void Renderer::renderScene(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes,
                           const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                           const float backgroundColor[3],
                           float ambientIntensity, float directLightIntensity,
                           const float cameraPos[3], const float cameraTarget[3],
                           int display_w, int display_h,
                           bool altRenderMode, bool useGradientBackground,
                           bool usePathTracer,
                           const RenderSettings& renderSettings,
                           const TransformationState& transformState) {
    const std::chrono::steady_clock::time_point frameStart = std::chrono::steady_clock::now();

    // Pre-warm path tracer GPU resources so first renderer mode switch is instant.
    // After first allocation this is a cheap no-op until viewport size changes.
    ensurePathTracerResources(display_w, display_h);

    if (usePathTracer) {
        solidGpuTimer.lastMilliseconds = 0.0;
        renderPathTracer(shapes,
                         selectedShapes,
                         lightDir, lightColor, ambientColor,
                         backgroundColor, ambientIntensity, directLightIntensity,
                         cameraPos, cameraTarget,
                         display_w, display_h,
                         useGradientBackground,
                         renderSettings,
                         transformState);
    } else {
        pathGpuTimer.lastMilliseconds = 0.0;
        denoiseGpuTimer.lastMilliseconds = 0.0;
        renderSolid(shapes, selectedShapes,
                    lightDir, lightColor, ambientColor,
                    backgroundColor, ambientIntensity, directLightIntensity,
                    cameraPos, cameraTarget,
                    display_w, display_h,
                    altRenderMode, useGradientBackground,
                    renderSettings,
                    transformState);
    }

    processPerfTimers();

    const std::chrono::steady_clock::time_point frameEnd = std::chrono::steady_clock::now();
    const double frameMs = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(frameEnd - frameStart).count();
    cpuFrameAccumulatedMs += frameMs;
    ++cpuFrameAccumulatedCount;

    lastPerfSnapshot.cpuFrameMs = frameMs;
    lastPerfSnapshot.cpuSceneUploadMs = cpuSceneUploadMs;
    lastPerfSnapshot.cpuMeshUploadMs = cpuMeshUploadMs;
    lastPerfSnapshot.cpuSetupMs = cpuSetupMs;
    lastPerfSnapshot.cpuDrawSubmitMs = cpuDrawSubmitMs;
    lastPerfSnapshot.gpuSolidMs = solidGpuTimer.lastMilliseconds;
    lastPerfSnapshot.gpuPathMs = pathGpuTimer.lastMilliseconds;
    lastPerfSnapshot.gpuDenoiseMs = denoiseGpuTimer.lastMilliseconds;

    logPerfIfNeeded(shapes);
}

} // namespace rmt
