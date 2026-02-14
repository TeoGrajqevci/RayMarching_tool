#pragma once

#include <string>

#include "rmt/scene/Shape.h"

namespace rmt {

std::string getDisplayNameFromPath(const std::string& path);
Shape buildDefaultMeshShape(const std::string& path, int volumeId);

} // namespace rmt
