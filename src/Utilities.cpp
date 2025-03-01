#include "Utilities.h"
#include <cmath>

void sphericalToCartesian(float distance, float theta, float phi, const float target[3], float outPos[3])
{
    outPos[0] = target[0] + distance * cosf(phi) * sinf(theta);
    outPos[1] = target[1] + distance * sinf(phi);
    outPos[2] = target[2] + distance * cosf(phi) * cosf(theta);
}