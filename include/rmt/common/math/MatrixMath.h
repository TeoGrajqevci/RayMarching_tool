#pragma once

#include <cmath>

namespace rmt {

inline void mat3SetIdentity(float out[9]) {
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

inline void mat3RotationX(float angle, float out[9]) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    out[0] = 1.0f; out[1] = 0.0f; out[2] = 0.0f;
    out[3] = 0.0f; out[4] = c;    out[5] = -s;
    out[6] = 0.0f; out[7] = s;    out[8] = c;
}

inline void mat3RotationY(float angle, float out[9]) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    out[0] = c;    out[1] = 0.0f; out[2] = s;
    out[3] = 0.0f; out[4] = 1.0f; out[5] = 0.0f;
    out[6] = -s;   out[7] = 0.0f; out[8] = c;
}

inline void mat3RotationZ(float angle, float out[9]) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    out[0] = c;    out[1] = -s;   out[2] = 0.0f;
    out[3] = s;    out[4] = c;    out[5] = 0.0f;
    out[6] = 0.0f; out[7] = 0.0f; out[8] = 1.0f;
}

} // namespace rmt
