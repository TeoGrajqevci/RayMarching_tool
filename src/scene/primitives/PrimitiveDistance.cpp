#include "rmt/scene/primitives/PrimitiveDistance.h"

#include <algorithm>
#include <cmath>

#include "rmt/io/mesh/MeshSdfRegistry.h"
#include "rmt/scene/EvalStats.h"
#include "rmt/scene/RuntimeSceneEvaluator.h"
#include "rmt/scene/RuntimeShapeBuilder.h"
#include "rmt/scene/Shape.h"
#include "rmt/scene/modifiers/ShapeModifiers.h"

namespace rmt {

namespace {

const float kHugeDistance = 1e6f;

float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(v, hi));
}

float sdSphereLocalCPU(const float p[3], float radius) {
    return std::sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]) - radius;
}

float sdBoxLocalCPU(const float p[3], const float b[3]) {
    const float dx = std::fabs(p[0]) - b[0];
    const float dy = std::fabs(p[1]) - b[1];
    const float dz = std::fabs(p[2]) - b[2];
    const float ax = std::max(dx, 0.0f);
    const float ay = std::max(dy, 0.0f);
    const float az = std::max(dz, 0.0f);
    const float outside = std::sqrt(ax * ax + ay * ay + az * az);
    const float inside = std::min(std::max(dx, std::max(dy, dz)), 0.0f);
    return outside + inside;
}

float sdTorusLocalCPU(const float p[3], float majorRadius, float minorRadius) {
    const float qx = std::sqrt(p[0] * p[0] + p[2] * p[2]) - majorRadius;
    return std::sqrt(qx * qx + p[1] * p[1]) - minorRadius;
}

float sdCylinderLocalCPU(const float p[3], float radius, float halfHeight) {
    const float q = std::sqrt(p[0] * p[0] + p[2] * p[2]) - radius;
    const float d = std::fabs(p[1]) - halfHeight;
    const float outside = std::sqrt(std::max(q, 0.0f) * std::max(q, 0.0f) +
                                    std::max(d, 0.0f) * std::max(d, 0.0f));
    const float inside = std::min(std::max(q, d), 0.0f);
    return outside + inside;
}

float sdConeLocalCPU(const float p[3], float baseRadius, float halfHeight) {
    const float qx = std::sqrt(p[0] * p[0] + p[2] * p[2]);
    const float qy = p[1];

    const float k1x = 0.0f;
    const float k1y = halfHeight;
    const float k2x = -baseRadius;
    const float k2y = 2.0f * halfHeight;

    const float caX = qx - std::min(qx, (qy < 0.0f) ? baseRadius : 0.0f);
    const float caY = std::fabs(qy) - halfHeight;

    const float k2Dot = k2x * k2x + k2y * k2y;
    const float t = k2Dot > 0.0f
        ? clampf(((k1x - qx) * k2x + (k1y - qy) * k2y) / k2Dot, 0.0f, 1.0f)
        : 0.0f;
    const float cbX = qx - k1x + k2x * t;
    const float cbY = qy - k1y + k2y * t;

    const float caDot = caX * caX + caY * caY;
    const float cbDot = cbX * cbX + cbY * cbY;
    const float distance = std::sqrt(std::max(std::min(caDot, cbDot), 0.0f));
    const float sign = (cbX < 0.0f && caY < 0.0f) ? -1.0f : 1.0f;
    return distance * sign;
}

