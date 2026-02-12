#include "Shapes.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>

namespace {

const float kHugeDistance = 1e6f;

#if !defined(NDEBUG)
std::atomic<std::uint64_t> gPrimitiveEvaluations(0);
std::atomic<std::uint64_t> gShapeEvaluations(0);
std::atomic<std::uint64_t> gSceneEvaluations(0);

inline void countPrimitiveEvaluation() {
    gPrimitiveEvaluations.fetch_add(1, std::memory_order_relaxed);
}

inline void countShapeEvaluation() {
    gShapeEvaluations.fetch_add(1, std::memory_order_relaxed);
}

inline void countSceneEvaluation() {
    gSceneEvaluations.fetch_add(1, std::memory_order_relaxed);
}
#else
inline void countPrimitiveEvaluation() {}
inline void countShapeEvaluation() {}
inline void countSceneEvaluation() {}
#endif

inline float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(v, hi));
}

inline float mixf(float a, float b, float t) {
    return a + (b - a) * t;
}

inline float absSum3(const float v[3]) {
    return std::fabs(v[0]) + std::fabs(v[1]) + std::fabs(v[2]);
}

inline bool anyAbsGreaterThan(const float v[3], float threshold) {
    return std::fabs(v[0]) > threshold || std::fabs(v[1]) > threshold || std::fabs(v[2]) > threshold;
}

inline bool nonUnitScale(const float v[3], float threshold) {
    return std::fabs(v[0] - 1.0f) > threshold ||
           std::fabs(v[1] - 1.0f) > threshold ||
           std::fabs(v[2] - 1.0f) > threshold;
}

inline void setIdentity3(float out[9]) {
    out[0] = 1.0f; out[1] = 0.0f; out[2] = 0.0f;
    out[3] = 0.0f; out[4] = 1.0f; out[5] = 0.0f;
    out[6] = 0.0f; out[7] = 0.0f; out[8] = 1.0f;
}

inline void mat3Mul(const float a[9], const float b[9], float out[9]) {
    float result[9];
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            result[r * 3 + c] =
                a[r * 3 + 0] * b[0 * 3 + c] +
                a[r * 3 + 1] * b[1 * 3 + c] +
                a[r * 3 + 2] * b[2 * 3 + c];
        }
    }
    for (int i = 0; i < 9; ++i) {
        out[i] = result[i];
    }
}

inline void makeRotX(float angle, float out[9]) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    out[0] = 1.0f; out[1] = 0.0f; out[2] = 0.0f;
    out[3] = 0.0f; out[4] = c;    out[5] = -s;
    out[6] = 0.0f; out[7] = s;    out[8] = c;
}

inline void makeRotY(float angle, float out[9]) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    out[0] = c;    out[1] = 0.0f; out[2] = s;
    out[3] = 0.0f; out[4] = 1.0f; out[5] = 0.0f;
    out[6] = -s;   out[7] = 0.0f; out[8] = c;
}

inline void makeRotZ(float angle, float out[9]) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    out[0] = c;    out[1] = -s;   out[2] = 0.0f;
    out[3] = s;    out[4] = c;    out[5] = 0.0f;
    out[6] = 0.0f; out[7] = 0.0f; out[8] = 1.0f;
}

inline void buildInverseRotationRows(const float angles[3], float out[9]) {
    float rz[9];
    float ry[9];
    float rx[9];
    float tmp[9];

    makeRotZ(-angles[2], rz);
    makeRotY(-angles[1], ry);
    makeRotX(-angles[0], rx);

    mat3Mul(rz, ry, tmp);
    mat3Mul(tmp, rx, out);
}

inline void applyInverseRotationRows(const float rows[9], const float p[3], float out[3]) {
    out[0] = rows[0] * p[0] + rows[1] * p[1] + rows[2] * p[2];
    out[1] = rows[3] * p[0] + rows[4] * p[1] + rows[5] * p[2];
    out[2] = rows[6] * p[0] + rows[7] * p[1] + rows[8] * p[2];
}

