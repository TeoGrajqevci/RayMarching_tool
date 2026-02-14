#include "rmt/io/mesh/import/MeshNormalizer.h"

#include <algorithm>
#include <limits>

#include "MeshImportInternals.h"

namespace rmt {

bool normalizeMeshToUnitCube(std::vector<Vec3f>& vertices, std::string& outError) {
    if (vertices.empty()) {
        outError = "Cannot normalize an empty mesh.";
        return false;
    }

    Vec3f minP(
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max());
    Vec3f maxP(
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max());

    for (std::size_t i = 0; i < vertices.size(); ++i) {
        const Vec3f& p = vertices[i];
        minP[0] = std::min(minP[0], p[0]);
        minP[1] = std::min(minP[1], p[1]);
        minP[2] = std::min(minP[2], p[2]);
        maxP[0] = std::max(maxP[0], p[0]);
        maxP[1] = std::max(maxP[1], p[1]);
        maxP[2] = std::max(maxP[2], p[2]);
    }

    const Vec3f center(
        0.5f * (minP[0] + maxP[0]),
        0.5f * (minP[1] + maxP[1]),
        0.5f * (minP[2] + maxP[2]));

    const float extentX = maxP[0] - minP[0];
    const float extentY = maxP[1] - minP[1];
    const float extentZ = maxP[2] - minP[2];
    const float maxExtent = std::max(extentX, std::max(extentY, extentZ));
    if (maxExtent <= 1e-8f) {
        outError = "Mesh bounding box is degenerate.";
        return false;
    }

    const float invScale = 2.0f / maxExtent;
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        vertices[i][0] = (vertices[i][0] - center[0]) * invScale;
        vertices[i][1] = (vertices[i][1] - center[1]) * invScale;
        vertices[i][2] = (vertices[i][2] - center[2]) * invScale;
    }

    return true;
}

} // namespace rmt
