#include "rmt/io/mesh/MeshSdfRegistry.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace rmt {

namespace {

const float kHugeDistance = 1e6f;

inline float clampf(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

inline float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

} // namespace

MeshSDFRegistry& MeshSDFRegistry::getInstance() {
    static MeshSDFRegistry instance;
    return instance;
}

MeshSDFRegistry::MeshSDFRegistry()
    : sharedResolution(0),
      version(1) {}

int MeshSDFRegistry::addVolume(const std::string& sourcePath, int resolution, std::vector<float>&& samples) {
    if (resolution <= 1) {
        return -1;
    }

    const std::size_t voxelCount = static_cast<std::size_t>(resolution) *
                                   static_cast<std::size_t>(resolution) *
                                   static_cast<std::size_t>(resolution);
    if (samples.size() != voxelCount) {
        return -1;
    }

    if (sharedResolution != 0 && sharedResolution != resolution) {
        return -1;
    }

    if (sharedResolution == 0) {
        sharedResolution = resolution;
    }

    MeshSDFVolume volume;
    volume.id = static_cast<int>(volumes.size());
    volume.resolution = resolution;
    volume.sourcePath = sourcePath;
    volume.samples = std::move(samples);
    volumes.push_back(std::move(volume));
    ++version;

    return static_cast<int>(volumes.size()) - 1;
}

const MeshSDFVolume* MeshSDFRegistry::getVolume(int id) const {
    if (id < 0 || id >= static_cast<int>(volumes.size())) {
        return nullptr;
    }
    return &volumes[static_cast<std::size_t>(id)];
}

std::size_t MeshSDFRegistry::getVolumeCount() const {
    return volumes.size();
}

int MeshSDFRegistry::getSharedResolution() const {
    return sharedResolution;
}

std::uint64_t MeshSDFRegistry::getVersion() const {
    return version;
}

float sampleMeshSDFNormalized(int volumeId, const float pNormalized[3]) {
    const MeshSDFVolume* volume = MeshSDFRegistry::getInstance().getVolume(volumeId);
    if (volume == nullptr || volume->resolution <= 1 || volume->samples.empty()) {
        return kHugeDistance;
    }

    const int res = volume->resolution;
    const float clamped[3] = {
        clampf(pNormalized[0], -1.0f, 1.0f),
        clampf(pNormalized[1], -1.0f, 1.0f),
        clampf(pNormalized[2], -1.0f, 1.0f)
    };
    const float outside[3] = {
        std::max(std::fabs(pNormalized[0]) - 1.0f, 0.0f),
        std::max(std::fabs(pNormalized[1]) - 1.0f, 0.0f),
        std::max(std::fabs(pNormalized[2]) - 1.0f, 0.0f)
    };
    const float outsideLength = std::sqrt(outside[0] * outside[0] +
                                          outside[1] * outside[1] +
                                          outside[2] * outside[2]);

    const float gx = clampf((clamped[0] * 0.5f + 0.5f) * static_cast<float>(res), 0.0f, static_cast<float>(res - 1));
    const float gy = clampf((clamped[1] * 0.5f + 0.5f) * static_cast<float>(res), 0.0f, static_cast<float>(res - 1));
    const float gz = clampf((clamped[2] * 0.5f + 0.5f) * static_cast<float>(res), 0.0f, static_cast<float>(res - 1));

    const int x0 = std::max(0, std::min(res - 1, static_cast<int>(std::floor(gx))));
    const int y0 = std::max(0, std::min(res - 1, static_cast<int>(std::floor(gy))));
    const int z0 = std::max(0, std::min(res - 1, static_cast<int>(std::floor(gz))));
    const int x1 = std::min(x0 + 1, res - 1);
    const int y1 = std::min(y0 + 1, res - 1);
    const int z1 = std::min(z0 + 1, res - 1);

    const float tx = clampf(gx - static_cast<float>(x0), 0.0f, 1.0f);
    const float ty = clampf(gy - static_cast<float>(y0), 0.0f, 1.0f);
    const float tz = clampf(gz - static_cast<float>(z0), 0.0f, 1.0f);

    const std::size_t resZStride = static_cast<std::size_t>(res) * static_cast<std::size_t>(res);
    const auto sampleAt = [&](int x, int y, int z) -> float {
        const std::size_t index = static_cast<std::size_t>(x) +
                                  static_cast<std::size_t>(res) * static_cast<std::size_t>(y) +
                                  resZStride * static_cast<std::size_t>(z);
        return volume->samples[index];
    };

    const float c000 = sampleAt(x0, y0, z0);
    const float c100 = sampleAt(x1, y0, z0);
    const float c010 = sampleAt(x0, y1, z0);
    const float c110 = sampleAt(x1, y1, z0);
    const float c001 = sampleAt(x0, y0, z1);
    const float c101 = sampleAt(x1, y0, z1);
    const float c011 = sampleAt(x0, y1, z1);
    const float c111 = sampleAt(x1, y1, z1);

    const float c00 = lerpf(c000, c100, tx);
    const float c10 = lerpf(c010, c110, tx);
    const float c01 = lerpf(c001, c101, tx);
    const float c11 = lerpf(c011, c111, tx);

    const float c0 = lerpf(c00, c10, ty);
    const float c1 = lerpf(c01, c11, ty);
    const float sampled = lerpf(c0, c1, tz);
    const float voxelSize = 2.0f / static_cast<float>(res);
    const float conservativeMargin = 0.5f * std::sqrt(3.0f) * voxelSize;
    return sampled + outsideLength - conservativeMargin;
}

} // namespace rmt