inline void applyTwistCPU(const float p[3], float k, int axis, float out[3]) {
    if (std::abs(k) < 1e-6f) {
        out[0] = p[0];
        out[1] = p[1];
        out[2] = p[2];
        return;
    }

    const int ax = std::max(0, std::min(axis, 2));
    if (ax == 0) {
        const float c = std::cos(k * p[0]);
        const float s = std::sin(k * p[0]);
        out[0] = p[0];
        out[1] = c * p[1] + s * p[2];
        out[2] = -s * p[1] + c * p[2];
    } else if (ax == 1) {
        const float c = std::cos(k * p[1]);
        const float s = std::sin(k * p[1]);
        out[0] = c * p[0] + s * p[2];
        out[1] = p[1];
        out[2] = -s * p[0] + c * p[2];
    } else {
        const float c = std::cos(k * p[2]);
        const float s = std::sin(k * p[2]);
        out[0] = c * p[0] + s * p[1];
        out[1] = -s * p[0] + c * p[1];
        out[2] = p[2];
    }
}

inline void applyCheapBendCPU(const float p[3], float k, int axis, float out[3]) {
    if (std::abs(k) < 1e-6f) {
        out[0] = p[0];
        out[1] = p[1];
        out[2] = p[2];
        return;
    }

    const int ax = std::max(0, std::min(axis, 2));
    if (ax == 0) {
        const float c = std::cos(k * p[1]);
        const float s = std::sin(k * p[1]);
        out[0] = p[0];
        out[1] = c * p[1] + s * p[2];
        out[2] = -s * p[1] + c * p[2];
    } else if (ax == 1) {
        const float c = std::cos(k * p[2]);
        const float s = std::sin(k * p[2]);
        out[0] = c * p[0] + s * p[2];
        out[1] = p[1];
        out[2] = -s * p[0] + c * p[2];
    } else {
        const float c = std::cos(k * p[0]);
        const float s = std::sin(k * p[0]);
        out[0] = c * p[0] + s * p[1];
        out[1] = -s * p[0] + c * p[1];
        out[2] = p[2];
    }
}

inline void applyTwistCombinedCPU(const float p[3], const float k[3], float out[3]) {
    float tmp[3] = { p[0], p[1], p[2] };
    float next[3];
    for (int axis = 0; axis < 3; ++axis) {
        applyTwistCPU(tmp, k[axis], axis, next);
        tmp[0] = next[0];
        tmp[1] = next[1];
        tmp[2] = next[2];
    }
    out[0] = tmp[0];
    out[1] = tmp[1];
    out[2] = tmp[2];
}

inline void applyCheapBendCombinedCPU(const float p[3], const float k[3], float out[3]) {
    float tmp[3] = { p[0], p[1], p[2] };
    float next[3];
    for (int axis = 0; axis < 3; ++axis) {
        applyCheapBendCPU(tmp, k[axis], axis, next);
        tmp[0] = next[0];
        tmp[1] = next[1];
        tmp[2] = next[2];
    }
    out[0] = tmp[0];
    out[1] = tmp[1];
    out[2] = tmp[2];
}

inline float sdSphereLocalCPU(const float p[3], float radius) {
    return std::sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]) - radius;
}

