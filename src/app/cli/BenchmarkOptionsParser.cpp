#include "rmt/app/cli/BenchmarkOptionsParser.h"

#include "rmt/RenderSettings.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <string>
#include <vector>

namespace rmt {

namespace {

bool parseIntArgument(const std::string& value, int& outValue) {
    if (value.empty()) {
        return false;
    }
    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != '\0') {
        return false;
    }
    if (parsed <= 0 || parsed > 2000000) {
        return false;
    }
    outValue = static_cast<int>(parsed);
    return true;
}

bool parseNonNegativeIntArgument(const std::string& value, int& outValue, int maxValue) {
    if (value.empty()) {
        return false;
    }
    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != '\0') {
        return false;
    }
    if (parsed < 0 || parsed > static_cast<long>(maxValue)) {
        return false;
    }
    outValue = static_cast<int>(parsed);
    return true;
}

bool parsePositiveFloatArgument(const std::string& value, float& outValue) {
    if (value.empty()) {
        return false;
    }
    char* end = nullptr;
    const double parsed = std::strtod(value.c_str(), &end);
    if (end == value.c_str() || *end != '\0') {
        return false;
    }
    if (!(parsed > 0.0) || parsed > 1000.0) {
        return false;
    }
    outValue = static_cast<float>(parsed);
    return true;
}

bool parseResolutionToken(const std::string& token, std::array<int, 2>& outResolution) {
    const std::size_t xPos = token.find('x');
    if (xPos == std::string::npos) {
        return false;
    }
    int width = 0;
    int height = 0;
    if (!parseIntArgument(token.substr(0, xPos), width)) {
        return false;
    }
    if (!parseIntArgument(token.substr(xPos + 1), height)) {
        return false;
    }
    outResolution[0] = width;
    outResolution[1] = height;
    return true;
}

std::vector<std::array<int, 2> > parseResolutionList(const std::string& value) {
    std::vector<std::array<int, 2> > resolutions;
    std::size_t start = 0;
    while (start < value.size()) {
        const std::size_t comma = value.find(',', start);
        const std::string token =
            value.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
        std::array<int, 2> resolution = {{0, 0}};
        if (parseResolutionToken(token, resolution)) {
            resolutions.push_back(resolution);
        }
        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }
    return resolutions;
}

} // namespace

BenchmarkOptions::BenchmarkOptions()
    : evenMixShapeCount(0),
      runSuite(false),
      benchmarkUsePathTracer(false),
      benchmarkPathTracerBounces(6),
      warmupFrames(45),
      measuredFrames(90),
      counterFrames(18),
      qualityLevel(RAYMARCH_QUALITY_HIGH),
      hiddenWindow(true),
      disablePerfLogs(true),
      resolutions(),
      csvOutputPath("benchmark_results.csv"),
      captureDir(),
      compareDir(),
      compareThreshold(0.025f) {
    resolutions.push_back({{1280, 720}});
    resolutions.push_back({{1920, 1080}});
}

