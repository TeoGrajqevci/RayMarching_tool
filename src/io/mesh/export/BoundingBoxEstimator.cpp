#include "rmt/io/mesh/export/BoundingBoxEstimator.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "rmt/io/mesh/export/MeshExporter.h"

namespace rmt {

void MeshExporter::calculateBoundingBox(const std::vector<Shape>& shapes,
                                        float& minX,
                                        float& minY,
                                        float& minZ,
                                        float& maxX,
                                        float& maxY,
                                        float& maxZ) {
    minX = minY = minZ = std::numeric_limits<float>::max();
    maxX = maxY = maxZ = -std::numeric_limits<float>::max();

    for (const Shape& shape : shapes) {
        float baseRadius = 0.0f;
        switch (shape.type) {
            case SHAPE_SPHERE:
                baseRadius = shape.param[0];
                break;
            case SHAPE_BOX:
                baseRadius = std::sqrt(
                    shape.param[0] * shape.param[0] +
                    shape.param[1] * shape.param[1] +
                    shape.param[2] * shape.param[2]
                );
                break;
            case SHAPE_TORUS:
                baseRadius = shape.param[0] + shape.param[1];
                break;
            case SHAPE_CYLINDER:
                baseRadius = std::sqrt(shape.param[0] * shape.param[0] + shape.param[1] * shape.param[1]);
                break;
            case SHAPE_CONE:
                baseRadius = std::sqrt(shape.param[0] * shape.param[0] + shape.param[1] * shape.param[1]);
                break;
            case SHAPE_MANDELBULB:
                baseRadius = std::max(1.5f, std::max(shape.param[2], 2.0f) * 0.5f);
                break;
            case SHAPE_MENGER_SPONGE:
                baseRadius = std::sqrt(3.0f) * std::max(shape.param[0], 0.01f);
                break;
            case SHAPE_MESH_SDF:
                baseRadius = std::sqrt(3.0f) * std::max(shape.param[0], 0.01f);
                break;
            default:
                continue;
        }

        const float maxScale = std::max(
            std::fabs(shape.scale[0]),
            std::max(std::fabs(shape.scale[1]), std::fabs(shape.scale[2]))
        );
        baseRadius *= std::max(maxScale, 0.001f);

        const float eX = std::max(shape.elongation[0], 0.0f);
        const float eY = std::max(shape.elongation[1], 0.0f);
        const float eZ = std::max(shape.elongation[2], 0.0f);
        baseRadius += std::sqrt(eX * eX + eY * eY + eZ * eZ);

        baseRadius += std::max(shape.extra, 0.0f);
        const float warpBoost = 1.0f + 0.5f * (
            std::fabs(shape.twist[0]) + std::fabs(shape.twist[1]) + std::fabs(shape.twist[2]) +
            std::fabs(shape.bend[0]) + std::fabs(shape.bend[1]) + std::fabs(shape.bend[2])
        );
        float extent = baseRadius * warpBoost + 0.01f;
        if (shape.mirrorModifierEnabled) {
            extent += std::max(shape.mirrorSmoothness, 0.0f);
        }
        const int maxMirrorX = (shape.mirrorModifierEnabled && shape.mirrorAxis[0]) ? 1 : 0;
        const int maxMirrorY = (shape.mirrorModifierEnabled && shape.mirrorAxis[1]) ? 1 : 0;
        const int maxMirrorZ = (shape.mirrorModifierEnabled && shape.mirrorAxis[2]) ? 1 : 0;

        for (int ix = 0; ix <= maxMirrorX; ++ix) {
            for (int iy = 0; iy <= maxMirrorY; ++iy) {
                for (int iz = 0; iz <= maxMirrorZ; ++iz) {
                    const float cx = (ix == 1) ? (2.0f * shape.mirrorOffset[0] - shape.center[0]) : shape.center[0];
                    const float cy = (iy == 1) ? (2.0f * shape.mirrorOffset[1] - shape.center[1]) : shape.center[1];
                    const float cz = (iz == 1) ? (2.0f * shape.mirrorOffset[2] - shape.center[2]) : shape.center[2];

                    const float shapeMinX = cx - extent;
                    const float shapeMinY = cy - extent;
                    const float shapeMinZ = cz - extent;
                    const float shapeMaxX = cx + extent;
                    const float shapeMaxY = cy + extent;
                    const float shapeMaxZ = cz + extent;

                    minX = std::min(minX, shapeMinX);
                    minY = std::min(minY, shapeMinY);
                    minZ = std::min(minZ, shapeMinZ);
                    maxX = std::max(maxX, shapeMaxX);
                    maxY = std::max(maxY, shapeMaxY);
                    maxZ = std::max(maxZ, shapeMaxZ);
                }
            }
        }
    }

    if (minX >= maxX || minY >= maxY || minZ >= maxZ) {
        minX = minY = minZ = -5.0f;
        maxX = maxY = maxZ = 5.0f;
    }
}

} // namespace rmt
