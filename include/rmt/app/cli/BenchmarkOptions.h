#pragma once

#include <array>
#include <string>
#include <vector>

namespace rmt {

struct BenchmarkOptions {
    int evenMixShapeCount;
    bool runSuite;
    bool benchmarkUsePathTracer;
    int benchmarkPathTracerBounces;
    int warmupFrames;
    int measuredFrames;
    int counterFrames;
    int qualityLevel;
    bool hiddenWindow;
    bool disablePerfLogs;
    std::vector<std::array<int, 2> > resolutions;
    std::string csvOutputPath;
    std::string captureDir;
    std::string compareDir;
    float compareThreshold;

    BenchmarkOptions();
};

} // namespace rmt
