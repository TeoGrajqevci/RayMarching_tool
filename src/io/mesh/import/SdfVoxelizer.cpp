#include "rmt/io/mesh/import/SdfVoxelizer.h"

#include <cmath>
#include <vector>

#include "MeshImportInternals.h"

#include "makelevelset3.h"

namespace rmt {

bool voxelizeMeshToSdfSamples(const std::vector<Vec3f>& vertices,
                              const std::vector<Vec3ui>& faces,
                              int sdfResolution,
                              std::vector<float>& outSamples,
                              std::string& outError) {
    if (sdfResolution < 16 || sdfResolution > 256) {
        outError = "Invalid SDF resolution.";
        return false;
    }

    Array3f sdfGrid;
    const Vec3f origin(-1.0f, -1.0f, -1.0f);
    const float dx = 2.0f / static_cast<float>(sdfResolution);
    make_level_set3(faces, vertices, origin, dx, sdfResolution, sdfResolution, sdfResolution, sdfGrid);

    const std::size_t voxelCount = static_cast<std::size_t>(sdfResolution) *
                                   static_cast<std::size_t>(sdfResolution) *
                                   static_cast<std::size_t>(sdfResolution);
    outSamples.assign(voxelCount, 0.0f);

    const std::size_t zStride = static_cast<std::size_t>(sdfResolution) * static_cast<std::size_t>(sdfResolution);
    for (int z = 0; z < sdfResolution; ++z) {
        for (int y = 0; y < sdfResolution; ++y) {
            for (int x = 0; x < sdfResolution; ++x) {
                const std::size_t index = static_cast<std::size_t>(x) +
                                          static_cast<std::size_t>(sdfResolution) * static_cast<std::size_t>(y) +
                                          zStride * static_cast<std::size_t>(z);
                const float value = sdfGrid(x, y, z);
                outSamples[index] = std::isfinite(value) ? value : 1e3f;
            }
        }
    }

    return true;
}

} // namespace rmt