float sdMandelbulbLocalCPU(const float p[3],
                           float power,
                           float iterationsF,
                           float bailout,
                           float derivativeBias,
                           float distanceScale) {
    const float safePower = clampf(power, 2.0f, 16.0f);
    const int iterations = std::max(1, std::min(static_cast<int>(std::floor(iterationsF + 0.5f)), 64));
    const float bailoutRadius = std::max(bailout, 2.0f);
    const float safeDerivativeBias = clampf(derivativeBias, 0.01f, 4.0f);
    const float safeDistanceScale = std::max(distanceScale, 0.01f);

    float z[3] = { p[0], p[1], p[2] };
    float dr = 1.0f;
    float r = 0.0f;

    for (int i = 0; i < iterations; ++i) {
        r = std::sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
        if (r > bailoutRadius) {
            break;
        }

        const float safeR = std::max(r, 1e-6f);
        const float theta = std::acos(clampf(z[2] / safeR, -1.0f, 1.0f));
        const float phi = std::atan2(z[1], z[0]);
        const float zr = std::pow(safeR, safePower);
        dr = std::pow(safeR, safePower - 1.0f) * safePower * dr + safeDerivativeBias;

        const float t = theta * safePower;
        const float pAngle = phi * safePower;
        const float sinT = std::sin(t);

        z[0] = zr * sinT * std::cos(pAngle) + p[0];
        z[1] = zr * sinT * std::sin(pAngle) + p[1];
        z[2] = zr * std::cos(t) + p[2];
    }

    r = std::max(r, 1e-6f);
    return safeDistanceScale * (0.5f * std::log(r) * r / std::max(dr, 1e-6f));
}

float sdMengerSpongeLocalCPU(const float p[3], float halfExtent, float iterationsF, float holeScale) {
    const float safeHalfExtent = std::max(halfExtent, 0.01f);
    const int iterations = std::max(1, std::min(static_cast<int>(std::floor(iterationsF + 0.5f)), 8));
    const float carveScale = clampf(holeScale, 2.0f, 4.0f);

    const float unitBox[3] = { 1.0f, 1.0f, 1.0f };
    const float q[3] = { p[0] / safeHalfExtent, p[1] / safeHalfExtent, p[2] / safeHalfExtent };
    float d = sdBoxLocalCPU(q, unitBox);
    float scale = 1.0f;

    for (int i = 0; i < iterations; ++i) {
        float a[3];
        for (int axis = 0; axis < 3; ++axis) {
            float wrapped = std::fmod(q[axis] * scale, 2.0f);
            if (wrapped < 0.0f) {
                wrapped += 2.0f;
            }
            a[axis] = wrapped - 1.0f;
        }
        scale *= 3.0f;

        const float r0 = std::fabs(1.0f - carveScale * std::fabs(a[0]));
        const float r1 = std::fabs(1.0f - carveScale * std::fabs(a[1]));
        const float r2 = std::fabs(1.0f - carveScale * std::fabs(a[2]));
        const float carve = (
            std::min(
                std::max(r0, r1),
                std::min(std::max(r1, r2), std::max(r2, r0))
            ) - 1.0f
        ) / scale;
        d = std::max(d, carve);
    }

    return d * safeHalfExtent;
}

} // namespace

float primitiveDistanceCPU(int primitiveType, const float p[3], const RuntimeShapeData& shapeData) {
    countPrimitiveEvaluation();

    switch (primitiveType) {
        case SHAPE_SPHERE:
            return sdSphereLocalCPU(p, shapeData.param[3]);
        case SHAPE_BOX: {
            const float extents[3] = { shapeData.param[0], shapeData.param[1], shapeData.param[2] };
            return sdBoxLocalCPU(p, extents);
        }
        case SHAPE_TORUS:
            return sdTorusLocalCPU(p, shapeData.param[0], shapeData.param[1]);
        case SHAPE_CYLINDER:
            return sdCylinderLocalCPU(p, shapeData.param[0], shapeData.param[1]);
        case SHAPE_CONE:
            return sdConeLocalCPU(p, shapeData.param[0], shapeData.param[1]);
        case SHAPE_MANDELBULB:
            return sdMandelbulbLocalCPU(
                p,
                shapeData.param[0],
                shapeData.param[1],
                shapeData.param[2],
                shapeData.fractalExtra[0],
                shapeData.fractalExtra[1]
            );
        case SHAPE_MENGER_SPONGE:
            return sdMengerSpongeLocalCPU(p, shapeData.param[0], shapeData.param[1], shapeData.param[2]);
        case SHAPE_MESH_SDF: {
            const float baseScale = std::max(shapeData.param[0], 0.001f);
            const int volumeId = static_cast<int>(std::floor(shapeData.param[1] + 0.5f));
            const float normalizedP[3] = {
                p[0] / baseScale,
                p[1] / baseScale,
                p[2] / baseScale
            };
            return sampleMeshSDFNormalized(volumeId, normalizedP) * baseScale;
        }
        default:
            return kHugeDistance;
    }
}

