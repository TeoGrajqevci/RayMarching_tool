#ifndef RMT_IO_MESH_IMPORTER_H
#define RMT_IO_MESH_IMPORTER_H

#include <string>

#include "rmt/scene/Shape.h"

namespace rmt {

struct MeshImportResult {
    bool success;
    Shape shape;
    std::string message;
};

bool isSupportedMeshImportFile(const std::string& path);
MeshImportResult importMeshAsSDFShape(const std::string& path, int sdfResolution);

} // namespace rmt

#endif // RMT_IO_MESH_IMPORTER_H
