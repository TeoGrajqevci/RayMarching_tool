#pragma once

#include <string>
#include <vector>

#include "rmt/app/benchmark/BenchmarkCapture.h"
#include "rmt/app/cli/BenchmarkOptions.h"

struct GLFWwindow;

namespace rmt {

class Renderer;

struct BenchmarkRunStats {
    double avgCpuFrameMs;
    double avgCpuSceneUploadMs;
    double avgCpuMeshUploadMs;
    double avgCpuSetupMs;
    double avgCpuDrawSubmitMs;
    double avgGpuSolidMs;
    double avgGpuPathMs;
    double avgGpuDenoiseMs;
    double fps;
    int frameCount;

    BenchmarkRunStats()
        : avgCpuFrameMs(0.0),
          avgCpuSceneUploadMs(0.0),
          avgCpuMeshUploadMs(0.0),
          avgCpuSetupMs(0.0),
          avgCpuDrawSubmitMs(0.0),
          avgGpuSolidMs(0.0),
          avgGpuPathMs(0.0),
          avgGpuDenoiseMs(0.0),
          fps(0.0),
          frameCount(0) {}
};

struct BenchmarkCounterStats {
    double avgMarchSteps;
    double maxMarchSteps;
    double avgShadowSteps;
    double maxShadowSteps;
    double avgTransmissionSteps;
    double maxTransmissionSteps;
    double hitRate;
    double thresholdHitRate;
    double refinedHitRate;
    double missRate;

    BenchmarkCounterStats()
        : avgMarchSteps(0.0),
          maxMarchSteps(0.0),
          avgShadowSteps(0.0),
          maxShadowSteps(0.0),
          avgTransmissionSteps(0.0),
          maxTransmissionSteps(0.0),
          hitRate(0.0),
          thresholdHitRate(0.0),
          refinedHitRate(0.0),
          missRate(0.0) {}
};

struct BenchmarkCaseResult {
    std::string mode;
    std::string renderer;
    std::string sceneKey;
    std::string sceneLabel;
    int width;
    int height;
    BenchmarkRunStats runStats;
    BenchmarkCounterStats counterStats;
    ImageDiffResult spatialDiff;
    double temporalFlicker;
    bool regressionOk;

    BenchmarkCaseResult()
        : mode(),
          renderer(),
          sceneKey(),
          sceneLabel(),
          width(0),
          height(0),
          runStats(),
          counterStats(),
          spatialDiff(),
          temporalFlicker(0.0),
          regressionOk(true) {}
};

int runBenchmarkSuite(GLFWwindow* window,
                      Renderer& renderer,
                      const BenchmarkOptions& benchmarkOptions,
                      const float lightDir[3],
                      const float lightColor[3],
                      const float ambientColor[3],
                      const float backgroundColor[3],
                      float ambientIntensity,
                      float directLightIntensity);

} // namespace rmt
