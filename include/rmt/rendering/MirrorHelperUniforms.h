#ifndef RMT_RENDERING_MIRROR_HELPER_UNIFORMS_H
#define RMT_RENDERING_MIRROR_HELPER_UNIFORMS_H

#include <vector>

#include "rmt/input/Input.h"
#include "rmt/rendering/ShaderProgram.h"
#include "rmt/scene/Shape.h"

namespace rmt {

struct MirrorHelperUniformState {
    int show;
    int selected;
    int selectedAxis;
    float axisEnabled[3];
    float offset[3];
    float center[3];
    float extent;

    MirrorHelperUniformState();
};

MirrorHelperUniformState buildMirrorHelperUniformState(const std::vector<Shape>& shapes,
                                                       const std::vector<int>& selectedShapes,
                                                       const TransformationState& transformState);

void uploadMirrorHelperUniforms(const Shader& shader,
                                const MirrorHelperUniformState& state);

} // namespace rmt

#endif // RMT_RENDERING_MIRROR_HELPER_UNIFORMS_H
