#include "rmt/app/benchmark/BenchmarkReport.h"

#include <fstream>
#include <iostream>

namespace rmt {

bool writeBenchmarkCsvReport(const std::string& csvOutputPath,
                             const std::vector<BenchmarkCaseResult>& benchmarkResults) {
    if (csvOutputPath.empty()) {
        return false;
    }

    std::ofstream csv(csvOutputPath.c_str());
    if (!csv.is_open()) {
        std::cerr << "[Benchmark] Failed to open CSV output: " << csvOutputPath << std::endl;
        return false;
    }

    csv << "renderer,mode,scene,width,height,fps,cpu_ms,gpu_solid_ms,gpu_path_ms,gpu_primary_ms,cpu_scene_ms,cpu_mesh_ms,cpu_setup_ms,cpu_draw_ms,"
           "avg_steps,max_steps,avg_shadow_steps,max_shadow_steps,avg_transmission_steps,max_transmission_steps,"
           "hit_rate,threshold_hit_rate,refined_hit_rate,miss_rate,spatial_mean_abs,spatial_max_abs,temporal_flicker,regression_ok\n";

    for (std::size_t i = 0; i < benchmarkResults.size(); ++i) {
        const BenchmarkCaseResult& r = benchmarkResults[i];
        const double gpuPrimaryMs =
            (r.renderer == "pathtracer") ? r.runStats.avgGpuPathMs : r.runStats.avgGpuSolidMs;
        csv << r.renderer << ","
            << r.mode << ","
            << r.sceneKey << ","
            << r.width << ","
            << r.height << ","
            << r.runStats.fps << ","
            << r.runStats.avgCpuFrameMs << ","
            << r.runStats.avgGpuSolidMs << ","
            << r.runStats.avgGpuPathMs << ","
            << gpuPrimaryMs << ","
            << r.runStats.avgCpuSceneUploadMs << ","
            << r.runStats.avgCpuMeshUploadMs << ","
            << r.runStats.avgCpuSetupMs << ","
            << r.runStats.avgCpuDrawSubmitMs << ","
            << r.counterStats.avgMarchSteps << ","
            << r.counterStats.maxMarchSteps << ","
            << r.counterStats.avgShadowSteps << ","
            << r.counterStats.maxShadowSteps << ","
            << r.counterStats.avgTransmissionSteps << ","
            << r.counterStats.maxTransmissionSteps << ","
            << r.counterStats.hitRate << ","
            << r.counterStats.thresholdHitRate << ","
            << r.counterStats.refinedHitRate << ","
            << r.counterStats.missRate << ","
            << r.spatialDiff.meanAbs << ","
            << r.spatialDiff.maxAbs << ","
            << r.temporalFlicker << ","
            << (r.regressionOk ? 1 : 0)
            << "\n";
    }

    std::cout << "[Benchmark] Wrote CSV report: " << csvOutputPath << std::endl;
    return true;
}

} // namespace rmt
