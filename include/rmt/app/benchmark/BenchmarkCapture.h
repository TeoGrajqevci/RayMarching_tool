#pragma once

#include <vector>

namespace rmt {

struct ImageDiffResult {
    double meanAbs;
    double maxAbs;

    ImageDiffResult() : meanAbs(0.0), maxAbs(0.0) {}
};

ImageDiffResult computeImageDiff(const std::vector<unsigned char>& a,
                                 const std::vector<unsigned char>& b);

std::vector<unsigned char> captureRgbFramebuffer(int width, int height);

} // namespace rmt