float primitiveDistanceScaledCPU(int primitiveType, const float p[3], const RuntimeShapeData& shapeData) {
    const float safeScale[3] = {
        std::max(std::fabs(shapeData.scale[0]), 0.001f),
        std::max(std::fabs(shapeData.scale[1]), 0.001f),
        std::max(std::fabs(shapeData.scale[2]), 0.001f)
    };

    const float scaled[3] = {
        p[0] / safeScale[0],
        p[1] / safeScale[1],
        p[2] / safeScale[2]
    };

    const float minScale = std::min(safeScale[0], std::min(safeScale[1], safeScale[2]));
    return primitiveDistanceCPU(primitiveType, scaled, shapeData) * minScale;
}

float primitiveRadiusForShape(int primitiveType, const float param[4]) {
    switch (primitiveType) {
        case SHAPE_SPHERE:
            return std::max(param[3], 0.0f);
        case SHAPE_BOX:
            return std::sqrt(param[0] * param[0] + param[1] * param[1] + param[2] * param[2]);
        case SHAPE_TORUS:
            return std::max(param[0] + param[1], 0.0f);
        case SHAPE_CYLINDER:
            return std::sqrt(param[0] * param[0] + param[1] * param[1]);
        case SHAPE_CONE:
            return std::sqrt(param[0] * param[0] + param[1] * param[1]);
        case SHAPE_MANDELBULB:
            return std::max(1.5f, std::max(param[2], 2.0f) * 0.5f);
        case SHAPE_MENGER_SPONGE:
            return std::sqrt(3.0f) * std::max(param[0], 0.01f);
        case SHAPE_MESH_SDF: {
            const float baseScale = std::max(param[0], 0.01f);
            float radius = std::sqrt(3.0f) * baseScale;
            const int sharedResolution = MeshSDFRegistry::getInstance().getSharedResolution();
            if (sharedResolution > 1) {
                const float voxelSize = (2.0f * baseScale) / static_cast<float>(sharedResolution);
                radius += 0.5f * std::sqrt(3.0f) * voxelSize;
            }
            return radius;
        }
        default:
            return 0.0f;
    }
}

float evaluatePrimitiveDistanceCPU(const float p[3], const Shape& shape, int primitiveType) {
    const RuntimeShapeData shapeData = buildRuntimeShapeDataForPrimitive(shape, primitiveType);
    return evaluateRuntimeShapeDistance(p, shapeData);
}

float sdSphereCPU(const float p[3], const Shape& shape) {
    return evaluatePrimitiveDistanceCPU(p, shape, SHAPE_SPHERE);
}

float sdBoxCPU(const float p[3], const Shape& shape) {
    return evaluatePrimitiveDistanceCPU(p, shape, SHAPE_BOX);
}

float sdTorusCPU(const float p[3], const Shape& shape) {
    return evaluatePrimitiveDistanceCPU(p, shape, SHAPE_TORUS);
}

float sdCylinderCPU(const float p[3], const Shape& shape) {
    return evaluatePrimitiveDistanceCPU(p, shape, SHAPE_CYLINDER);
}

float sdConeCPU(const float p[3], const Shape& shape) {
    return evaluatePrimitiveDistanceCPU(p, shape, SHAPE_CONE);
}

float sdMandelbulbCPU(const float p[3], const Shape& shape) {
    return evaluatePrimitiveDistanceCPU(p, shape, SHAPE_MANDELBULB);
}

float sdMengerSpongeCPU(const float p[3], const Shape& shape) {
    return evaluatePrimitiveDistanceCPU(p, shape, SHAPE_MENGER_SPONGE);
}

} // namespace rmt
