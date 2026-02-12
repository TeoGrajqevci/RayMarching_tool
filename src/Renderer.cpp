#include "Renderer.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

const int kMaxSelected = 128;
const int kShapeTexelStride = 14;
const int kTimerQueryCount = 4;
const int kAccelMinDim = 4;
const int kAccelMaxDim = 96;
const int kAccelMaxCells = 131072;
const int kAccelFallbackCellCandidateCount = 512;

void hashRaw(std::uint64_t& hash, const void* data, std::size_t size) {
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= static_cast<std::uint64_t>(bytes[i]);
        hash *= 1099511628211ULL;
    }
}

template <typename T>
void hashValue(std::uint64_t& hash, const T& value) {
    hashRaw(hash, &value, sizeof(T));
}

inline std::size_t shapeTexelBaseOffset(int shapeIndex, int texelSlot) {
    return static_cast<std::size_t>(shapeIndex * kShapeTexelStride + texelSlot) * 4u;
}

inline void writePackedTexel(std::vector<float>& buffer,
                             int shapeIndex,
                             int texelSlot,
                             float x, float y, float z, float w) {
    const std::size_t base = shapeTexelBaseOffset(shapeIndex, texelSlot);
    buffer[base + 0] = x;
    buffer[base + 1] = y;
    buffer[base + 2] = z;
    buffer[base + 3] = w;
}

