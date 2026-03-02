#include "rmt/io/mesh/import/ImportedShapeFactory.h"

#include <string>

#include "MeshImportInternals.h"

namespace rmt {

std::string getDisplayNameFromPath(const std::string& path) {
    const std::string::size_type slash = path.find_last_of("/\\");
    std::string filename = (slash == std::string::npos) ? path : path.substr(slash + 1);
    const std::string::size_type dot = filename.find_last_of('.');
    if (dot != std::string::npos) {
        filename = filename.substr(0, dot);
    }
    if (filename.empty()) {
        return "MeshSDF";
    }
    return filename;
}

Shape buildDefaultMeshShape(const std::string& path, int volumeId) {
    Shape shape;
    shape.type = SHAPE_MESH_SDF;
    shape.center[0] = 0.0f;
    shape.center[1] = 0.0f;
    shape.center[2] = 0.0f;
    shape.param[0] = 0.75f;
    shape.param[1] = static_cast<float>(volumeId);
    shape.param[2] = 0.0f;
    shape.fractalExtra[0] = 1.0f;
    shape.fractalExtra[1] = 1.0f;
    shape.rotation[0] = 0.0f;
    shape.rotation[1] = 0.0f;
    shape.rotation[2] = 0.0f;
    shape.extra = 0.0f;
    shape.scale[0] = 1.0f;
    shape.scale[1] = 1.0f;
    shape.scale[2] = 1.0f;
    shape.scaleMode = SCALE_MODE_ELONGATE;
    shape.elongation[0] = 0.0f;
    shape.elongation[1] = 0.0f;
    shape.elongation[2] = 0.0f;
    shape.twist[0] = 0.0f;
    shape.twist[1] = 0.0f;
    shape.twist[2] = 0.0f;
    shape.bend[0] = 0.0f;
    shape.bend[1] = 0.0f;
    shape.bend[2] = 0.0f;
    shape.name = getDisplayNameFromPath(path);
    shape.meshSourcePath = path;
    shape.blendOp = BLEND_NONE;
    shape.smoothness = 0.1f;
    shape.albedo[0] = 0.85f;
    shape.albedo[1] = 0.85f;
    shape.albedo[2] = 0.85f;
    shape.metallic = 0.0f;
    shape.roughness = 0.1f;
    shape.emission[0] = 0.0f;
    shape.emission[1] = 0.0f;
    shape.emission[2] = 0.0f;
    shape.emissionStrength = 0.0f;
    shape.transmission = 0.0f;
    shape.ior = 1.5f;
    shape.dispersion = 0.0f;
    shape.bendModifierEnabled = false;
    shape.twistModifierEnabled = false;
    shape.mirrorModifierEnabled = false;
    shape.mirrorAxis[0] = false;
    shape.mirrorAxis[1] = false;
    shape.mirrorAxis[2] = false;
    shape.mirrorOffset[0] = 0.0f;
    shape.mirrorOffset[1] = 0.0f;
    shape.mirrorOffset[2] = 0.0f;
    shape.mirrorSmoothness = 0.0f;
    shape.arrayModifierEnabled = false;
    shape.arrayAxis[0] = false;
    shape.arrayAxis[1] = false;
    shape.arrayAxis[2] = false;
    shape.arraySpacing[0] = 2.0f;
    shape.arraySpacing[1] = 2.0f;
    shape.arraySpacing[2] = 2.0f;
    shape.arrayRepeatCount[0] = 3;
    shape.arrayRepeatCount[1] = 3;
    shape.arrayRepeatCount[2] = 3;
    shape.arraySmoothness = 0.0f;
    shape.modifierStack[0] = SHAPE_MODIFIER_BEND;
    shape.modifierStack[1] = SHAPE_MODIFIER_TWIST;
    shape.modifierStack[2] = SHAPE_MODIFIER_ARRAY;
    return shape;
}

} // namespace rmt
