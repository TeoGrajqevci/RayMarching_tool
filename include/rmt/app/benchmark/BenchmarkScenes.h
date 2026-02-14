#pragma once

#include <string>
#include <vector>

#include "rmt/scene/Shape.h"

namespace rmt {

enum BenchmarkSceneId {
    BENCH_SCENE_SIMPLE = 0,
    BENCH_SCENE_REPEATED = 1,
    BENCH_SCENE_WORST = 2
};

struct BenchmarkSceneSpec {
    BenchmarkSceneId id;
    std::string key;
    std::string label;
    std::vector<Shape> shapes;
    float cameraTarget[3];
    float cameraDistance;
    float thetaStart;
    float thetaSpeed;
    float phiBase;
    float phiAmplitude;
};

std::vector<Shape> buildEvenMixBenchmarkScene(int shapeCount);
std::vector<BenchmarkSceneSpec> buildBenchmarkSceneSpecs(bool pathTracerMode);
void sampleBenchmarkCamera(const BenchmarkSceneSpec& scene,
                           int frameIndex,
                           int totalFrames,
                           float outCameraPos[3],
                           float outCameraTarget[3]);

} // namespace rmt
