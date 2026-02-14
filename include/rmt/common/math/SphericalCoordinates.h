#ifndef RMT_COMMON_MATH_SPHERICAL_COORDINATES_H
#define RMT_COMMON_MATH_SPHERICAL_COORDINATES_H

namespace rmt {

void sphericalToCartesian(float distance, float theta, float phi, const float target[3], float outPos[3]);

} // namespace rmt

#endif // RMT_COMMON_MATH_SPHERICAL_COORDINATES_H
