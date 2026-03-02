#include "rmt/scene/modifiers/ShapeModifiers.h"

#include <algorithm>
#include <cmath>

#include "rmt/common/math/MatrixMath.h"
#include "rmt/common/math/Vec3Math.h"

namespace rmt {

namespace {

void applyTwistCPU(const float p[3], float k, int axis, float out[3]) {
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

void applyCheapBendCPU(const float p[3], float k, int axis, float out[3]) {
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

} // namespace

float absSum3(const float v[3]) {
    return vec3AbsSum(v);
}

bool anyAbsGreaterThan(const float v[3], float threshold) {
    return std::fabs(v[0]) > threshold ||
           std::fabs(v[1]) > threshold ||
           std::fabs(v[2]) > threshold;
}

bool nonUnitScale(const float v[3], float threshold) {
    return std::fabs(v[0] - 1.0f) > threshold ||
           std::fabs(v[1] - 1.0f) > threshold ||
           std::fabs(v[2] - 1.0f) > threshold;
}

void setIdentity3(float out[9]) {
    mat3SetIdentity(out);
}

void buildInverseRotationRows(const float angles[3], float out[9]) {
    float rz[9];
    float ry[9];
    float rx[9];
    float tmp[9];

    mat3RotationZ(-angles[2], rz);
    mat3RotationY(-angles[1], ry);
    mat3RotationX(-angles[0], rx);

    mat3Mul(rz, ry, tmp);
    mat3Mul(tmp, rx, out);
}

void applyInverseRotationRows(const float rows[9], const float p[3], float out[3]) {
    out[0] = rows[0] * p[0] + rows[1] * p[1] + rows[2] * p[2];
    out[1] = rows[3] * p[0] + rows[4] * p[1] + rows[5] * p[2];
    out[2] = rows[6] * p[0] + rows[7] * p[1] + rows[8] * p[2];
}

void applyRotationInverse(const float p[3], const float angles[3], float out[3]) {
    float rows[9];
    buildInverseRotationRows(angles, rows);
    applyInverseRotationRows(rows, p, out);
}

void applyTwistCombinedCPU(const float p[3], const float k[3], float out[3]) {
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

void applyCheapBendCombinedCPU(const float p[3], const float k[3], float out[3]) {
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

void applyDomainRepeatCombinedCPU(const float p[3], const float axisEnabled[3], const float spacing[3], float out[3]) {
    out[0] = p[0];
    out[1] = p[1];
    out[2] = p[2];

    for (int axis = 0; axis < 3; ++axis) {
        if (axisEnabled[axis] <= 0.5f) {
            continue;
        }
        const float safeSpacing = std::max(std::fabs(spacing[axis]), 1e-4f);
        const float halfSpacing = 0.5f * safeSpacing;
        out[axis] = std::fmod(out[axis] + halfSpacing, safeSpacing);
        if (out[axis] < 0.0f) {
            out[axis] += safeSpacing;
        }
        out[axis] -= halfSpacing;
    }
}

} // namespace rmt