inline float sdBoxLocalCPU(const float p[3], const float b[3]) {
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

inline float sdTorusLocalCPU(const float p[3], float majorRadius, float minorRadius) {
    const float qx = std::sqrt(p[0] * p[0] + p[2] * p[2]) - majorRadius;
    return std::sqrt(qx * qx + p[1] * p[1]) - minorRadius;
}

inline float sdCylinderLocalCPU(const float p[3], float radius, float halfHeight) {
    const float q = std::sqrt(p[0] * p[0] + p[2] * p[2]) - radius;
    const float d = std::fabs(p[1]) - halfHeight;
    const float outside = std::sqrt(std::max(q, 0.0f) * std::max(q, 0.0f) +
                                    std::max(d, 0.0f) * std::max(d, 0.0f));
    const float inside = std::min(std::max(q, d), 0.0f);
    return outside + inside;
}

inline float sdConeLocalCPU(const float p[3], float baseRadius, float halfHeight) {
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

inline float sdMandelbulbLocalCPU(const float p[3], float power, float iterationsF, float bailout) {
    const float safePower = clampf(power, 2.0f, 16.0f);
    const int iterations = std::max(1, std::min(static_cast<int>(std::floor(iterationsF + 0.5f)), 64));
    const float bailoutRadius = std::max(bailout, 2.0f);

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
        dr = std::pow(safeR, safePower - 1.0f) * safePower * dr + 1.0f;

        const float t = theta * safePower;
        const float pAngle = phi * safePower;
        const float sinT = std::sin(t);

        z[0] = zr * sinT * std::cos(pAngle) + p[0];
        z[1] = zr * sinT * std::sin(pAngle) + p[1];
        z[2] = zr * std::cos(t) + p[2];
    }

    r = std::max(r, 1e-6f);
    return 0.5f * std::log(r) * r / std::max(dr, 1e-6f);
}

inline float primitiveDistanceCPU(int primitiveType, const float p[3], const RuntimeShapeData& shapeData) {
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
            return sdMandelbulbLocalCPU(p, shapeData.param[0], shapeData.param[1], shapeData.param[2]);
        default:
            return kHugeDistance;
    }
}

inline float primitiveDistanceScaledCPU(int primitiveType, const float p[3], const RuntimeShapeData& shapeData) {
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

inline float primitiveRadiusForShape(int primitiveType, const float param[4]) {
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
        default:
            return 0.0f;
    }
}

inline float smoothUnionDistance(float candidateDist, float sceneDist, float smoothness) {
    const float k = std::max(smoothness, 1e-4f);
    const float h = clampf(0.5f + 0.5f * (sceneDist - candidateDist) / k, 0.0f, 1.0f);
    return mixf(sceneDist, candidateDist, h) - k * h * (1.0f - h);
}

inline float smoothSubtractionDistance(float candidateDist, float sceneDist, float smoothness) {
    const float k = std::max(smoothness, 1e-4f);
    const float h = clampf(0.5f - 0.5f * (sceneDist + candidateDist) / k, 0.0f, 1.0f);
    return mixf(sceneDist, -candidateDist, h) + k * h * (1.0f - h);
}

inline float smoothIntersectionDistance(float candidateDist, float sceneDist, float smoothness) {
    const float k = std::max(smoothness, 1e-4f);
    const float h = clampf(0.5f - 0.5f * (sceneDist - candidateDist) / k, 0.0f, 1.0f);
    return mixf(sceneDist, candidateDist, h) + k * h * (1.0f - h);
}

inline bool shouldSkipFromBounds(const RuntimeShapeData& shapeData,
                                 const float p[3],
                                 float sceneDist) {
    const float dx = p[0] - shapeData.center[0];
    const float dy = p[1] - shapeData.center[1];
    const float dz = p[2] - shapeData.center[2];
    const float centerDist = std::sqrt(dx * dx + dy * dy + dz * dz);
    const float lb = centerDist - shapeData.influenceRadius;
    const float ub = centerDist + shapeData.influenceRadius;
    const float k = std::max(shapeData.smoothness, 1e-4f);

    switch (shapeData.blendOp) {
        case BLEND_NONE:
            return lb >= sceneDist;
        case BLEND_SMOOTH_UNION:
            return lb >= sceneDist + k;
        case BLEND_SMOOTH_SUBTRACTION:
            return sceneDist + lb >= k;
        case BLEND_SMOOTH_INTERSECTION:
            return ub <= sceneDist - k;
        default:
            return lb >= sceneDist;
    }
}

