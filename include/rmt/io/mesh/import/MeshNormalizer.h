#pragma once

#include <string>
#include <vector>

#include "vec.h"

namespace rmt {

bool normalizeMeshToUnitCube(std::vector<Vec3f>& vertices, std::string& outError);

} // namespace rmt
