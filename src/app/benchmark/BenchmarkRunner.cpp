#include "rmt/app/benchmark/BenchmarkRunner.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "rmt/app/benchmark/BenchmarkCapture.h"
#include "rmt/app/benchmark/BenchmarkReport.h"
#include "rmt/app/benchmark/BenchmarkScenes.h"
#include "rmt/common/fs/DirectoryUtils.h"
#include "rmt/common/io/PpmCodec.h"
#include "rmt/input/Input.h"
#include "rmt/RenderSettings.h"
#include "rmt/rendering/Renderer.h"

namespace rmt {

int runBenchmarkSuite(GLFWwindow* window,
                      Renderer& renderer,
                      const BenchmarkOptions& benchmarkOptions,
                      const float lightDir[3],
                      const float lightColor[3],
                      const float ambientColor[3],
                      const float backgroundColor[3],
                      float ambientIntensity,
                      float directLightIntensity) {
    glfwSwapInterval(0);
    renderer.setPerfLoggingEnabled(!benchmarkOptions.disablePerfLogs);

    RenderSettings benchSettings;
    benchSettings.rayMarchQuality = benchmarkOptions.qualityLevel;
    benchSettings.debugOutputMode = RENDER_DEBUG_OUTPUT_NONE;
    benchSettings.pathTracerMaxBounces = benchmarkOptions.benchmarkPathTracerBounces;
    benchSettings.pathTracerUseFixedSeed = true;
    benchSettings.pathTracerFixedSeed = 1337;
    benchSettings.denoiserEnabled = false;

    const std::vector<int> selectedShapes;
    const bool useGradientBackground = false;
    const bool altRenderMode = true;
    const bool usePathTracer = benchmarkOptions.benchmarkUsePathTracer;
    const std::string rendererLabel = usePathTracer ? "pathtracer" : "solid";
    const TransformationState transformState;

    if (!benchmarkOptions.captureDir.empty()) {
        ensureDirectoryExists(benchmarkOptions.captureDir);
    }
    if (!benchmarkOptions.compareDir.empty()) {
        ensureDirectoryExists(benchmarkOptions.compareDir);
    }

    std::vector<BenchmarkSceneSpec> benchmarkScenes = buildBenchmarkSceneSpecs(benchmarkOptions.benchmarkUsePathTracer);
    if (benchmarkOptions.evenMixShapeCount > 0) {
        BenchmarkSceneSpec evenMix;
        evenMix.id = BENCH_SCENE_REPEATED;
        evenMix.key = "even_mix";
        evenMix.label = "Even Mix";
        evenMix.shapes = buildEvenMixBenchmarkScene(benchmarkOptions.evenMixShapeCount);
        evenMix.cameraTarget[0] = 0.0f;
        evenMix.cameraTarget[1] = 0.0f;
        evenMix.cameraTarget[2] = 0.0f;
        evenMix.cameraDistance = std::max(5.0f, std::cbrt(static_cast<float>(benchmarkOptions.evenMixShapeCount)) * 2.2f);
        evenMix.thetaStart = 0.3f;
        evenMix.thetaSpeed = 0.6f;
        evenMix.phiBase = 0.32f;
        evenMix.phiAmplitude = 0.08f;
        benchmarkScenes.clear();
        benchmarkScenes.push_back(evenMix);
    }

    std::vector<BenchmarkCaseResult> benchmarkResults;
    const int totalFramesForPath = std::max(benchmarkOptions.measuredFrames + benchmarkOptions.warmupFrames, 1);
    bool regressionAllOk = true;

    for (std::size_t sceneIndex = 0; sceneIndex < benchmarkScenes.size(); ++sceneIndex) {
        const BenchmarkSceneSpec& scene = benchmarkScenes[sceneIndex];
        std::cout << "[Benchmark] Scene: " << scene.label
                  << " (shapes=" << scene.shapes.size() << ")" << std::endl;

        for (std::size_t resIndex = 0; resIndex < benchmarkOptions.resolutions.size(); ++resIndex) {
            const int width = benchmarkOptions.resolutions[resIndex][0];
            const int height = benchmarkOptions.resolutions[resIndex][1];
            glfwSetWindowSize(window, width, height);
            glViewport(0, 0, width, height);
            std::vector<unsigned char> baselineReferenceImage;
            bool hasBaselineReferenceImage = false;
            double baselineTemporalFlicker = 0.0;
            bool hasBaselineTemporalFlicker = false;

            for (int modeIndex = 0; modeIndex < 2; ++modeIndex) {
                const bool optimizedMode = (modeIndex == 1);
                const std::string modeLabel = optimizedMode ? "optimized" : "baseline";
                benchSettings.optimizedTracingEnabled = optimizedMode;

                double sumCpuFrameMs = 0.0;
                double sumCpuSceneMs = 0.0;
                double sumCpuMeshMs = 0.0;
                double sumCpuSetupMs = 0.0;
                double sumCpuDrawMs = 0.0;
                double sumGpuSolidMs = 0.0;
                double sumGpuPathMs = 0.0;
                double sumGpuDenoiseMs = 0.0;
                int measuredCount = 0;

                for (int frame = 0; frame < benchmarkOptions.warmupFrames + benchmarkOptions.measuredFrames; ++frame) {
                    glfwPollEvents();
                    float benchCameraPos[3] = {0.0f, 0.0f, 0.0f};
                    float benchCameraTarget[3] = {0.0f, 0.0f, 0.0f};
                    sampleBenchmarkCamera(scene, frame, totalFramesForPath, benchCameraPos, benchCameraTarget);

                    glViewport(0, 0, width, height);
                    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT);

                    renderer.renderScene(scene.shapes, selectedShapes,
                                         lightDir, lightColor, ambientColor,
                                         backgroundColor, ambientIntensity, directLightIntensity,
                                         benchCameraPos, benchCameraTarget,
                                         width, height,
                                         altRenderMode, useGradientBackground, usePathTracer,
                                         benchSettings, transformState);

                    glfwSwapBuffers(window);
                    renderer.resolvePerfTimersBlocking();

                    if (frame >= benchmarkOptions.warmupFrames) {
                        const RendererPerfSnapshot& snapshot = renderer.getLastPerfSnapshot();
                        sumCpuFrameMs += snapshot.cpuFrameMs;
                        sumCpuSceneMs += snapshot.cpuSceneUploadMs;
                        sumCpuMeshMs += snapshot.cpuMeshUploadMs;
                        sumCpuSetupMs += snapshot.cpuSetupMs;
                        sumCpuDrawMs += snapshot.cpuDrawSubmitMs;
                        sumGpuSolidMs += snapshot.gpuSolidMs;
                        sumGpuPathMs += snapshot.gpuPathMs;
                        sumGpuDenoiseMs += snapshot.gpuDenoiseMs;
                        ++measuredCount;
                    }
                }

                BenchmarkCaseResult result;
                result.mode = modeLabel;
                result.renderer = rendererLabel;
                result.sceneKey = scene.key;
                result.sceneLabel = scene.label;
                result.width = width;
                result.height = height;
                result.runStats.frameCount = measuredCount;
                if (measuredCount > 0) {
                    const double invCount = 1.0 / static_cast<double>(measuredCount);
                    result.runStats.avgCpuFrameMs = sumCpuFrameMs * invCount;
                    result.runStats.avgCpuSceneUploadMs = sumCpuSceneMs * invCount;
                    result.runStats.avgCpuMeshUploadMs = sumCpuMeshMs * invCount;
                    result.runStats.avgCpuSetupMs = sumCpuSetupMs * invCount;
                    result.runStats.avgCpuDrawSubmitMs = sumCpuDrawMs * invCount;
                    result.runStats.avgGpuSolidMs = sumGpuSolidMs * invCount;
                    result.runStats.avgGpuPathMs = sumGpuPathMs * invCount;
                    result.runStats.avgGpuDenoiseMs = sumGpuDenoiseMs * invCount;
                    const double gpuPrimaryMs =
                        usePathTracer ? result.runStats.avgGpuPathMs : result.runStats.avgGpuSolidMs;
                    const double effectiveFrameMs = std::max(result.runStats.avgCpuFrameMs, gpuPrimaryMs);
                    result.runStats.fps = effectiveFrameMs > 0.0
                        ? (1000.0 / effectiveFrameMs)
                        : 0.0;
                }

                if (benchmarkOptions.counterFrames > 0 && !usePathTracer) {
                    benchSettings.debugOutputMode = RENDER_DEBUG_OUTPUT_COUNTERS;
                    double sumAvgMarch = 0.0;
                    double sumMaxMarch = 0.0;
                    double sumAvgShadow = 0.0;
                    double sumMaxShadow = 0.0;
                    double sumAvgTransmission = 0.0;
                    double sumMaxTransmission = 0.0;
                    double sumHitRate = 0.0;
                    double sumThresholdHitRate = 0.0;
                    double sumRefinedHitRate = 0.0;
                    double sumMissRate = 0.0;
                    int counterSamples = 0;

                    for (int frame = 0; frame < benchmarkOptions.counterFrames; ++frame) {
                        glfwPollEvents();
                        float benchCameraPos[3] = {0.0f, 0.0f, 0.0f};
                        float benchCameraTarget[3] = {0.0f, 0.0f, 0.0f};
                        sampleBenchmarkCamera(scene,
                                              benchmarkOptions.warmupFrames + benchmarkOptions.measuredFrames + frame,
                                              totalFramesForPath + benchmarkOptions.counterFrames,
                                              benchCameraPos, benchCameraTarget);

                        glViewport(0, 0, width, height);
                        glClear(GL_COLOR_BUFFER_BIT);
                        renderer.renderScene(scene.shapes, selectedShapes,
                                             lightDir, lightColor, ambientColor,
                                             backgroundColor, ambientIntensity, directLightIntensity,
                                             benchCameraPos, benchCameraTarget,
                                             width, height,
                                             altRenderMode, useGradientBackground, usePathTracer,
                                             benchSettings, transformState);

                        renderer.collectDebugCountersFromFramebuffer(width, height);
                        glfwSwapBuffers(window);

                        const ShaderPerfCounters& counters = renderer.getLastPerfSnapshot().shaderCounters;
                        sumAvgMarch += counters.avgMarchSteps;
                        sumMaxMarch += counters.maxMarchSteps;
                        sumAvgShadow += counters.avgShadowSteps;
                        sumMaxShadow += counters.maxShadowSteps;
                        sumAvgTransmission += counters.avgTransmissionSteps;
                        sumMaxTransmission += counters.maxTransmissionSteps;
                        sumHitRate += counters.hitRate;
                        sumThresholdHitRate += counters.thresholdHitRate;
                        sumRefinedHitRate += counters.refinedHitRate;
                        sumMissRate += counters.missRate;
                        ++counterSamples;
                    }

                    if (counterSamples > 0) {
                        const double invCounter = 1.0 / static_cast<double>(counterSamples);
                        result.counterStats.avgMarchSteps = sumAvgMarch * invCounter;
                        result.counterStats.maxMarchSteps = sumMaxMarch * invCounter;
                        result.counterStats.avgShadowSteps = sumAvgShadow * invCounter;
                        result.counterStats.maxShadowSteps = sumMaxShadow * invCounter;
                        result.counterStats.avgTransmissionSteps = sumAvgTransmission * invCounter;
                        result.counterStats.maxTransmissionSteps = sumMaxTransmission * invCounter;
                        result.counterStats.hitRate = sumHitRate * invCounter;
                        result.counterStats.thresholdHitRate = sumThresholdHitRate * invCounter;
                        result.counterStats.refinedHitRate = sumRefinedHitRate * invCounter;
                        result.counterStats.missRate = sumMissRate * invCounter;
                    }
                    benchSettings.debugOutputMode = RENDER_DEBUG_OUTPUT_NONE;
                }

                float captureCameraPos[3] = {0.0f, 0.0f, 0.0f};
                float captureCameraTarget[3] = {0.0f, 0.0f, 0.0f};
                sampleBenchmarkCamera(scene, benchmarkOptions.warmupFrames / 2 + 3, totalFramesForPath, captureCameraPos, captureCameraTarget);
                glViewport(0, 0, width, height);
                glClear(GL_COLOR_BUFFER_BIT);
                renderer.renderScene(scene.shapes, selectedShapes,
                                     lightDir, lightColor, ambientColor,
                                     backgroundColor, ambientIntensity, directLightIntensity,
                                     captureCameraPos, captureCameraTarget,
                                     width, height,
                                     altRenderMode, useGradientBackground, usePathTracer,
                                     benchSettings, transformState);

                std::vector<unsigned char> captured = captureRgbFramebuffer(width, height);
                glfwSwapBuffers(window);

                const std::string fileName = scene.key + "_" + modeLabel + "_" +
                                             std::to_string(width) + "x" + std::to_string(height) + ".ppm";
                if (!benchmarkOptions.captureDir.empty()) {
                    const std::string outputPath = benchmarkOptions.captureDir + "/" + fileName;
                    if (!savePPM(outputPath, width, height, captured)) {
                        std::cerr << "[Benchmark] Failed to save capture: " << outputPath << std::endl;
                    }
                }

                if (!benchmarkOptions.compareDir.empty()) {
                    int refW = 0;
                    int refH = 0;
                    std::vector<unsigned char> reference;
                    const std::string referencePath = benchmarkOptions.compareDir + "/" + fileName;
                    if (loadPPM(referencePath, refW, refH, reference) &&
                        refW == width && refH == height) {
                        result.spatialDiff = computeImageDiff(captured, reference);
                        const double threshold = static_cast<double>(benchmarkOptions.compareThreshold);
                        if (result.spatialDiff.meanAbs > threshold &&
                            result.spatialDiff.maxAbs > threshold * 12.0) {
                            result.regressionOk = false;
                            regressionAllOk = false;
                        }
                    } else {
                        result.spatialDiff.maxAbs = std::numeric_limits<double>::infinity();
                        result.spatialDiff.meanAbs = std::numeric_limits<double>::infinity();
                        result.regressionOk = false;
                        regressionAllOk = false;
                        std::cerr << "[Benchmark] Missing or invalid reference: " << referencePath << std::endl;
                    }
                } else {
                    if (!optimizedMode) {
                        baselineReferenceImage = captured;
                        hasBaselineReferenceImage = true;
                    } else if (hasBaselineReferenceImage) {
                        result.spatialDiff = computeImageDiff(captured, baselineReferenceImage);
                        const double threshold = static_cast<double>(benchmarkOptions.compareThreshold);
                        if (result.spatialDiff.meanAbs > threshold &&
                            result.spatialDiff.maxAbs > threshold * 12.0) {
                            result.regressionOk = false;
                            regressionAllOk = false;
                        }
                    }
                }

                float temporalCamPosA[3] = {0.0f, 0.0f, 0.0f};
                float temporalCamTargetA[3] = {0.0f, 0.0f, 0.0f};
                float temporalCamPosB[3] = {0.0f, 0.0f, 0.0f};
                float temporalCamTargetB[3] = {0.0f, 0.0f, 0.0f};
                sampleBenchmarkCamera(scene, benchmarkOptions.warmupFrames + 7, totalFramesForPath, temporalCamPosA, temporalCamTargetA);
                sampleBenchmarkCamera(scene, benchmarkOptions.warmupFrames + 8, totalFramesForPath, temporalCamPosB, temporalCamTargetB);

                glViewport(0, 0, width, height);
                glClear(GL_COLOR_BUFFER_BIT);
                renderer.renderScene(scene.shapes, selectedShapes,
                                     lightDir, lightColor, ambientColor,
                                     backgroundColor, ambientIntensity, directLightIntensity,
                                     temporalCamPosA, temporalCamTargetA,
                                     width, height,
                                     altRenderMode, useGradientBackground, usePathTracer,
                                     benchSettings, transformState);
                std::vector<unsigned char> temporalA = captureRgbFramebuffer(width, height);
                glfwSwapBuffers(window);

                glViewport(0, 0, width, height);
                glClear(GL_COLOR_BUFFER_BIT);
                renderer.renderScene(scene.shapes, selectedShapes,
                                     lightDir, lightColor, ambientColor,
                                     backgroundColor, ambientIntensity, directLightIntensity,
                                     temporalCamPosB, temporalCamTargetB,
                                     width, height,
                                     altRenderMode, useGradientBackground, usePathTracer,
                                     benchSettings, transformState);
                std::vector<unsigned char> temporalB = captureRgbFramebuffer(width, height);
                glfwSwapBuffers(window);

                result.temporalFlicker = computeImageDiff(temporalA, temporalB).meanAbs;
                if (!optimizedMode) {
                    baselineTemporalFlicker = result.temporalFlicker;
                    hasBaselineTemporalFlicker = true;
                } else if (hasBaselineTemporalFlicker) {
                    const double allowedTemporal =
                        baselineTemporalFlicker * 1.35 + static_cast<double>(benchmarkOptions.compareThreshold) * 0.5;
                    if (result.temporalFlicker > allowedTemporal) {
                        result.regressionOk = false;
                        regressionAllOk = false;
                    }
                }

                benchmarkResults.push_back(result);

                std::cout << "[Benchmark] "
                          << rendererLabel << " "
                          << modeLabel << " "
                          << scene.key << " "
                          << width << "x" << height
                          << " fps=" << std::fixed << std::setprecision(2) << result.runStats.fps
                          << " cpu_ms=" << result.runStats.avgCpuFrameMs
                          << " gpu_ms=" << (usePathTracer ? result.runStats.avgGpuPathMs : result.runStats.avgGpuSolidMs)
                          << " avg_steps=" << result.counterStats.avgMarchSteps
                          << " max_steps=" << result.counterStats.maxMarchSteps
                          << std::endl;
            }
        }
    }

    writeBenchmarkCsvReport(benchmarkOptions.csvOutputPath, benchmarkResults);
    return regressionAllOk ? 0 : 2;
}

} // namespace rmt