float clampf(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

int clampi(int value, int minValue, int maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

float safeCameraFovDegrees(const RenderSettings& renderSettings) {
    return clampf(renderSettings.cameraFovDegrees, 20.0f, 120.0f);
}

int safeCameraProjectionMode(const RenderSettings& renderSettings) {
    if (renderSettings.cameraProjectionMode == CAMERA_PROJECTION_ORTHOGRAPHIC) {
        return CAMERA_PROJECTION_ORTHOGRAPHIC;
    }
    return CAMERA_PROJECTION_PERSPECTIVE;
}

int safePathTracerMaxBounces(const RenderSettings& renderSettings) {
    return clampi(renderSettings.pathTracerMaxBounces, 1, 12);
}

int safeDenoiseStartSample(const RenderSettings& renderSettings) {
    return clampi(renderSettings.denoiseStartSample, 1, 4096);
}

int safeDenoiseInterval(const RenderSettings& renderSettings) {
    return clampi(renderSettings.denoiseInterval, 1, 1024);
}

float computeCameraOrthoScale(const float cameraPos[3], const float cameraTarget[3], float fovDegrees) {
    const float dx = cameraPos[0] - cameraTarget[0];
    const float dy = cameraPos[1] - cameraTarget[1];
    const float dz = cameraPos[2] - cameraTarget[2];
    const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    const float halfFovRadians = fovDegrees * 0.5f * (3.14159265358979323846f / 180.0f);
    return std::max(0.05f, distance * std::tan(halfFovRadians));
}

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
      packedSceneTexels(),
      accelCellRangeTexels(),
      accelShapeIndexTexels(),
      sceneTexelStride(kShapeTexelStride),
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
      cpuFrameAccumulatedMs(0.0),
      cpuFrameAccumulatedCount(0),
      lastPerfLogTime(0.0),
      lastPrimitiveEvalCount(0),
      lastShapeEvalCount(0),
      lastSceneEvalCount(0)
{
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

    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &maxTextureBufferTexels);
    maxSceneShapes = std::max(1, maxTextureBufferTexels / sceneTexelStride);

    glGenQueries(kTimerQueryCount, solidGpuTimer.queries);
    glGenQueries(kTimerQueryCount, pathGpuTimer.queries);
    glGenQueries(kTimerQueryCount, denoiseGpuTimer.queries);

#ifdef RMT_HAS_OIDN
    oidnBufferByteSize = 0;
    try {
        oidn::DeviceType selectedType = oidn::DeviceType::CPU;
        std::string selectedName;

        const int physicalDeviceCount = oidn::getNumPhysicalDevices();
        for (int i = 0; i < physicalDeviceCount; ++i) {
            oidn::PhysicalDeviceRef physicalDevice(i);
            const oidn::DeviceType type = physicalDevice.get<oidn::DeviceType>("type");
            if (type == oidn::DeviceType::CPU) {
                continue;
            }

            oidn::DeviceRef candidate = physicalDevice.newDevice();
            if (!candidate) {
                continue;
            }

            selectedType = type;
            selectedName = physicalDevice.get<std::string>("name");
            oidnDevice = candidate;
            break;
        }

        if (!oidnDevice) {
            throw std::runtime_error("No OIDN GPU device found. CPU denoiser fallback is disabled.");
        }

        oidnDevice.commit();
        const char* deviceError = nullptr;
        if (oidnDevice.getError(deviceError) != oidn::Error::None) {
            throw std::runtime_error(deviceError != nullptr ? deviceError : "Unknown OIDN device error");
        }

        oidnFilter = oidnDevice.newFilter("RT");
        denoiserAvailable = true;
        denoiserUsingGPU = true;

        std::cout << "OIDN initialized on GPU device [" << deviceTypeName(selectedType) << "]";
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
}

void Renderer::uploadCommonUniforms(const Shader& activeShader,
                                    const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                                    const float backgroundColor[3],
                                    float ambientIntensity, float directLightIntensity,
                                    const float cameraPos[3], const float cameraTarget[3],
                                    int display_w, int display_h,
                                    bool useGradientBackground,
                                    const RenderSettings& renderSettings) const {
    const float cameraFovDegrees = safeCameraFovDegrees(renderSettings);
    const int cameraProjectionMode = safeCameraProjectionMode(renderSettings);
    const float cameraOrthoScale = computeCameraOrthoScale(cameraPos, cameraTarget, cameraFovDegrees);

    activeShader.setVec2("iResolution", static_cast<float>(display_w), static_cast<float>(display_h));
    activeShader.setFloat("iTime", static_cast<float>(glfwGetTime()));
    activeShader.setVec3("lightDir", lightDir[0], lightDir[1], lightDir[2]);
    activeShader.setVec3("lightColor", lightColor[0], lightColor[1], lightColor[2]);
    activeShader.setVec3("ambientColor", ambientColor[0], ambientColor[1], ambientColor[2]);
    activeShader.setVec3("backgroundColor", backgroundColor[0], backgroundColor[1], backgroundColor[2]);
    activeShader.setFloat("ambientIntensity", std::max(ambientIntensity, 0.0f));
    activeShader.setFloat("directLightIntensity", std::max(directLightIntensity, 0.0f));
    activeShader.setVec3("cameraPos", cameraPos[0], cameraPos[1], cameraPos[2]);
    activeShader.setVec3("cameraTarget", cameraTarget[0], cameraTarget[1], cameraTarget[2]);
    activeShader.setFloat("uCameraFovDegrees", cameraFovDegrees);
    activeShader.setInt("uCameraProjectionMode", cameraProjectionMode);
    activeShader.setFloat("uCameraOrthoScale", cameraOrthoScale);
    activeShader.setInt("uBackgroundGradient", useGradientBackground ? 1 : 0);
}

std::uint64_t Renderer::computeShapeBufferHash(const std::vector<Shape>& shapes) const {
    std::uint64_t hash = 1469598103934665603ULL;

    const int shapeCount = static_cast<int>(shapes.size());
    hashValue(hash, shapeCount);
    for (int i = 0; i < shapeCount; ++i) {
        const Shape& shape = shapes[static_cast<std::size_t>(i)];
        hashValue(hash, shape.type);
        hashRaw(hash, shape.center, sizeof(shape.center));
        hashRaw(hash, shape.param, sizeof(shape.param));
        hashRaw(hash, shape.rotation, sizeof(shape.rotation));
        hashValue(hash, shape.extra);
        hashRaw(hash, shape.scale, sizeof(shape.scale));
        hashValue(hash, shape.scaleMode);
        hashRaw(hash, shape.elongation, sizeof(shape.elongation));

        float twist[3] = {
            shape.twistModifierEnabled ? shape.twist[0] : 0.0f,
            shape.twistModifierEnabled ? shape.twist[1] : 0.0f,
            shape.twistModifierEnabled ? shape.twist[2] : 0.0f
        };
        float bend[3] = {
            shape.bendModifierEnabled ? shape.bend[0] : 0.0f,
            shape.bendModifierEnabled ? shape.bend[1] : 0.0f,
            shape.bendModifierEnabled ? shape.bend[2] : 0.0f
        };
        float mirror[3] = {
            (shape.mirrorModifierEnabled && shape.mirrorAxis[0]) ? 1.0f : 0.0f,
            (shape.mirrorModifierEnabled && shape.mirrorAxis[1]) ? 1.0f : 0.0f,
            (shape.mirrorModifierEnabled && shape.mirrorAxis[2]) ? 1.0f : 0.0f
        };
        float mirrorOffset[3] = {
            (shape.mirrorModifierEnabled && shape.mirrorAxis[0]) ? shape.mirrorOffset[0] : 0.0f,
            (shape.mirrorModifierEnabled && shape.mirrorAxis[1]) ? shape.mirrorOffset[1] : 0.0f,
            (shape.mirrorModifierEnabled && shape.mirrorAxis[2]) ? shape.mirrorOffset[2] : 0.0f
        };
        const float mirrorSmoothness = shape.mirrorModifierEnabled ? std::max(shape.mirrorSmoothness, 0.0f) : 0.0f;

        hashRaw(hash, twist, sizeof(twist));
        hashRaw(hash, bend, sizeof(bend));
        hashRaw(hash, mirror, sizeof(mirror));
        hashRaw(hash, mirrorOffset, sizeof(mirrorOffset));
        hashValue(hash, mirrorSmoothness);

        hashValue(hash, shape.blendOp);
        hashValue(hash, shape.smoothness);
        hashRaw(hash, shape.albedo, sizeof(shape.albedo));
        hashValue(hash, shape.metallic);
        hashValue(hash, shape.roughness);
        hashRaw(hash, shape.emission, sizeof(shape.emission));
        hashValue(hash, shape.emissionStrength);
        hashValue(hash, shape.transmission);
        hashValue(hash, shape.ior);
    }

    return hash;
}

void Renderer::buildAccelerationGrid(const std::vector<RuntimeShapeData>& runtimeShapes, int shapeCount) {
    accelGridEnabled = false;
    accelGridDim = {1, 1, 1};
    accelGridMin = {sceneBoundsCenter[0], sceneBoundsCenter[1], sceneBoundsCenter[2]};
    accelGridInvCellSize = {1.0f, 1.0f, 1.0f};
    accelCellCount = 1;
    accelFallbackCellCount = 0;
    accelShapeIndexCount = 0;
    accelAverageCandidates = 0.0f;

    accelCellRangeTexels.assign(4u, 0.0f);
    accelShapeIndexTexels.assign(4u, 0.0f);

    if (shapeCount <= 8 || sceneBoundsRadius <= 1e-5f) {
        glBindBuffer(GL_TEXTURE_BUFFER, accelCellRangeBufferObject);
        glBufferData(GL_TEXTURE_BUFFER,
                     static_cast<GLsizeiptr>(accelCellRangeTexels.size() * sizeof(float)),
                     accelCellRangeTexels.data(),
                     GL_DYNAMIC_DRAW);
        glBindTexture(GL_TEXTURE_BUFFER, accelCellRangeTexture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, accelCellRangeBufferObject);

        glBindBuffer(GL_TEXTURE_BUFFER, accelShapeIndexBufferObject);
        glBufferData(GL_TEXTURE_BUFFER,
                     static_cast<GLsizeiptr>(accelShapeIndexTexels.size() * sizeof(float)),
                     accelShapeIndexTexels.data(),
                     GL_DYNAMIC_DRAW);
        glBindTexture(GL_TEXTURE_BUFFER, accelShapeIndexTexture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, accelShapeIndexBufferObject);

        glBindTexture(GL_TEXTURE_BUFFER, 0);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
        return;
    }

    std::array<float, 3> boundsMin = {
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    };
    std::array<float, 3> boundsMax = {
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max()
    };

    for (int i = 0; i < shapeCount; ++i) {
        const RuntimeShapeData& shape = runtimeShapes[static_cast<std::size_t>(i)];
        const float r = std::max(shape.influenceRadius, 0.0f);
        for (int axis = 0; axis < 3; ++axis) {
            boundsMin[axis] = std::min(boundsMin[axis], shape.center[axis] - r);
            boundsMax[axis] = std::max(boundsMax[axis], shape.center[axis] + r);
        }
    }

    std::array<float, 3> extent = {
        std::max(boundsMax[0] - boundsMin[0], 1e-3f),
        std::max(boundsMax[1] - boundsMin[1], 1e-3f),
        std::max(boundsMax[2] - boundsMin[2], 1e-3f)
    };

    const float maxExtent = std::max(extent[0], std::max(extent[1], extent[2]));
    const std::array<float, 3> axisRatio = {
        extent[0] / maxExtent,
        extent[1] / maxExtent,
        extent[2] / maxExtent
    };

    const int targetCells = std::max(256, std::min(kAccelMaxCells, shapeCount * 24));
    const float ratioProduct = std::max(axisRatio[0] * axisRatio[1] * axisRatio[2], 1e-3f);
    const float baseDim = std::cbrt(static_cast<float>(targetCells) / ratioProduct);

    for (int axis = 0; axis < 3; ++axis) {
        const int dim = static_cast<int>(std::round(baseDim * axisRatio[axis]));
        accelGridDim[axis] = std::max(kAccelMinDim, std::min(kAccelMaxDim, dim));
    }

    while (accelGridDim[0] * accelGridDim[1] * accelGridDim[2] > kAccelMaxCells) {
        int maxAxis = 0;
        if (accelGridDim[1] > accelGridDim[maxAxis]) {
            maxAxis = 1;
        }
        if (accelGridDim[2] > accelGridDim[maxAxis]) {
            maxAxis = 2;
        }
        if (accelGridDim[maxAxis] <= kAccelMinDim) {
            break;
        }
        accelGridDim[maxAxis] -= 1;
    }

    const int cellCount = accelGridDim[0] * accelGridDim[1] * accelGridDim[2];
    accelCellCount = cellCount;
    if (cellCount <= 1) {
        glBindBuffer(GL_TEXTURE_BUFFER, accelCellRangeBufferObject);
        glBufferData(GL_TEXTURE_BUFFER,
                     static_cast<GLsizeiptr>(accelCellRangeTexels.size() * sizeof(float)),
                     accelCellRangeTexels.data(),
                     GL_DYNAMIC_DRAW);
        glBindTexture(GL_TEXTURE_BUFFER, accelCellRangeTexture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, accelCellRangeBufferObject);

        glBindBuffer(GL_TEXTURE_BUFFER, accelShapeIndexBufferObject);
        glBufferData(GL_TEXTURE_BUFFER,
                     static_cast<GLsizeiptr>(accelShapeIndexTexels.size() * sizeof(float)),
                     accelShapeIndexTexels.data(),
                     GL_DYNAMIC_DRAW);
        glBindTexture(GL_TEXTURE_BUFFER, accelShapeIndexTexture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, accelShapeIndexBufferObject);

        glBindTexture(GL_TEXTURE_BUFFER, 0);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
        return;
    }

    accelGridMin = boundsMin;
    const std::array<float, 3> cellSize = {
        extent[0] / static_cast<float>(accelGridDim[0]),
        extent[1] / static_cast<float>(accelGridDim[1]),
        extent[2] / static_cast<float>(accelGridDim[2])
    };
    accelGridInvCellSize = {
        1.0f / std::max(cellSize[0], 1e-6f),
        1.0f / std::max(cellSize[1], 1e-6f),
        1.0f / std::max(cellSize[2], 1e-6f)
    };

    const float cellDiag = std::sqrt(cellSize[0] * cellSize[0] +
                                     cellSize[1] * cellSize[1] +
                                     cellSize[2] * cellSize[2]);

    std::vector<std::vector<int>> cells(static_cast<std::size_t>(cellCount));

    for (int shapeIndex = 0; shapeIndex < shapeCount; ++shapeIndex) {
        const RuntimeShapeData& shape = runtimeShapes[static_cast<std::size_t>(shapeIndex)];
        const float r = std::max(shape.influenceRadius, 0.0f) + cellDiag;

        const int minX = std::max(0, std::min(accelGridDim[0] - 1,
            static_cast<int>(std::floor((shape.center[0] - r - accelGridMin[0]) * accelGridInvCellSize[0]))));
        const int minY = std::max(0, std::min(accelGridDim[1] - 1,
            static_cast<int>(std::floor((shape.center[1] - r - accelGridMin[1]) * accelGridInvCellSize[1]))));
        const int minZ = std::max(0, std::min(accelGridDim[2] - 1,
            static_cast<int>(std::floor((shape.center[2] - r - accelGridMin[2]) * accelGridInvCellSize[2]))));

        const int maxX = std::max(0, std::min(accelGridDim[0] - 1,
            static_cast<int>(std::floor((shape.center[0] + r - accelGridMin[0]) * accelGridInvCellSize[0]))));
        const int maxY = std::max(0, std::min(accelGridDim[1] - 1,
            static_cast<int>(std::floor((shape.center[1] + r - accelGridMin[1]) * accelGridInvCellSize[1]))));
        const int maxZ = std::max(0, std::min(accelGridDim[2] - 1,
            static_cast<int>(std::floor((shape.center[2] + r - accelGridMin[2]) * accelGridInvCellSize[2]))));

        for (int z = minZ; z <= maxZ; ++z) {
            for (int y = minY; y <= maxY; ++y) {
                for (int x = minX; x <= maxX; ++x) {
                    const int cellIndex = x + y * accelGridDim[0] + z * accelGridDim[0] * accelGridDim[1];
                    cells[static_cast<std::size_t>(cellIndex)].push_back(shapeIndex);
                }
            }
        }
    }

    accelCellRangeTexels.assign(static_cast<std::size_t>(cellCount) * 4u, 0.0f);
    accelShapeIndexTexels.clear();
    accelShapeIndexTexels.reserve(static_cast<std::size_t>(shapeCount * 16) * 4u);

    int fallbackCells = 0;
    std::uint64_t totalCandidates = 0;
    for (int cellIndex = 0; cellIndex < cellCount; ++cellIndex) {
        const std::vector<int>& cellShapes = cells[static_cast<std::size_t>(cellIndex)];
        const std::size_t base = static_cast<std::size_t>(cellIndex) * 4u;

        if (static_cast<int>(cellShapes.size()) > kAccelFallbackCellCandidateCount) {
            accelCellRangeTexels[base + 0] = 0.0f;
            accelCellRangeTexels[base + 1] = 0.0f;
            accelCellRangeTexels[base + 2] = 1.0f;
            accelCellRangeTexels[base + 3] = 0.0f;
            ++fallbackCells;
            continue;
        }

        const int start = static_cast<int>(accelShapeIndexTexels.size() / 4u);
        const int count = static_cast<int>(cellShapes.size());
        totalCandidates += static_cast<std::uint64_t>(count);
        accelCellRangeTexels[base + 0] = static_cast<float>(start);
        accelCellRangeTexels[base + 1] = static_cast<float>(count);
        accelCellRangeTexels[base + 2] = 0.0f;
        accelCellRangeTexels[base + 3] = 0.0f;

        for (int shapeIndex : cellShapes) {
            accelShapeIndexTexels.push_back(static_cast<float>(shapeIndex));
            accelShapeIndexTexels.push_back(0.0f);
            accelShapeIndexTexels.push_back(0.0f);
            accelShapeIndexTexels.push_back(0.0f);
        }
    }

    if (accelShapeIndexTexels.empty()) {
        accelShapeIndexTexels.assign(4u, 0.0f);
    }
    if (accelCellRangeTexels.empty()) {
        accelCellRangeTexels.assign(4u, 0.0f);
    }

    accelGridEnabled = (fallbackCells < cellCount);
    accelFallbackCellCount = fallbackCells;
    if (accelGridEnabled) {
        accelShapeIndexCount = static_cast<int>(accelShapeIndexTexels.size() / 4u);
        const int activeCells = std::max(1, cellCount - fallbackCells);
        accelAverageCandidates = static_cast<float>(static_cast<double>(totalCandidates) / static_cast<double>(activeCells));
    } else {
        accelShapeIndexCount = 0;
        accelAverageCandidates = 0.0f;
    }

    glBindBuffer(GL_TEXTURE_BUFFER, accelCellRangeBufferObject);
    glBufferData(GL_TEXTURE_BUFFER,
                 static_cast<GLsizeiptr>(accelCellRangeTexels.size() * sizeof(float)),
                 accelCellRangeTexels.data(),
                 GL_DYNAMIC_DRAW);
    glBindTexture(GL_TEXTURE_BUFFER, accelCellRangeTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, accelCellRangeBufferObject);

    glBindBuffer(GL_TEXTURE_BUFFER, accelShapeIndexBufferObject);
    glBufferData(GL_TEXTURE_BUFFER,
                 static_cast<GLsizeiptr>(accelShapeIndexTexels.size() * sizeof(float)),
                 accelShapeIndexTexels.data(),
                 GL_DYNAMIC_DRAW);
    glBindTexture(GL_TEXTURE_BUFFER, accelShapeIndexTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, accelShapeIndexBufferObject);

    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

void Renderer::ensureSceneBuffer(const std::vector<Shape>& shapes) {
    const std::uint64_t hash = computeShapeBufferHash(shapes);
    if (sceneBufferReady && hash == sceneBufferHash) {
        return;
    }

    const std::vector<RuntimeShapeData> runtimeShapes = buildRuntimeShapeDataList(shapes);

    int shapeCount = static_cast<int>(runtimeShapes.size());
    if (shapeCount > maxSceneShapes) {
        if (lastCapacityWarningShapeCount != shapeCount) {
            std::cerr << "Warning: Scene has " << shapeCount
                      << " shapes but current GPU texture-buffer capacity is " << maxSceneShapes
                      << " shapes (GL_MAX_TEXTURE_BUFFER_SIZE=" << maxTextureBufferTexels
                      << "). Rendering is clamped to capacity." << std::endl;
            lastCapacityWarningShapeCount = shapeCount;
        }
        shapeCount = maxSceneShapes;
    }

    if (shapeCount > 0) {
        std::array<float, 3> boundsMin = {
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()
        };
        std::array<float, 3> boundsMax = {
            -std::numeric_limits<float>::max(),
            -std::numeric_limits<float>::max(),
            -std::numeric_limits<float>::max()
        };

        for (int i = 0; i < shapeCount; ++i) {
            const RuntimeShapeData& runtimeShape = runtimeShapes[static_cast<std::size_t>(i)];
            const float r = std::max(runtimeShape.influenceRadius, 0.0f);
            for (int axis = 0; axis < 3; ++axis) {
                boundsMin[axis] = std::min(boundsMin[axis], runtimeShape.center[axis] - r);
                boundsMax[axis] = std::max(boundsMax[axis], runtimeShape.center[axis] + r);
            }
        }

        sceneBoundsCenter = {
            0.5f * (boundsMin[0] + boundsMax[0]),
            0.5f * (boundsMin[1] + boundsMax[1]),
            0.5f * (boundsMin[2] + boundsMax[2])
        };
        sceneBoundsRadius = 0.0f;
        for (int i = 0; i < shapeCount; ++i) {
            const RuntimeShapeData& runtimeShape = runtimeShapes[static_cast<std::size_t>(i)];
            const float dx = runtimeShape.center[0] - sceneBoundsCenter[0];
            const float dy = runtimeShape.center[1] - sceneBoundsCenter[1];
            const float dz = runtimeShape.center[2] - sceneBoundsCenter[2];
            const float d = std::sqrt(dx * dx + dy * dy + dz * dz) + std::max(runtimeShape.influenceRadius, 0.0f);
            sceneBoundsRadius = std::max(sceneBoundsRadius, d);
        }
    } else {
        sceneBoundsCenter = {0.0f, 0.0f, 0.0f};
        sceneBoundsRadius = 0.0f;
    }

    buildAccelerationGrid(runtimeShapes, shapeCount);

    const int storageShapeCount = std::max(shapeCount, 1);
    packedSceneTexels.assign(static_cast<std::size_t>(storageShapeCount * sceneTexelStride) * 4u, 0.0f);
    for (int i = 0; i < shapeCount; ++i) {
        const RuntimeShapeData& runtimeShape = runtimeShapes[static_cast<std::size_t>(i)];
        const Shape& sourceShape = shapes[static_cast<std::size_t>(i)];

        writePackedTexel(packedSceneTexels, i, 0,
                         static_cast<float>(runtimeShape.type),
                         static_cast<float>(runtimeShape.blendOp),
                         static_cast<float>(runtimeShape.flags),
                         runtimeShape.smoothness);

        writePackedTexel(packedSceneTexels, i, 1,
                         runtimeShape.center[0], runtimeShape.center[1], runtimeShape.center[2],
                         runtimeShape.influenceRadius);

        writePackedTexel(packedSceneTexels, i, 2,
                         runtimeShape.param[0], runtimeShape.param[1], runtimeShape.param[2], runtimeShape.param[3]);

        writePackedTexel(packedSceneTexels, i, 3,
                         runtimeShape.invRotationRows[0], runtimeShape.invRotationRows[1], runtimeShape.invRotationRows[2],
                         runtimeShape.boundRadius);

        writePackedTexel(packedSceneTexels, i, 4,
                         runtimeShape.invRotationRows[3], runtimeShape.invRotationRows[4], runtimeShape.invRotationRows[5],
                         runtimeShape.roundness);

        writePackedTexel(packedSceneTexels, i, 5,
                         runtimeShape.invRotationRows[6], runtimeShape.invRotationRows[7], runtimeShape.invRotationRows[8],
                         0.0f);

        writePackedTexel(packedSceneTexels, i, 6,
                         runtimeShape.scale[0], runtimeShape.scale[1], runtimeShape.scale[2],
                         runtimeShape.mirrorOffset[0]);

        writePackedTexel(packedSceneTexels, i, 7,
                         runtimeShape.elongation[0], runtimeShape.elongation[1], runtimeShape.elongation[2],
                         runtimeShape.mirrorOffset[1]);

        writePackedTexel(packedSceneTexels, i, 8,
                         runtimeShape.twist[0], runtimeShape.twist[1], runtimeShape.twist[2],
                         runtimeShape.mirrorOffset[2]);

        writePackedTexel(packedSceneTexels, i, 9,
                         runtimeShape.bend[0], runtimeShape.bend[1], runtimeShape.bend[2], runtimeShape.mirrorSmoothness);

        writePackedTexel(packedSceneTexels, i, 10,
                         sourceShape.albedo[0], sourceShape.albedo[1], sourceShape.albedo[2], sourceShape.metallic);

        writePackedTexel(packedSceneTexels, i, 11,
                         sourceShape.roughness,
                         runtimeShape.mirror[0],
                         runtimeShape.mirror[1],
                         runtimeShape.mirror[2]);

        writePackedTexel(packedSceneTexels, i, 12,
                         sourceShape.emission[0],
                         sourceShape.emission[1],
                         sourceShape.emission[2],
                         sourceShape.emissionStrength);

        writePackedTexel(packedSceneTexels, i, 13,
                         sourceShape.transmission,
                         sourceShape.ior,
                         0.0f,
                         0.0f);
    }

    glBindBuffer(GL_TEXTURE_BUFFER, sceneBufferObject);
    glBufferData(GL_TEXTURE_BUFFER,
                 static_cast<GLsizeiptr>(packedSceneTexels.size() * sizeof(float)),
                 packedSceneTexels.empty() ? nullptr : packedSceneTexels.data(),
                 GL_DYNAMIC_DRAW);
    glBindTexture(GL_TEXTURE_BUFFER, sceneBufferTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, sceneBufferObject);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    uploadedShapeCount = shapeCount;
    sceneBufferHash = hash;
    sceneBufferReady = true;
}

void Renderer::bindSceneBuffer(const Shader& activeShader, int textureUnit) const {
    const int accelCellRangeUnit = textureUnit + 1;
    const int accelShapeIndexUnit = textureUnit + 2;

    activeShader.setInt("shapeCount", uploadedShapeCount);
    activeShader.setInt("uShapeTexelStride", sceneTexelStride);
    activeShader.setInt("uShapeBuffer", textureUnit);
    activeShader.setInt("uAccelCellRanges", accelCellRangeUnit);
    activeShader.setInt("uAccelShapeIndices", accelShapeIndexUnit);
    activeShader.setInt("uAccelGridEnabled", accelGridEnabled ? 1 : 0);
    activeShader.setIVec3("uAccelGridDim", accelGridDim[0], accelGridDim[1], accelGridDim[2]);
    activeShader.setVec3("uAccelGridMin", accelGridMin[0], accelGridMin[1], accelGridMin[2]);
    activeShader.setVec3("uAccelGridInvCellSize",
                         accelGridInvCellSize[0],
                         accelGridInvCellSize[1],
                         accelGridInvCellSize[2]);
    activeShader.setVec3("uSceneBoundsCenter",
                         sceneBoundsCenter[0],
                         sceneBoundsCenter[1],
                         sceneBoundsCenter[2]);
    activeShader.setFloat("uSceneBoundsRadius", sceneBoundsRadius);

    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_BUFFER, sceneBufferTexture);
    glActiveTexture(GL_TEXTURE0 + accelCellRangeUnit);
    glBindTexture(GL_TEXTURE_BUFFER, accelCellRangeTexture);
    glActiveTexture(GL_TEXTURE0 + accelShapeIndexUnit);
    glBindTexture(GL_TEXTURE_BUFFER, accelShapeIndexTexture);
}

void Renderer::renderSolid(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes,
                           const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                           const float backgroundColor[3],
                           float ambientIntensity, float directLightIntensity,
                           const float cameraPos[3], const float cameraTarget[3],
                           int display_w, int display_h,
                           bool altRenderMode, bool useGradientBackground,
                           const RenderSettings& renderSettings,
                           const TransformationState& transformState) {
    ensureSceneBuffer(shapes);

    Shader& shader = *solidShader;
    shader.use();
    shader.setInt("uRenderMode", altRenderMode ? 1 : 0);

    uploadCommonUniforms(shader,
                         lightDir, lightColor, ambientColor,
                         backgroundColor, ambientIntensity, directLightIntensity,
                         cameraPos, cameraTarget,
                         display_w, display_h,
                         useGradientBackground,
                         renderSettings);

    bindSceneBuffer(shader, 0);

    const int maxSelected = std::min(static_cast<int>(selectedShapes.size()), kMaxSelected);
    shader.setInt("uSelectedCount", maxSelected);

    int selectedIndices[kMaxSelected];
    std::fill(selectedIndices, selectedIndices + kMaxSelected, -1);
    for (int i = 0; i < maxSelected; ++i) {
        selectedIndices[i] = selectedShapes[static_cast<std::size_t>(i)];
    }
    shader.setIntArray("uSelectedIndices", selectedIndices, kMaxSelected);

    shader.setFloat("uOutlineThickness", 0.015f);
    shader.setInt("uShowAxisGuides", transformState.showAxisGuides ? 1 : 0);
    shader.setInt("uActiveAxis", transformState.activeAxis);
    shader.setVec3("uGuideCenter",
                   transformState.guideCenter[0],
                   transformState.guideCenter[1],
                   transformState.guideCenter[2]);
    shader.setVec3("uGuideAxisDirection",
                   transformState.guideAxisDirection[0],
                   transformState.guideAxisDirection[1],
                   transformState.guideAxisDirection[2]);

    int mirrorHelperShow = 0;
    int mirrorHelperSelected = 0;
    int mirrorHelperSelectedAxis = -1;
    float mirrorHelperAxisEnabled[3] = {0.0f, 0.0f, 0.0f};
    float mirrorHelperOffset[3] = {0.0f, 0.0f, 0.0f};
    float mirrorHelperCenter[3] = {0.0f, 0.0f, 0.0f};
    float mirrorHelperExtent = 1.0f;
    auto setupMirrorHelperForShape = [&](int shapeIndex) -> bool {
        if (shapeIndex < 0 || shapeIndex >= static_cast<int>(shapes.size())) {
            return false;
        }

        const Shape& helperSourceShape = shapes[static_cast<std::size_t>(shapeIndex)];
        if (!helperSourceShape.mirrorModifierEnabled ||
            (!helperSourceShape.mirrorAxis[0] && !helperSourceShape.mirrorAxis[1] && !helperSourceShape.mirrorAxis[2])) {
            return false;
        }

        mirrorHelperShow = 1;
        mirrorHelperAxisEnabled[0] = helperSourceShape.mirrorAxis[0] ? 1.0f : 0.0f;
        mirrorHelperAxisEnabled[1] = helperSourceShape.mirrorAxis[1] ? 1.0f : 0.0f;
        mirrorHelperAxisEnabled[2] = helperSourceShape.mirrorAxis[2] ? 1.0f : 0.0f;
        mirrorHelperOffset[0] = helperSourceShape.mirrorOffset[0];
        mirrorHelperOffset[1] = helperSourceShape.mirrorOffset[1];
        mirrorHelperOffset[2] = helperSourceShape.mirrorOffset[2];
        mirrorHelperCenter[0] = helperSourceShape.center[0];
        mirrorHelperCenter[1] = helperSourceShape.center[1];
        mirrorHelperCenter[2] = helperSourceShape.center[2];
        const RuntimeShapeData helperShape = buildRuntimeShapeData(helperSourceShape);
        mirrorHelperExtent = std::max(0.5f, helperShape.boundRadius * 1.5f);

        if (transformState.mirrorHelperSelected &&
            transformState.mirrorHelperShapeIndex == shapeIndex &&
            transformState.mirrorHelperAxis >= 0 &&
            transformState.mirrorHelperAxis < 3 &&
            helperSourceShape.mirrorAxis[transformState.mirrorHelperAxis]) {
            mirrorHelperSelected = 1;
            mirrorHelperSelectedAxis = transformState.mirrorHelperAxis;
        }

        return true;
    };

    bool helperConfigured = false;
    if (transformState.mirrorHelperSelected) {
        helperConfigured = setupMirrorHelperForShape(transformState.mirrorHelperShapeIndex);
    }
    if (!helperConfigured && !selectedShapes.empty()) {
        helperConfigured = setupMirrorHelperForShape(selectedShapes.front());
    }
    shader.setInt("uMirrorHelperShow", mirrorHelperShow);
    shader.setVec3("uMirrorHelperAxisEnabled",
                   mirrorHelperAxisEnabled[0],
                   mirrorHelperAxisEnabled[1],
                   mirrorHelperAxisEnabled[2]);
    shader.setVec3("uMirrorHelperOffset",
                   mirrorHelperOffset[0],
                   mirrorHelperOffset[1],
                   mirrorHelperOffset[2]);
    shader.setVec3("uMirrorHelperCenter",
                   mirrorHelperCenter[0],
                   mirrorHelperCenter[1],
                   mirrorHelperCenter[2]);
    shader.setFloat("uMirrorHelperExtent", mirrorHelperExtent);
    shader.setInt("uMirrorHelperSelected", mirrorHelperSelected);
    shader.setInt("uMirrorHelperSelectedAxis", mirrorHelperSelectedAxis);

    const GLuint timerQuery = solidGpuTimer.queries[solidGpuTimer.writeIndex];
    glBeginQuery(GL_TIME_ELAPSED, timerQuery);
    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glEndQuery(GL_TIME_ELAPSED);

    solidGpuTimer.writeIndex = (solidGpuTimer.writeIndex + 1) % kTimerQueryCount;
    solidGpuTimer.initializedCount = std::min(solidGpuTimer.initializedCount + 1, kTimerQueryCount);
}

void Renderer::ensurePathTracerResources(int display_w, int display_h) {
    display_w = std::max(display_w, 1);
    display_h = std::max(display_h, 1);

    if (display_w == accumulationWidth && display_h == accumulationHeight &&
        accumulationTextures[0] != 0 && accumulationTextures[1] != 0 && denoisedTexture != 0) {
        return;
    }

    accumulationWidth = display_w;
    accumulationHeight = display_h;

    if (accumulationTextures[0] == 0 || accumulationTextures[1] == 0) {
        glGenTextures(2, accumulationTextures);
    }
    if (denoisedTexture == 0) {
        glGenTextures(1, &denoisedTexture);
    }

    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, accumulationTextures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, accumulationWidth, accumulationHeight, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    glBindTexture(GL_TEXTURE_2D, denoisedTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, accumulationWidth, accumulationHeight, 0, GL_RGB, GL_FLOAT, nullptr);

    const std::size_t rgbFloatCount = static_cast<std::size_t>(accumulationWidth) *
                                      static_cast<std::size_t>(accumulationHeight) * 3u;
    denoiseOutput.assign(rgbFloatCount, 0.0f);

    const std::size_t readbackByteSize = rgbFloatCount * sizeof(float);
    if (denoiseReadbackPBOs[0] == 0 || denoiseReadbackPBOs[1] == 0) {
        glGenBuffers(2, denoiseReadbackPBOs);
    }

    for (int i = 0; i < 2; ++i) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, denoiseReadbackPBOs[i]);
        glBufferData(GL_PIXEL_PACK_BUFFER, static_cast<GLsizeiptr>(readbackByteSize), nullptr, GL_STREAM_READ);
        denoiseReadbackReady[i] = false;
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    denoiseReadbackIndex = 0;

#ifdef RMT_HAS_OIDN
    if (denoiserAvailable) {
        const oidn::Storage storageMode = denoiserUsingGPU ? oidn::Storage::Device : oidn::Storage::Host;

        oidnColorBuffer = oidnDevice.newBuffer(readbackByteSize, storageMode);
        oidnOutputBuffer = oidnDevice.newBuffer(readbackByteSize, storageMode);
        oidnBufferByteSize = readbackByteSize;

        const char* deviceError = nullptr;
        if (!oidnColorBuffer || !oidnOutputBuffer ||
            oidnDevice.getError(deviceError) != oidn::Error::None) {
            std::cerr << "OIDN buffer allocation failed: "
                      << (deviceError != nullptr ? deviceError : "unknown error")
                      << std::endl;
            denoiserAvailable = false;
            denoiserUsingGPU = false;
            oidnColorBuffer.release();
            oidnOutputBuffer.release();
            oidnBufferByteSize = 0;
        }
    }
#endif

    resetPathTracerAccumulation();
}

void Renderer::resetPathTracerAccumulation() {
    if (accumulationWidth <= 0 || accumulationHeight <= 0 ||
        accumulationTextures[0] == 0 || accumulationTextures[1] == 0 || denoisedTexture == 0) {
        return;
    }

    GLint previousFBO = 0;
    GLint previousViewport[4] = {0, 0, 1, 1};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    glBindFramebuffer(GL_FRAMEBUFFER, accumulationFBO);
    glViewport(0, 0, accumulationWidth, accumulationHeight);

    for (int i = 0; i < 2; ++i) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumulationTextures[i], 0);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, denoisedTexture, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFBO));
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);

    accumulationWriteIndex = 0;
    pathSampleCount = 0;
    accumulationInitialized = true;
    hasDenoisedFrame = false;
    denoiseReadbackReady[0] = false;
    denoiseReadbackReady[1] = false;
    denoiseReadbackIndex = 0;
}

