#pragma once

#include <cmath>

namespace rmt {

inline float vec3Dot(const float a[3], const float b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline float vec3Length(const float v[3]) {
    return std::sqrt(vec3Dot(v, v));
}

inline float vec3AbsSum(const float v[3]) {
    return std::fabs(v[0]) + std::fabs(v[1]) + std::fabs(v[2]);
}

inline void vec3Normalize(float v[3]) {
    const float len = vec3Length(v);
    if (len > 1e-6f) {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
}

inline void vec3Cross(const float a[3], const float b[3], float out[3]) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

} // namespace rmt
