#include "rmt/app/benchmark/BenchmarkCapture.h"

#include <glad/glad.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace rmt {

ImageDiffResult computeImageDiff(const std::vector<unsigned char>& a,
                                 const std::vector<unsigned char>& b) {
    ImageDiffResult result;
    if (a.size() != b.size() || a.empty()) {
        result.maxAbs = std::numeric_limits<double>::infinity();
        result.meanAbs = std::numeric_limits<double>::infinity();
        return result;
    }

    double accum = 0.0;
    double maxAbs = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const double diff = std::abs(static_cast<int>(a[i]) - static_cast<int>(b[i])) / 255.0;
        accum += diff;
        maxAbs = std::max(maxAbs, diff);
    }

    result.meanAbs = accum / static_cast<double>(a.size());
    result.maxAbs = maxAbs;
    return result;
}

std::vector<unsigned char> captureRgbFramebuffer(int width, int height) {
    std::vector<unsigned char> pixels(
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3u, 0u);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    return pixels;
}

} // namespace rmt