std::uint64_t Renderer::computeSceneHash(const std::vector<Shape>& shapes,
                                         const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                                         const float backgroundColor[3],
                                         float ambientIntensity, float directLightIntensity,
                                         const float cameraPos[3], const float cameraTarget[3],
                                         int display_w, int display_h,
                                         bool useGradientBackground,
                                         const RenderSettings& renderSettings) const {
    std::uint64_t hash = 1469598103934665603ULL;
    const float cameraFovDegrees = safeCameraFovDegrees(renderSettings);
    const int cameraProjectionMode = safeCameraProjectionMode(renderSettings);
    const int pathTracerMaxBounces = safePathTracerMaxBounces(renderSettings);
    const bool denoiserEnabled = renderSettings.denoiserEnabled;
    const int denoiseStartSample = safeDenoiseStartSample(renderSettings);
    const int denoiseInterval = safeDenoiseInterval(renderSettings);
    const float safeAmbientIntensity = std::max(ambientIntensity, 0.0f);
    const float safeDirectLightIntensity = std::max(directLightIntensity, 0.0f);

    const int shapeCount = static_cast<int>(shapes.size());
    hashValue(hash, shapeCount);
    hashRaw(hash, lightDir, sizeof(float) * 3u);
    hashRaw(hash, lightColor, sizeof(float) * 3u);
    hashRaw(hash, ambientColor, sizeof(float) * 3u);
    hashRaw(hash, backgroundColor, sizeof(float) * 3u);
    hashValue(hash, safeAmbientIntensity);
    hashValue(hash, safeDirectLightIntensity);
    hashRaw(hash, cameraPos, sizeof(float) * 3u);
    hashRaw(hash, cameraTarget, sizeof(float) * 3u);
    hashValue(hash, display_w);
    hashValue(hash, display_h);
    hashValue(hash, useGradientBackground);
    hashValue(hash, cameraFovDegrees);
    hashValue(hash, cameraProjectionMode);
    hashValue(hash, pathTracerMaxBounces);
    hashValue(hash, denoiserEnabled);
    hashValue(hash, denoiseStartSample);
    hashValue(hash, denoiseInterval);

    for (int i = 0; i < shapeCount; ++i) {
        const Shape& shape = shapes[static_cast<std::size_t>(i)];
        hashValue(hash, shape.type);
        hashRaw(hash, shape.center, sizeof(shape.center));
        hashRaw(hash, shape.param, sizeof(shape.param));
        hashRaw(hash, shape.rotation, sizeof(shape.rotation));
        hashValue(hash, shape.extra);
        hashRaw(hash, shape.scale, sizeof(shape.scale));
        hashValue(hash, shape.scaleMode);
        hashRaw(hash, shape.elongation, sizeof(shape.elongation));

        float twist[3] = {
            shape.twistModifierEnabled ? shape.twist[0] : 0.0f,
            shape.twistModifierEnabled ? shape.twist[1] : 0.0f,
            shape.twistModifierEnabled ? shape.twist[2] : 0.0f
        };
        float bend[3] = {
            shape.bendModifierEnabled ? shape.bend[0] : 0.0f,
            shape.bendModifierEnabled ? shape.bend[1] : 0.0f,
            shape.bendModifierEnabled ? shape.bend[2] : 0.0f
        };
        float mirror[3] = {
            (shape.mirrorModifierEnabled && shape.mirrorAxis[0]) ? 1.0f : 0.0f,
            (shape.mirrorModifierEnabled && shape.mirrorAxis[1]) ? 1.0f : 0.0f,
            (shape.mirrorModifierEnabled && shape.mirrorAxis[2]) ? 1.0f : 0.0f
        };
        float mirrorOffset[3] = {
            (shape.mirrorModifierEnabled && shape.mirrorAxis[0]) ? shape.mirrorOffset[0] : 0.0f,
            (shape.mirrorModifierEnabled && shape.mirrorAxis[1]) ? shape.mirrorOffset[1] : 0.0f,
            (shape.mirrorModifierEnabled && shape.mirrorAxis[2]) ? shape.mirrorOffset[2] : 0.0f
        };
        const float mirrorSmoothness = shape.mirrorModifierEnabled ? std::max(shape.mirrorSmoothness, 0.0f) : 0.0f;

        hashRaw(hash, twist, sizeof(twist));
        hashRaw(hash, bend, sizeof(bend));
        hashRaw(hash, mirror, sizeof(mirror));
        hashRaw(hash, mirrorOffset, sizeof(mirrorOffset));
        hashValue(hash, mirrorSmoothness);

        hashValue(hash, shape.blendOp);
        hashValue(hash, shape.smoothness);
        hashRaw(hash, shape.albedo, sizeof(shape.albedo));
        hashValue(hash, shape.metallic);
        hashValue(hash, shape.roughness);
        hashRaw(hash, shape.emission, sizeof(shape.emission));
        hashValue(hash, shape.emissionStrength);
        hashValue(hash, shape.transmission);
        hashValue(hash, shape.ior);
    }

    return hash;
}

