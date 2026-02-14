#include "rmt/scene/RuntimeShapeBuilder.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "rmt/scene/Shape.h"
#include "rmt/scene/modifiers/ShapeModifiers.h"
#include "rmt/scene/primitives/PrimitiveDistance.h"

namespace rmt {

namespace {

void finalizeRuntimeShapeRadii(RuntimeShapeData& out) {
    float baseRadius = primitiveRadiusForShape(out.type, out.param);
    const float maxScale = std::max(out.scale[0], std::max(out.scale[1], out.scale[2]));
    baseRadius *= maxScale;

    const float ex = std::max(out.elongation[0], 0.0f);
    const float ey = std::max(out.elongation[1], 0.0f);
    const float ez = std::max(out.elongation[2], 0.0f);
    baseRadius += std::sqrt(ex * ex + ey * ey + ez * ez);

    baseRadius += out.roundness;
    const float warpBoost = 1.0f + 0.5f * (absSum3(out.twist) + absSum3(out.bend));
    out.boundRadius = baseRadius * warpBoost + 0.01f;

    float mirrorCenterOffset = 0.0f;
    if ((out.flags & RUNTIME_SHAPE_HAS_MIRROR) != 0) {
        const float dx = out.mirror[0] > 0.5f ? (2.0f * std::fabs(out.center[0] - out.mirrorOffset[0])) : 0.0f;
        const float dy = out.mirror[1] > 0.5f ? (2.0f * std::fabs(out.center[1] - out.mirrorOffset[1])) : 0.0f;
        const float dz = out.mirror[2] > 0.5f ? (2.0f * std::fabs(out.center[2] - out.mirrorOffset[2])) : 0.0f;
        mirrorCenterOffset = std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    out.influenceRadius = out.boundRadius +
                          mirrorCenterOffset +
                          std::max(out.smoothness, 0.0f) +
                          std::max(out.mirrorSmoothness, 0.0f);
}

} // namespace

void packPrimitiveParameters(const Shape& shape, int primitiveType, float out[4]) {
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
    out[3] = 0.0f;

    switch (primitiveType) {
        case SHAPE_SPHERE:
            out[3] = shape.param[0];
            break;
        case SHAPE_BOX:
            out[0] = shape.param[0];
            out[1] = shape.param[1];
            out[2] = shape.param[2];
            break;
        case SHAPE_TORUS:
            out[0] = shape.param[0];
            out[1] = shape.param[1];
            break;
        case SHAPE_CYLINDER:
            out[0] = shape.param[0];
            out[1] = shape.param[1];
            break;
        case SHAPE_CONE:
            out[0] = shape.param[0];
            out[1] = shape.param[1];
            break;
        case SHAPE_MANDELBULB:
            out[0] = shape.param[0];
            out[1] = shape.param[1];
            out[2] = shape.param[2];
            break;
        case SHAPE_MENGER_SPONGE:
            out[0] = shape.param[0];
            out[1] = shape.param[1];
            out[2] = shape.param[2];
            break;
        case SHAPE_MESH_SDF:
            out[0] = std::max(shape.param[0], 0.001f);
            out[1] = shape.param[1];
            break;
        default:
            break;
    }
}

void packFractalExtraParameters(const Shape& shape, int primitiveType, float out[2]) {
    out[0] = 0.0f;
    out[1] = 0.0f;

    if (primitiveType == SHAPE_MANDELBULB) {
        out[0] = (shape.fractalExtra[0] > 0.0f) ? shape.fractalExtra[0] : 1.0f;
        out[1] = (shape.fractalExtra[1] > 0.0f) ? shape.fractalExtra[1] : 1.0f;
    }
}

RuntimeShapeData buildRuntimeShapeData(const Shape& shape) {
    RuntimeShapeData out;
    out.type = shape.type;
    out.blendOp = shape.blendOp;
    out.flags = 0;
    out.scaleMode = shape.scaleMode;

    out.center[0] = shape.center[0];
    out.center[1] = shape.center[1];
    out.center[2] = shape.center[2];

    packPrimitiveParameters(shape, shape.type, out.param);
    packFractalExtraParameters(shape, shape.type, out.fractalExtra);

    const bool hasRotation = anyAbsGreaterThan(shape.rotation, 1e-6f);
    if (hasRotation) {
        buildInverseRotationRows(shape.rotation, out.invRotationRows);
        out.flags |= RUNTIME_SHAPE_HAS_ROTATION;
    } else {
        setIdentity3(out.invRotationRows);
    }

    out.scale[0] = std::max(std::fabs(shape.scale[0]), 0.001f);
    out.scale[1] = std::max(std::fabs(shape.scale[1]), 0.001f);
    out.scale[2] = std::max(std::fabs(shape.scale[2]), 0.001f);
    if (nonUnitScale(out.scale, 1e-5f)) {
        out.flags |= RUNTIME_SHAPE_HAS_SCALE;
    }

    out.elongation[0] = shape.elongation[0];
    out.elongation[1] = shape.elongation[1];
    out.elongation[2] = shape.elongation[2];
    if (anyAbsGreaterThan(out.elongation, 1e-6f)) {
        out.flags |= RUNTIME_SHAPE_HAS_ELONGATION;
    }

    out.roundness = std::max(shape.extra, 0.0f);
    if (out.roundness > 1e-6f) {
        out.flags |= RUNTIME_SHAPE_HAS_ROUNDING;
    }

    out.twist[0] = shape.twistModifierEnabled ? shape.twist[0] : 0.0f;
    out.twist[1] = shape.twistModifierEnabled ? shape.twist[1] : 0.0f;
    out.twist[2] = shape.twistModifierEnabled ? shape.twist[2] : 0.0f;
    if (anyAbsGreaterThan(out.twist, 1e-6f)) {
        out.flags |= RUNTIME_SHAPE_HAS_TWIST;
    }

    out.bend[0] = shape.bendModifierEnabled ? shape.bend[0] : 0.0f;
    out.bend[1] = shape.bendModifierEnabled ? shape.bend[1] : 0.0f;
    out.bend[2] = shape.bendModifierEnabled ? shape.bend[2] : 0.0f;
    if (anyAbsGreaterThan(out.bend, 1e-6f)) {
        out.flags |= RUNTIME_SHAPE_HAS_BEND;
    }

    out.mirror[0] = (shape.mirrorModifierEnabled && shape.mirrorAxis[0]) ? 1.0f : 0.0f;
    out.mirror[1] = (shape.mirrorModifierEnabled && shape.mirrorAxis[1]) ? 1.0f : 0.0f;
    out.mirror[2] = (shape.mirrorModifierEnabled && shape.mirrorAxis[2]) ? 1.0f : 0.0f;
    out.mirrorOffset[0] = shape.mirrorOffset[0];
    out.mirrorOffset[1] = shape.mirrorOffset[1];
    out.mirrorOffset[2] = shape.mirrorOffset[2];
    if (anyAbsGreaterThan(out.mirror, 0.5f)) {
        out.flags |= RUNTIME_SHAPE_HAS_MIRROR;
    } else {
        out.mirrorOffset[0] = 0.0f;
        out.mirrorOffset[1] = 0.0f;
        out.mirrorOffset[2] = 0.0f;
    }

    out.mirrorSmoothness = (out.flags & RUNTIME_SHAPE_HAS_MIRROR) != 0
        ? std::max(shape.mirrorSmoothness, 0.0f)
        : 0.0f;

    out.smoothness = std::max(shape.smoothness, 1e-4f);
    if (shape.scaleMode == SCALE_MODE_ELONGATE) {
        out.flags |= RUNTIME_SHAPE_SCALE_MODE_ELONGATE;
    }

    finalizeRuntimeShapeRadii(out);
    return out;
}

RuntimeShapeData buildRuntimeShapeDataForPrimitive(const Shape& shape, int primitiveType) {
    RuntimeShapeData out = buildRuntimeShapeData(shape);
    out.type = primitiveType;
    packPrimitiveParameters(shape, primitiveType, out.param);
    packFractalExtraParameters(shape, primitiveType, out.fractalExtra);
    finalizeRuntimeShapeRadii(out);
    return out;
}

std::vector<RuntimeShapeData> buildRuntimeShapeDataList(const std::vector<Shape>& shapes) {
    std::vector<RuntimeShapeData> runtimeShapes;
    runtimeShapes.reserve(shapes.size());
    for (std::vector<Shape>::const_iterator it = shapes.begin(); it != shapes.end(); ++it) {
        runtimeShapes.push_back(buildRuntimeShapeData(*it));
    }
    return runtimeShapes;
}

} // namespace rmt
