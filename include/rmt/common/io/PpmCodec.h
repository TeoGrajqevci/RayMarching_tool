#pragma once

#include <string>
#include <vector>

namespace rmt {

bool savePPM(const std::string& filePath, int width, int height,
             const std::vector<unsigned char>& rgbPixels);

bool loadPPM(const std::string& filePath, int& width, int& height,
             std::vector<unsigned char>& rgbPixels);

} // namespace rmt
