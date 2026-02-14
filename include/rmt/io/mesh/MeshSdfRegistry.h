#ifndef RMT_IO_MESH_SDF_REGISTRY_H
#define RMT_IO_MESH_SDF_REGISTRY_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace rmt {

struct MeshSDFVolume {
    int id;
    int resolution;
    std::string sourcePath;
    std::vector<float> samples;
};

class MeshSDFRegistry {
public:
    static MeshSDFRegistry& getInstance();

    int addVolume(const std::string& sourcePath, int resolution, std::vector<float>&& samples);
    const MeshSDFVolume* getVolume(int id) const;
    std::size_t getVolumeCount() const;
    int getSharedResolution() const;
    std::uint64_t getVersion() const;

private:
    MeshSDFRegistry();

    std::vector<MeshSDFVolume> volumes;
    int sharedResolution;
    std::uint64_t version;
};

float sampleMeshSDFNormalized(int volumeId, const float pNormalized[3]);

} // namespace rmt

#endif // RMT_IO_MESH_SDF_REGISTRY_H
