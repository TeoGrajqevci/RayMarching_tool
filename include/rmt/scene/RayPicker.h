#pragma once

#include <vector>

#include "rmt/scene/Shape.h"

namespace rmt {

int pickRayMarchSDF(const float rayOrigin[3], const float rayDir[3],
                    const std::vector<Shape>& shapes,
                    float& outT);

} // namespace rmt
