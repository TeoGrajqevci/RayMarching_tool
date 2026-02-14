#include "rmt/io/mesh/import/MeshImporter.h"

#include <string>
#include <vector>

#include "MeshImportInternals.h"

#include "rmt/io/mesh/MeshSdfRegistry.h"

namespace rmt {

bool isSupportedMeshImportFile(const std::string& path) {
    const std::string ext = getFileExtensionLower(path);
    return ext == ".obj" || ext == ".fbx";
}

MeshImportResult importMeshAsSDFShape(const std::string& path, int sdfResolution) {
    MeshImportResult result;
    result.success = false;

    if (!isSupportedMeshImportFile(path)) {
        result.message = "Unsupported file extension. Only .obj and .fbx are accepted.";
        return result;
    }

    std::vector<Vec3f> vertices;
    std::vector<Vec3ui> faces;
    std::string error;
    if (!loadMeshTrianglesWithAssimp(path, vertices, faces, error)) {
        result.message = "Failed to read mesh: " + error;
        return result;
    }

    if (!normalizeMeshToUnitCube(vertices, error)) {
        result.message = "Failed to normalize mesh: " + error;
        return result;
    }

    std::vector<float> samples;
    if (!voxelizeMeshToSdfSamples(vertices, faces, sdfResolution, samples, error)) {
        result.message = "Failed to voxelize mesh: " + error;
        return result;
    }

    const int volumeId = MeshSDFRegistry::getInstance().addVolume(path, sdfResolution, std::move(samples));
    if (volumeId < 0) {
        result.message = "Failed to register mesh SDF volume (resolution mismatch or invalid data).";
        return result;
    }

    result.shape = buildDefaultMeshShape(path, volumeId);
    result.success = true;
    result.message = "Imported mesh SDF volume #" + std::to_string(volumeId);
    return result;
}

} // namespace rmt