inline void packPrimitiveParameters(const Shape& shape, int primitiveType, float out[4]) {
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
    out[3] = 0.0f;

    switch (primitiveType) {
        case SHAPE_SPHERE:
            out[3] = shape.param[0];
            break;
        case SHAPE_BOX:
            out[0] = shape.param[0];
            out[1] = shape.param[1];
            out[2] = shape.param[2];
            break;
        case SHAPE_TORUS:
            out[0] = shape.param[0];
            out[1] = shape.param[1];
            break;
        case SHAPE_CYLINDER:
            out[0] = shape.param[0];
            out[1] = shape.param[1];
            break;
        case SHAPE_CONE:
            out[0] = shape.param[0];
            out[1] = shape.param[1];
            break;
        case SHAPE_MANDELBULB:
            out[0] = shape.param[0];
            out[1] = shape.param[1];
            out[2] = shape.param[2];
            break;
        default:
            break;
    }
}

float evaluatePrimitiveDistanceCPU(const float p[3], const Shape& shape, int primitiveType) {
    RuntimeShapeData shapeData = buildRuntimeShapeData(shape);
    shapeData.type = primitiveType;
    packPrimitiveParameters(shape, primitiveType, shapeData.param);
    return evaluateRuntimeShapeDistance(p, shapeData);
}

} // namespace

