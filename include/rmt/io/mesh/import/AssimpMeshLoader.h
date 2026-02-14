#pragma once

#include <string>
#include <vector>

#include "vec.h"

namespace rmt {

std::string toLowerCopy(const std::string& value);
std::string getFileExtensionLower(const std::string& path);

bool loadMeshTrianglesWithAssimp(const std::string& path,
                                 std::vector<Vec3f>& vertices,
                                 std::vector<Vec3ui>& faces,
                                 std::string& outError);

} // namespace rmt
