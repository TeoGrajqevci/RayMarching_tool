#include "rmt/rendering/AccelerationGridBuilder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

#include "rmt/rendering/Renderer.h"
#include "RendererInternal.h"

namespace rmt {

using namespace renderer_internal;

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

} // namespace rmt
