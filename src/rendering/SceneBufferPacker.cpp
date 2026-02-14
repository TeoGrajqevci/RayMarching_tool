#include "rmt/rendering/SceneBufferPacker.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>

#include "rmt/io/mesh/MeshSdfRegistry.h"
#include "rmt/rendering/Renderer.h"
#include "RendererInternal.h"

namespace rmt {

using namespace renderer_internal;

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
                         runtimeShape.fractalExtra[0],
                         runtimeShape.fractalExtra[1]);
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

void Renderer::ensureMeshSdfBuffer() {
    MeshSDFRegistry& registry = MeshSDFRegistry::getInstance();
    const std::uint64_t currentVersion = registry.getVersion();
    if (currentVersion == meshSdfBufferVersion) {
        return;
    }

    meshSdfCount = static_cast<int>(registry.getVolumeCount());
    meshSdfResolution = registry.getSharedResolution();

    if (meshSdfCount <= 0 || meshSdfResolution <= 1) {
        packedMeshSdfTexels.assign(1u, 1e6f);
    } else {
        const std::size_t voxelsPerVolume = static_cast<std::size_t>(meshSdfResolution) *
                                            static_cast<std::size_t>(meshSdfResolution) *
                                            static_cast<std::size_t>(meshSdfResolution);
        packedMeshSdfTexels.assign(static_cast<std::size_t>(meshSdfCount) * voxelsPerVolume, 1e6f);

        for (int meshId = 0; meshId < meshSdfCount; ++meshId) {
            const MeshSDFVolume* volume = registry.getVolume(meshId);
            if (volume == nullptr ||
                volume->resolution != meshSdfResolution ||
                volume->samples.size() != voxelsPerVolume) {
                continue;
            }

            const std::size_t dstOffset = static_cast<std::size_t>(meshId) * voxelsPerVolume;
            std::copy(volume->samples.begin(),
                      volume->samples.end(),
                      packedMeshSdfTexels.begin() + dstOffset);
        }
    }

    glBindBuffer(GL_TEXTURE_BUFFER, meshSdfBufferObject);
    glBufferData(GL_TEXTURE_BUFFER,
                 static_cast<GLsizeiptr>(packedMeshSdfTexels.size() * sizeof(float)),
                 packedMeshSdfTexels.data(),
                 GL_DYNAMIC_DRAW);
    glBindTexture(GL_TEXTURE_BUFFER, meshSdfBufferTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, meshSdfBufferObject);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    meshSdfBufferVersion = currentVersion;
}

void Renderer::bindSceneBuffer(const Shader& activeShader, int textureUnit) const {
    const int accelCellRangeUnit = textureUnit + 1;
    const int accelShapeIndexUnit = textureUnit + 2;
    const int meshSdfUnit = textureUnit + 3;

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
    activeShader.setInt("uMeshSdfBuffer", meshSdfUnit);
    activeShader.setInt("uMeshSdfResolution", meshSdfResolution);
    activeShader.setInt("uMeshSdfCount", meshSdfCount);
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
    glActiveTexture(GL_TEXTURE0 + meshSdfUnit);
    glBindTexture(GL_TEXTURE_BUFFER, meshSdfBufferTexture);
}

} // namespace rmt
