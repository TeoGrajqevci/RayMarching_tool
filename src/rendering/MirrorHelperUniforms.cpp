#include "rmt/rendering/MirrorHelperUniforms.h"

#include <algorithm>

namespace rmt {

MirrorHelperUniformState::MirrorHelperUniformState()
    : show(0),
      selected(0),
      selectedAxis(-1),
      axisEnabled{0.0f, 0.0f, 0.0f},
      offset{0.0f, 0.0f, 0.0f},
      center{0.0f, 0.0f, 0.0f},
      extent(1.0f) {}

MirrorHelperUniformState buildMirrorHelperUniformState(const std::vector<Shape>& shapes,
                                                       const std::vector<int>& selectedShapes,
                                                       const TransformationState& transformState) {
    MirrorHelperUniformState state;

    const auto setupForShape = [&](int shapeIndex, MirrorHelperUniformState& outState) -> bool {
        if (shapeIndex < 0 || shapeIndex >= static_cast<int>(shapes.size())) {
            return false;
        }

        const Shape& shape = shapes[static_cast<std::size_t>(shapeIndex)];
        if (!shape.mirrorModifierEnabled ||
            (!shape.mirrorAxis[0] && !shape.mirrorAxis[1] && !shape.mirrorAxis[2])) {
            return false;
        }

        outState.show = 1;
        outState.axisEnabled[0] = shape.mirrorAxis[0] ? 1.0f : 0.0f;
        outState.axisEnabled[1] = shape.mirrorAxis[1] ? 1.0f : 0.0f;
        outState.axisEnabled[2] = shape.mirrorAxis[2] ? 1.0f : 0.0f;
        outState.offset[0] = shape.mirrorOffset[0];
        outState.offset[1] = shape.mirrorOffset[1];
        outState.offset[2] = shape.mirrorOffset[2];
        outState.center[0] = shape.center[0];
        outState.center[1] = shape.center[1];
        outState.center[2] = shape.center[2];

        const RuntimeShapeData runtimeShape = buildRuntimeShapeData(shape);
        outState.extent = std::max(0.5f, runtimeShape.boundRadius * 1.5f);

        if (transformState.mirrorHelperSelected &&
            transformState.mirrorHelperShapeIndex == shapeIndex &&
            transformState.mirrorHelperAxis >= 0 &&
            transformState.mirrorHelperAxis < 3 &&
            shape.mirrorAxis[transformState.mirrorHelperAxis]) {
            outState.selected = 1;
            outState.selectedAxis = transformState.mirrorHelperAxis;
        }

        return true;
    };

    bool configured = false;
    if (transformState.mirrorHelperSelected) {
        configured = setupForShape(transformState.mirrorHelperShapeIndex, state);
    }
    if (!configured && !selectedShapes.empty()) {
        setupForShape(selectedShapes.front(), state);
    }

    return state;
}

void uploadMirrorHelperUniforms(const Shader& shader,
                                const MirrorHelperUniformState& state) {
    shader.setInt("uMirrorHelperShow", state.show);
    shader.setVec3("uMirrorHelperAxisEnabled",
                   state.axisEnabled[0], state.axisEnabled[1], state.axisEnabled[2]);
    shader.setVec3("uMirrorHelperOffset",
                   state.offset[0], state.offset[1], state.offset[2]);
    shader.setVec3("uMirrorHelperCenter",
                   state.center[0], state.center[1], state.center[2]);
    shader.setFloat("uMirrorHelperExtent", state.extent);
    shader.setInt("uMirrorHelperSelected", state.selected);
    shader.setInt("uMirrorHelperSelectedAxis", state.selectedAxis);
}

} // namespace rmt