void Renderer::renderPathTracer(const std::vector<Shape>& shapes,
                                const std::vector<int>& selectedShapes,
                                const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                                const float backgroundColor[3],
                                float ambientIntensity, float directLightIntensity,
                                const float cameraPos[3], const float cameraTarget[3],
                                int display_w, int display_h,
                                bool useGradientBackground,
                                const RenderSettings& renderSettings,
                                const TransformationState& transformState) {
    ensureSceneBuffer(shapes);
    ensurePathTracerResources(display_w, display_h);

    const int pathTracerMaxBounces = safePathTracerMaxBounces(renderSettings);
    const bool denoiserEnabled = renderSettings.denoiserEnabled;
    const int denoiseStartSample = safeDenoiseStartSample(renderSettings);
    const int denoiseInterval = safeDenoiseInterval(renderSettings);
    const float cameraFovDegrees = safeCameraFovDegrees(renderSettings);
    const int cameraProjectionMode = safeCameraProjectionMode(renderSettings);
    const float cameraOrthoScale = computeCameraOrthoScale(cameraPos, cameraTarget, cameraFovDegrees);

    const std::uint64_t currentSceneHash = computeSceneHash(shapes,
                                                            lightDir, lightColor, ambientColor,
                                                            backgroundColor, ambientIntensity, directLightIntensity,
                                                            cameraPos, cameraTarget,
                                                            display_w, display_h,
                                                            useGradientBackground,
                                                            renderSettings);
    if (!accumulationInitialized || currentSceneHash != lastSceneHash) {
        resetPathTracerAccumulation();
        lastSceneHash = currentSceneHash;
    }

    Shader& shader = *pathTracerShader;
    bool hasActiveMirrorModifier = false;
    for (const Shape& shape : shapes) {
        if (shape.mirrorModifierEnabled &&
            (shape.mirrorAxis[0] || shape.mirrorAxis[1] || shape.mirrorAxis[2])) {
            hasActiveMirrorModifier = true;
            break;
        }
    }

    GLint previousViewport[4] = {0, 0, 1, 1};
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    const int readIndex = accumulationWriteIndex;
    const int writeIndex = 1 - accumulationWriteIndex;

    glBindFramebuffer(GL_FRAMEBUFFER, accumulationFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumulationTextures[writeIndex], 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Path tracer framebuffer is incomplete." << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
        return;
    }
    glViewport(0, 0, display_w, display_h);

    shader.use();
    shader.setInt("uSampleCount", pathSampleCount);
    shader.setInt("uFrameIndex", pathSampleCount + 1);
    shader.setInt("uPrevAccum", 0);
    shader.setInt("uMaxBounces", pathTracerMaxBounces);
    uploadCommonUniforms(shader,
                         lightDir, lightColor, ambientColor,
                         backgroundColor, ambientIntensity, directLightIntensity,
                         cameraPos, cameraTarget,
                         display_w, display_h,
                         useGradientBackground,
                         renderSettings);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, accumulationTextures[readIndex]);
    bindSceneBuffer(shader, 1);
    if (hasActiveMirrorModifier) {
        shader.setInt("uAccelGridEnabled", 0);
    }

    const GLuint pathTimerQuery = pathGpuTimer.queries[pathGpuTimer.writeIndex];
    glBeginQuery(GL_TIME_ELAPSED, pathTimerQuery);
    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glEndQuery(GL_TIME_ELAPSED);

    pathGpuTimer.writeIndex = (pathGpuTimer.writeIndex + 1) % kTimerQueryCount;
    pathGpuTimer.initializedCount = std::min(pathGpuTimer.initializedCount + 1, kTimerQueryCount);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);

    accumulationWriteIndex = writeIndex;
    ++pathSampleCount;

    GLuint displayTexture = accumulationTextures[accumulationWriteIndex];
    if (!denoiserEnabled) {
        hasDenoisedFrame = false;
    }