RuntimeShapeData buildRuntimeShapeData(const Shape& shape) {
    RuntimeShapeData out;
    out.type = shape.type;
    out.blendOp = shape.blendOp;
    out.flags = 0;
    out.scaleMode = shape.scaleMode;

    out.center[0] = shape.center[0];
    out.center[1] = shape.center[1];
    out.center[2] = shape.center[2];

    packPrimitiveParameters(shape, shape.type, out.param);

    const bool hasRotation = anyAbsGreaterThan(shape.rotation, 1e-6f);
    if (hasRotation) {
        buildInverseRotationRows(shape.rotation, out.invRotationRows);
        out.flags |= RUNTIME_SHAPE_HAS_ROTATION;
    } else {
        setIdentity3(out.invRotationRows);
    }

    out.scale[0] = std::max(std::fabs(shape.scale[0]), 0.001f);
    out.scale[1] = std::max(std::fabs(shape.scale[1]), 0.001f);
    out.scale[2] = std::max(std::fabs(shape.scale[2]), 0.001f);
    if (nonUnitScale(out.scale, 1e-5f)) {
        out.flags |= RUNTIME_SHAPE_HAS_SCALE;
    }

    out.elongation[0] = shape.elongation[0];
    out.elongation[1] = shape.elongation[1];
    out.elongation[2] = shape.elongation[2];
    if (anyAbsGreaterThan(out.elongation, 1e-6f)) {
        out.flags |= RUNTIME_SHAPE_HAS_ELONGATION;
    }

    out.roundness = std::max(shape.extra, 0.0f);
    if (out.roundness > 1e-6f) {
        out.flags |= RUNTIME_SHAPE_HAS_ROUNDING;
    }

    out.twist[0] = shape.twistModifierEnabled ? shape.twist[0] : 0.0f;
    out.twist[1] = shape.twistModifierEnabled ? shape.twist[1] : 0.0f;
    out.twist[2] = shape.twistModifierEnabled ? shape.twist[2] : 0.0f;
    if (anyAbsGreaterThan(out.twist, 1e-6f)) {
        out.flags |= RUNTIME_SHAPE_HAS_TWIST;
    }

    out.bend[0] = shape.bendModifierEnabled ? shape.bend[0] : 0.0f;
    out.bend[1] = shape.bendModifierEnabled ? shape.bend[1] : 0.0f;
    out.bend[2] = shape.bendModifierEnabled ? shape.bend[2] : 0.0f;
    if (anyAbsGreaterThan(out.bend, 1e-6f)) {
        out.flags |= RUNTIME_SHAPE_HAS_BEND;
    }

    out.mirror[0] = (shape.mirrorModifierEnabled && shape.mirrorAxis[0]) ? 1.0f : 0.0f;
    out.mirror[1] = (shape.mirrorModifierEnabled && shape.mirrorAxis[1]) ? 1.0f : 0.0f;
    out.mirror[2] = (shape.mirrorModifierEnabled && shape.mirrorAxis[2]) ? 1.0f : 0.0f;
    out.mirrorOffset[0] = shape.mirrorOffset[0];
    out.mirrorOffset[1] = shape.mirrorOffset[1];
    out.mirrorOffset[2] = shape.mirrorOffset[2];
    if (anyAbsGreaterThan(out.mirror, 0.5f)) {
        out.flags |= RUNTIME_SHAPE_HAS_MIRROR;
    } else {
        out.mirrorOffset[0] = 0.0f;
        out.mirrorOffset[1] = 0.0f;
        out.mirrorOffset[2] = 0.0f;
    }
    out.mirrorSmoothness = (out.flags & RUNTIME_SHAPE_HAS_MIRROR) != 0
        ? std::max(shape.mirrorSmoothness, 0.0f)
        : 0.0f;

    out.smoothness = std::max(shape.smoothness, 1e-4f);
    if (shape.scaleMode == SCALE_MODE_ELONGATE) {
        out.flags |= RUNTIME_SHAPE_SCALE_MODE_ELONGATE;
    }

    float baseRadius = primitiveRadiusForShape(out.type, out.param);
    const float maxScale = std::max(out.scale[0], std::max(out.scale[1], out.scale[2]));
    baseRadius *= maxScale;

    const float ex = std::max(out.elongation[0], 0.0f);
    const float ey = std::max(out.elongation[1], 0.0f);
    const float ez = std::max(out.elongation[2], 0.0f);
    baseRadius += std::sqrt(ex * ex + ey * ey + ez * ez);

    baseRadius += out.roundness;
    const float warpBoost = 1.0f + 0.5f * (absSum3(out.twist) + absSum3(out.bend));
    out.boundRadius = baseRadius * warpBoost + 0.01f;

    float mirrorCenterOffset = 0.0f;
    if ((out.flags & RUNTIME_SHAPE_HAS_MIRROR) != 0) {
        const float dx = out.mirror[0] > 0.5f ? (2.0f * std::fabs(out.center[0] - out.mirrorOffset[0])) : 0.0f;
        const float dy = out.mirror[1] > 0.5f ? (2.0f * std::fabs(out.center[1] - out.mirrorOffset[1])) : 0.0f;
        const float dz = out.mirror[2] > 0.5f ? (2.0f * std::fabs(out.center[2] - out.mirrorOffset[2])) : 0.0f;
        mirrorCenterOffset = std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    out.influenceRadius = out.boundRadius + mirrorCenterOffset +
                          std::max(out.smoothness, 0.0f) +
                          std::max(out.mirrorSmoothness, 0.0f);

    return out;
}

std::vector<RuntimeShapeData> buildRuntimeShapeDataList(const std::vector<Shape>& shapes) {
    std::vector<RuntimeShapeData> runtimeShapes;
    runtimeShapes.reserve(shapes.size());
    for (std::vector<Shape>::const_iterator it = shapes.begin(); it != shapes.end(); ++it) {
        runtimeShapes.push_back(buildRuntimeShapeData(*it));
    }
    return runtimeShapes;
}

float evaluateRuntimeShapeDistanceSingle(const float p[3], const RuntimeShapeData& shapeData) {
    float local[3] = {
        p[0] - shapeData.center[0],
        p[1] - shapeData.center[1],
        p[2] - shapeData.center[2]
    };

    float rotated[3];
    if ((shapeData.flags & RUNTIME_SHAPE_HAS_ROTATION) != 0) {
        applyInverseRotationRows(shapeData.invRotationRows, local, rotated);
    } else {
        rotated[0] = local[0];
        rotated[1] = local[1];
        rotated[2] = local[2];
    }

    float twisted[3];
    if ((shapeData.flags & RUNTIME_SHAPE_HAS_TWIST) != 0) {
        applyTwistCombinedCPU(rotated, shapeData.twist, twisted);
    } else {
        twisted[0] = rotated[0];
        twisted[1] = rotated[1];
        twisted[2] = rotated[2];
    }

    float bent[3];
    if ((shapeData.flags & RUNTIME_SHAPE_HAS_BEND) != 0) {
        applyCheapBendCombinedCPU(twisted, shapeData.bend, bent);
    } else {
        bent[0] = twisted[0];
        bent[1] = twisted[1];
        bent[2] = twisted[2];
    }

    float q[3] = {
        bent[0],
        bent[1],
        bent[2]
    };
    float correction = 0.0f;

    if ((shapeData.flags & RUNTIME_SHAPE_HAS_ELONGATION) != 0) {
        const float raw[3] = {
            std::fabs(bent[0]) - shapeData.elongation[0],
            std::fabs(bent[1]) - shapeData.elongation[1],
            std::fabs(bent[2]) - shapeData.elongation[2]
        };

        q[0] = std::max(raw[0], 0.0f);
        q[1] = std::max(raw[1], 0.0f);
        q[2] = std::max(raw[2], 0.0f);
        correction = std::min(std::max(raw[0], std::max(raw[1], raw[2])), 0.0f);
    }

    float distance = primitiveDistanceScaledCPU(shapeData.type, q, shapeData) + correction;

    if ((shapeData.flags & RUNTIME_SHAPE_HAS_ROUNDING) != 0) {
        distance -= shapeData.roundness;
    }

    const float warpAmount = absSum3(shapeData.twist) + absSum3(shapeData.bend);
    const float safety = 1.0f + 0.35f * std::min(warpAmount, 8.0f);
    distance /= safety;

    return distance;
}

float evaluateRuntimeShapeDistance(const float p[3], const RuntimeShapeData& shapeData) {
    countShapeEvaluation();

    float bestDistance = evaluateRuntimeShapeDistanceSingle(p, shapeData);
    if ((shapeData.flags & RUNTIME_SHAPE_HAS_MIRROR) == 0) {
        return bestDistance;
    }

    const int maxX = shapeData.mirror[0] > 0.5f ? 1 : 0;
    const int maxY = shapeData.mirror[1] > 0.5f ? 1 : 0;
    const int maxZ = shapeData.mirror[2] > 0.5f ? 1 : 0;
    if (maxX == 0 && maxY == 0 && maxZ == 0) {
        return bestDistance;
    }
    const float mirrorK = std::max(shapeData.mirrorSmoothness, 0.0f);

    for (int ix = 0; ix <= maxX; ++ix) {
        for (int iy = 0; iy <= maxY; ++iy) {
            for (int iz = 0; iz <= maxZ; ++iz) {
                if (ix == 0 && iy == 0 && iz == 0) {
                    continue;
                }

                float mirroredWorld[3] = {
                    ix == 1 ? (2.0f * shapeData.mirrorOffset[0] - p[0]) : p[0],
                    iy == 1 ? (2.0f * shapeData.mirrorOffset[1] - p[1]) : p[1],
                    iz == 1 ? (2.0f * shapeData.mirrorOffset[2] - p[2]) : p[2]
                };
                const float candidateDist = evaluateRuntimeShapeDistanceSingle(mirroredWorld, shapeData);
                if (mirrorK > 1e-6f) {
                    bestDistance = smoothUnionDistance(candidateDist, bestDistance, mirrorK);
                } else {
                    bestDistance = std::min(bestDistance, candidateDist);
                }
            }
        }
    }

    return bestDistance;
}

float evaluateRuntimeSceneDistance(const float p[3], const std::vector<RuntimeShapeData>& shapeDataList) {
    countSceneEvaluation();

    if (shapeDataList.empty()) {
        return kHugeDistance;
    }

    float sceneDist = kHugeDistance;
    for (std::size_t i = 0; i < shapeDataList.size(); ++i) {
        const RuntimeShapeData& shapeData = shapeDataList[i];
        if (shouldSkipFromBounds(shapeData, p, sceneDist)) {
            continue;
        }

        const float candidateDist = evaluateRuntimeShapeDistance(p, shapeData);
        switch (shapeData.blendOp) {
            case BLEND_SMOOTH_UNION:
                sceneDist = smoothUnionDistance(candidateDist, sceneDist, shapeData.smoothness);
                break;
            case BLEND_SMOOTH_SUBTRACTION:
                sceneDist = smoothSubtractionDistance(candidateDist, sceneDist, shapeData.smoothness);
                break;
            case BLEND_SMOOTH_INTERSECTION:
                sceneDist = smoothIntersectionDistance(candidateDist, sceneDist, shapeData.smoothness);
                break;
            case BLEND_NONE:
            default:
                sceneDist = std::min(sceneDist, candidateDist);
                break;
        }
    }

    return sceneDist;
}

ShapeEvalStats getShapeEvalStats() {
    ShapeEvalStats stats;
#if !defined(NDEBUG)
    stats.primitiveEvaluations = gPrimitiveEvaluations.load(std::memory_order_relaxed);
    stats.shapeEvaluations = gShapeEvaluations.load(std::memory_order_relaxed);
    stats.sceneEvaluations = gSceneEvaluations.load(std::memory_order_relaxed);
#else
    stats.primitiveEvaluations = 0;
    stats.shapeEvaluations = 0;
    stats.sceneEvaluations = 0;
#endif
    return stats;
}

void resetShapeEvalStats() {
#if !defined(NDEBUG)
    gPrimitiveEvaluations.store(0, std::memory_order_relaxed);
    gShapeEvaluations.store(0, std::memory_order_relaxed);
    gSceneEvaluations.store(0, std::memory_order_relaxed);
#endif
}

void applyRotationInverse(const float p[3], const float angles[3], float out[3]) {
    float rows[9];
    buildInverseRotationRows(angles, rows);
    applyInverseRotationRows(rows, p, out);
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

int pickRayMarchSDF(const float rayOrigin[3], const float rayDir[3],
                    const std::vector<Shape>& shapes,
                    float& outT) {
    const int maxPickSteps = 128;
    const float maxPickDist = 100.0f;
    const float pickSurfaceDist = 0.001f;

    const std::vector<RuntimeShapeData> runtimeShapes = buildRuntimeShapeDataList(shapes);
    if (runtimeShapes.empty()) {
        outT = 0.0f;
        return -1;
    }

    float t = 0.0f;
    int hitShape = -1;

    for (int step = 0; step < maxPickSteps; ++step) {
        const float p[3] = {
            rayOrigin[0] + rayDir[0] * t,
            rayOrigin[1] + rayDir[1] * t,
            rayOrigin[2] + rayDir[2] * t
        };

        const float sceneDist = evaluateRuntimeSceneDistance(p, runtimeShapes);
        if (sceneDist < pickSurfaceDist) {
            float bestDist = maxPickDist;
            for (std::size_t i = 0; i < runtimeShapes.size(); ++i) {
                const RuntimeShapeData& shapeData = runtimeShapes[i];
                const float dx = p[0] - shapeData.center[0];
                const float dy = p[1] - shapeData.center[1];
                const float dz = p[2] - shapeData.center[2];
                const float lb = std::sqrt(dx * dx + dy * dy + dz * dz) - shapeData.influenceRadius;
                if (lb >= bestDist) {
                    continue;
                }

                const float d = evaluateRuntimeShapeDistance(p, shapeData);
                if (d < bestDist) {
                    bestDist = d;
                    hitShape = static_cast<int>(i);
                }
            }
            break;
        }

        t += std::max(sceneDist, pickSurfaceDist * 0.25f);
        if (t > maxPickDist) {
            break;
        }
    }

    outT = t;
    return hitShape;
}