BenchmarkOptions parseBenchmarkOptions(int argc, char** argv) {
    BenchmarkOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i] != nullptr ? argv[i] : "");
        const std::string prefix = "--benchmark-even-mix=";
        if (arg.find(prefix) == 0) {
            int value = 0;
            if (parseIntArgument(arg.substr(prefix.size()), value)) {
                options.evenMixShapeCount = value;
            }
            continue;
        }

        if (arg == "--benchmark-even-mix" && i + 1 < argc) {
            int value = 0;
            if (parseIntArgument(std::string(argv[i + 1]), value)) {
                options.evenMixShapeCount = value;
            }
            ++i;
            continue;
        }

        if (arg == "--benchmark-suite") {
            options.runSuite = true;
            continue;
        }

        if (arg == "--benchmark-pathtracer") {
            options.benchmarkUsePathTracer = true;
            continue;
        }

        const std::string pathBouncesPrefix = "--benchmark-path-bounces=";
        if (arg.find(pathBouncesPrefix) == 0) {
            int value = 0;
            if (parseNonNegativeIntArgument(arg.substr(pathBouncesPrefix.size()), value, 12)) {
                options.benchmarkPathTracerBounces = std::max(1, value);
            }
            continue;
        }

        if (arg == "--benchmark-path-bounces" && i + 1 < argc) {
            int value = 0;
            if (parseNonNegativeIntArgument(std::string(argv[i + 1]), value, 12)) {
                options.benchmarkPathTracerBounces = std::max(1, value);
            }
            ++i;
            continue;
        }

        const std::string warmupPrefix = "--benchmark-warmup=";
        if (arg.find(warmupPrefix) == 0) {
            int value = 0;
            if (parseNonNegativeIntArgument(arg.substr(warmupPrefix.size()), value, 1000000)) {
                options.warmupFrames = value;
            }
            continue;
        }

        if (arg == "--benchmark-warmup" && i + 1 < argc) {
            int value = 0;
            if (parseNonNegativeIntArgument(std::string(argv[i + 1]), value, 1000000)) {
                options.warmupFrames = value;
            }
            ++i;
            continue;
        }

        const std::string framesPrefix = "--benchmark-frames=";
        if (arg.find(framesPrefix) == 0) {
            int value = 0;
            if (parseIntArgument(arg.substr(framesPrefix.size()), value)) {
                options.measuredFrames = value;
            }
            continue;
        }

        if (arg == "--benchmark-frames" && i + 1 < argc) {
            int value = 0;
            if (parseIntArgument(std::string(argv[i + 1]), value)) {
                options.measuredFrames = value;
            }
            ++i;
            continue;
        }

        const std::string counterPrefix = "--benchmark-counters=";
        if (arg.find(counterPrefix) == 0) {
            int value = 0;
            if (parseNonNegativeIntArgument(arg.substr(counterPrefix.size()), value, 1000000)) {
                options.counterFrames = value;
            }
            continue;
        }

        if (arg == "--benchmark-counters" && i + 1 < argc) {
            int value = 0;
            if (parseNonNegativeIntArgument(std::string(argv[i + 1]), value, 1000000)) {
                options.counterFrames = value;
            }
            ++i;
            continue;
        }

        const std::string qualityPrefix = "--benchmark-quality=";
        if (arg.find(qualityPrefix) == 0) {
            int value = 0;
            if (parseNonNegativeIntArgument(arg.substr(qualityPrefix.size()), value,
                                            RAYMARCH_QUALITY_LOW)) {
                options.qualityLevel = std::max(static_cast<int>(RAYMARCH_QUALITY_ULTRA),
                                                std::min(static_cast<int>(RAYMARCH_QUALITY_LOW),
                                                         value));
            }
            continue;
        }

        if (arg == "--benchmark-quality" && i + 1 < argc) {
            int value = 0;
            if (parseNonNegativeIntArgument(std::string(argv[i + 1]), value,
                                            RAYMARCH_QUALITY_LOW)) {
                options.qualityLevel = std::max(static_cast<int>(RAYMARCH_QUALITY_ULTRA),
                                                std::min(static_cast<int>(RAYMARCH_QUALITY_LOW),
                                                         value));
            }
            ++i;
            continue;
        }

        const std::string resPrefix = "--benchmark-res=";
        if (arg.find(resPrefix) == 0) {
            const std::vector<std::array<int, 2> > parsed =
                parseResolutionList(arg.substr(resPrefix.size()));
            if (!parsed.empty()) {
                options.resolutions = parsed;
            }
            continue;
        }

        if (arg == "--benchmark-res" && i + 1 < argc) {
            const std::vector<std::array<int, 2> > parsed =
                parseResolutionList(std::string(argv[i + 1]));
            if (!parsed.empty()) {
                options.resolutions = parsed;
            }
            ++i;
            continue;
        }

        const std::string outputPrefix = "--benchmark-output=";
        if (arg.find(outputPrefix) == 0) {
            options.csvOutputPath = arg.substr(outputPrefix.size());
            continue;
        }

        if (arg == "--benchmark-output" && i + 1 < argc) {
            options.csvOutputPath = std::string(argv[i + 1]);
            ++i;
            continue;
        }

        const std::string capturePrefix = "--benchmark-capture-dir=";
        if (arg.find(capturePrefix) == 0) {
            options.captureDir = arg.substr(capturePrefix.size());
            continue;
        }

        if (arg == "--benchmark-capture-dir" && i + 1 < argc) {
            options.captureDir = std::string(argv[i + 1]);
            ++i;
            continue;
        }

        const std::string comparePrefix = "--benchmark-compare-dir=";
        if (arg.find(comparePrefix) == 0) {
            options.compareDir = arg.substr(comparePrefix.size());
            continue;
        }

        if (arg == "--benchmark-compare-dir" && i + 1 < argc) {
            options.compareDir = std::string(argv[i + 1]);
            ++i;
            continue;
        }

        const std::string thresholdPrefix = "--benchmark-diff-threshold=";
        if (arg.find(thresholdPrefix) == 0) {
            float value = 0.0f;
            if (parsePositiveFloatArgument(arg.substr(thresholdPrefix.size()), value)) {
                options.compareThreshold = value;
            }
            continue;
        }

        if (arg == "--benchmark-diff-threshold" && i + 1 < argc) {
            float value = 0.0f;
            if (parsePositiveFloatArgument(std::string(argv[i + 1]), value)) {
                options.compareThreshold = value;
            }
            ++i;
            continue;
        }

        if (arg == "--benchmark-visible") {
            options.hiddenWindow = false;
            continue;
        }

        if (arg == "--benchmark-keep-logs") {
            options.disablePerfLogs = false;
            continue;
        }
    }

    return options;
}

} // namespace rmt
