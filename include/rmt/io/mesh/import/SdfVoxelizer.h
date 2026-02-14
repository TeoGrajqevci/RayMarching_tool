#pragma once

#include <string>
#include <vector>

#include "vec.h"

namespace rmt {

bool voxelizeMeshToSdfSamples(const std::vector<Vec3f>& vertices,
                              const std::vector<Vec3ui>& faces,
                              int sdfResolution,
                              std::vector<float>& outSamples,
                              std::string& outError);

} // namespace rmt