#ifdef RMT_HAS_OIDN
    if (denoiserEnabled &&
        denoiserAvailable &&
        pathSampleCount >= denoiseStartSample &&
        ((pathSampleCount - denoiseStartSample) % denoiseInterval == 0) &&
        denoiseReadbackPBOs[0] != 0 && denoiseReadbackPBOs[1] != 0) {

        const std::size_t byteSize =
            static_cast<std::size_t>(accumulationWidth) * static_cast<std::size_t>(accumulationHeight) * 3u * sizeof(float);

        const int submitIndex = denoiseReadbackIndex;
        const int processIndex = 1 - denoiseReadbackIndex;

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glBindTexture(GL_TEXTURE_2D, accumulationTextures[accumulationWriteIndex]);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, denoiseReadbackPBOs[submitIndex]);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, nullptr);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        denoiseReadbackReady[submitIndex] = true;

        if (denoiseReadbackReady[processIndex]) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, denoiseReadbackPBOs[processIndex]);
            void* mappedData = glMapBufferRange(GL_PIXEL_PACK_BUFFER,
                                                0,
                                                static_cast<GLsizeiptr>(byteSize),
                                                GL_MAP_READ_BIT);
            if (mappedData != nullptr) {
                try {
                    if (!oidnColorBuffer || !oidnOutputBuffer || oidnBufferByteSize != byteSize) {
                        const oidn::Storage storageMode = denoiserUsingGPU ? oidn::Storage::Device : oidn::Storage::Host;
                        oidnColorBuffer = oidnDevice.newBuffer(byteSize, storageMode);
                        oidnOutputBuffer = oidnDevice.newBuffer(byteSize, storageMode);
                        oidnBufferByteSize = byteSize;
                    }

                    oidnColorBuffer.write(0, byteSize, mappedData);

                    oidnFilter.setImage("color", oidnColorBuffer, oidn::Format::Float3,
                                        static_cast<std::size_t>(accumulationWidth),
                                        static_cast<std::size_t>(accumulationHeight));
                    oidnFilter.setImage("output", oidnOutputBuffer, oidn::Format::Float3,
                                        static_cast<std::size_t>(accumulationWidth),
                                        static_cast<std::size_t>(accumulationHeight));
                    oidnFilter.set("hdr", true);
                    oidnFilter.commit();
                    oidnFilter.execute();
                    oidnOutputBuffer.read(0, byteSize, denoiseOutput.data());

                    const char* errorMessage = nullptr;
                    if (oidnDevice.getError(errorMessage) == oidn::Error::None) {
                        const GLuint denoiseTimerQuery = denoiseGpuTimer.queries[denoiseGpuTimer.writeIndex];
                        glBeginQuery(GL_TIME_ELAPSED, denoiseTimerQuery);
                        glBindTexture(GL_TEXTURE_2D, denoisedTexture);
                        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                                        accumulationWidth, accumulationHeight,
                                        GL_RGB, GL_FLOAT,
                                        denoiseOutput.data());
                        glEndQuery(GL_TIME_ELAPSED);

                        denoiseGpuTimer.writeIndex = (denoiseGpuTimer.writeIndex + 1) % kTimerQueryCount;
                        denoiseGpuTimer.initializedCount = std::min(denoiseGpuTimer.initializedCount + 1, kTimerQueryCount);

                        hasDenoisedFrame = true;
                    } else {
                        std::cerr << "OIDN execution error: "
                                  << (errorMessage != nullptr ? errorMessage : "unknown error")
                                  << std::endl;
                        denoiserAvailable = false;
                        hasDenoisedFrame = false;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "OIDN execution exception: " << e.what() << std::endl;
                    denoiserAvailable = false;
                    hasDenoisedFrame = false;
                }

                glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            }

            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            denoiseReadbackReady[processIndex] = false;
        }

        denoiseReadbackIndex = processIndex;
    }
