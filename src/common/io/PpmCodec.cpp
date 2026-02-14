#include "rmt/common/io/PpmCodec.h"

#include <fstream>

namespace rmt {

bool savePPM(const std::string& filePath, int width, int height,
             const std::vector<unsigned char>& rgbPixels) {
    if (width <= 0 || height <= 0) {
        return false;
    }
    if (rgbPixels.size() != static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3u) {
        return false;
    }

    std::ofstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file << "P6\n" << width << " " << height << "\n255\n";
    file.write(reinterpret_cast<const char*>(rgbPixels.data()),
               static_cast<std::streamsize>(rgbPixels.size()));
    return file.good();
}

bool loadPPM(const std::string& filePath, int& width, int& height,
             std::vector<unsigned char>& rgbPixels) {
    width = 0;
    height = 0;
    rgbPixels.clear();

    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::string magic;
    file >> magic;
    if (magic != "P6") {
        return false;
    }

    int maxValue = 0;
    file >> width >> height >> maxValue;
    file.get();

    if (width <= 0 || height <= 0 || maxValue != 255) {
        return false;
    }

    rgbPixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3u);
    file.read(reinterpret_cast<char*>(rgbPixels.data()),
              static_cast<std::streamsize>(rgbPixels.size()));
    return file.good();
}

} // namespace rmt