#endif

    if (hasDenoisedFrame) {
        displayTexture = denoisedTexture;
    }

    int mirrorHelperShow = 0;
    int mirrorHelperSelected = 0;
    int mirrorHelperSelectedAxis = -1;
    float mirrorHelperAxisEnabled[3] = {0.0f, 0.0f, 0.0f};
    float mirrorHelperOffset[3] = {0.0f, 0.0f, 0.0f};
    float mirrorHelperCenter[3] = {0.0f, 0.0f, 0.0f};
    float mirrorHelperExtent = 1.0f;

    auto setupMirrorHelperForShape = [&](int shapeIndex) -> bool {
        if (shapeIndex < 0 || shapeIndex >= static_cast<int>(shapes.size())) {
            return false;
        }

        const Shape& helperSourceShape = shapes[static_cast<std::size_t>(shapeIndex)];
        if (!helperSourceShape.mirrorModifierEnabled ||
            (!helperSourceShape.mirrorAxis[0] && !helperSourceShape.mirrorAxis[1] && !helperSourceShape.mirrorAxis[2])) {
            return false;
        }

        mirrorHelperShow = 1;
        mirrorHelperAxisEnabled[0] = helperSourceShape.mirrorAxis[0] ? 1.0f : 0.0f;
        mirrorHelperAxisEnabled[1] = helperSourceShape.mirrorAxis[1] ? 1.0f : 0.0f;
        mirrorHelperAxisEnabled[2] = helperSourceShape.mirrorAxis[2] ? 1.0f : 0.0f;
        mirrorHelperOffset[0] = helperSourceShape.mirrorOffset[0];
        mirrorHelperOffset[1] = helperSourceShape.mirrorOffset[1];
        mirrorHelperOffset[2] = helperSourceShape.mirrorOffset[2];
        mirrorHelperCenter[0] = helperSourceShape.center[0];
        mirrorHelperCenter[1] = helperSourceShape.center[1];
        mirrorHelperCenter[2] = helperSourceShape.center[2];
        const RuntimeShapeData helperShape = buildRuntimeShapeData(helperSourceShape);
        mirrorHelperExtent = std::max(0.5f, helperShape.boundRadius * 1.5f);

        if (transformState.mirrorHelperSelected &&
            transformState.mirrorHelperShapeIndex == shapeIndex &&
            transformState.mirrorHelperAxis >= 0 &&
            transformState.mirrorHelperAxis < 3 &&
            helperSourceShape.mirrorAxis[transformState.mirrorHelperAxis]) {
            mirrorHelperSelected = 1;
            mirrorHelperSelectedAxis = transformState.mirrorHelperAxis;
        }

        return true;
    };

    bool helperConfigured = false;
    if (transformState.mirrorHelperSelected) {
        helperConfigured = setupMirrorHelperForShape(transformState.mirrorHelperShapeIndex);
    }
    if (!helperConfigured && !selectedShapes.empty()) {
        helperConfigured = setupMirrorHelperForShape(selectedShapes.front());
    }

    Shader& display = *pathTracerDisplayShader;
    display.use();
    display.setInt("uTexture", 0);
    display.setVec2("uResolution", static_cast<float>(display_w), static_cast<float>(display_h));
    display.setVec3("uCameraPos", cameraPos[0], cameraPos[1], cameraPos[2]);
    display.setVec3("uCameraTarget", cameraTarget[0], cameraTarget[1], cameraTarget[2]);
    display.setFloat("uCameraFovDegrees", cameraFovDegrees);
    display.setInt("uCameraProjectionMode", cameraProjectionMode);
    display.setFloat("uCameraOrthoScale", cameraOrthoScale);
    display.setInt("uMirrorHelperShow", mirrorHelperShow);
    display.setVec3("uMirrorHelperAxisEnabled",
                    mirrorHelperAxisEnabled[0], mirrorHelperAxisEnabled[1], mirrorHelperAxisEnabled[2]);
    display.setVec3("uMirrorHelperOffset",
                    mirrorHelperOffset[0], mirrorHelperOffset[1], mirrorHelperOffset[2]);
    display.setVec3("uMirrorHelperCenter",
                    mirrorHelperCenter[0], mirrorHelperCenter[1], mirrorHelperCenter[2]);
    display.setFloat("uMirrorHelperExtent", mirrorHelperExtent);
    display.setInt("uMirrorHelperSelected", mirrorHelperSelected);
    display.setInt("uMirrorHelperSelectedAxis", mirrorHelperSelectedAxis);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, displayTexture);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

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

void Renderer::logPerfIfNeeded(const std::vector<Shape>& shapes) {
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

void Renderer::renderScene(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes,
                           const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                           const float backgroundColor[3],
                           float ambientIntensity, float directLightIntensity,
                           const float cameraPos[3], const float cameraTarget[3],
                           int display_w, int display_h,
                           bool altRenderMode, bool useGradientBackground,
                           bool usePathTracer,
                           const RenderSettings& renderSettings,
                           const TransformationState& transformState)
{
    const std::chrono::steady_clock::time_point frameStart = std::chrono::steady_clock::now();

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

    logPerfIfNeeded(shapes);
}
