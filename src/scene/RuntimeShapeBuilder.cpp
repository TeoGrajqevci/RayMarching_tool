#include "rmt/scene/RuntimeShapeBuilder.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "rmt/scene/Shape.h"
#include "rmt/scene/modifiers/ShapeModifiers.h"
#include "rmt/scene/primitives/PrimitiveDistance.h"

namespace rmt {

namespace {

void sanitizeModifierStack(const int inStack[3], int outStack[3]) {
    const int defaults[3] = { SHAPE_MODIFIER_BEND, SHAPE_MODIFIER_TWIST, SHAPE_MODIFIER_ARRAY };
    bool used[3] = { false, false, false };
    int writeIndex = 0;

    for (int i = 0; i < 3; ++i) {
        const int value = inStack[i];
        if (value < SHAPE_MODIFIER_BEND || value > SHAPE_MODIFIER_ARRAY) {
            continue;
        }
        if (!used[value]) {
            outStack[writeIndex++] = value;
            used[value] = true;
        }
    }

    for (int i = 0; i < 3; ++i) {
        const int value = defaults[i];
        if (!used[value]) {
            outStack[writeIndex++] = value;
            used[value] = true;
        }
    }
}

void finalizeRuntimeShapeRadii(RuntimeShapeData& out, float displacementPadding) {
    float baseRadius = 0.0f;
    if (out.type == SHAPE_CURVE) {
        baseRadius = 0.01f;
        for (int i = 0; i < out.curveNodeCount; ++i) {
            const int base = i * 4;
            const float nx = out.curveNodes[base + 0];
            const float ny = out.curveNodes[base + 1];
            const float nz = out.curveNodes[base + 2];
            const float radius = std::max(out.curveNodes[base + 3], 0.001f);
            const float d = std::sqrt(nx * nx + ny * ny + nz * nz) + radius;
            baseRadius = std::max(baseRadius, d);
        }
    } else {
        baseRadius = primitiveRadiusForShape(out.type, out.param);
    }
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

    if ((out.flags & RUNTIME_SHAPE_HAS_ARRAY) != 0) {
        const float arrayOffsetX = out.arrayAxis[0] > 0.5f
            ? 0.5f * std::max(out.arrayRepeatCount[0] - 1.0f, 0.0f) * std::max(out.arraySpacing[0], 0.0f)
            : 0.0f;
        const float arrayOffsetY = out.arrayAxis[1] > 0.5f
            ? 0.5f * std::max(out.arrayRepeatCount[1] - 1.0f, 0.0f) * std::max(out.arraySpacing[1], 0.0f)
            : 0.0f;
        const float arrayOffsetZ = out.arrayAxis[2] > 0.5f
            ? 0.5f * std::max(out.arrayRepeatCount[2] - 1.0f, 0.0f) * std::max(out.arraySpacing[2], 0.0f)
            : 0.0f;
        const float arrayCenterOffset = std::sqrt(arrayOffsetX * arrayOffsetX +
                                                  arrayOffsetY * arrayOffsetY +
                                                  arrayOffsetZ * arrayOffsetZ);
        out.boundRadius += arrayCenterOffset;
        out.influenceRadius += arrayCenterOffset + std::max(out.arraySmoothness, 0.0f);
    }

    const float safeDisplacementPadding = std::max(displacementPadding, 0.0f);
    out.boundRadius += safeDisplacementPadding;
    out.influenceRadius += safeDisplacementPadding;
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
        case SHAPE_CURVE:
            out[0] = std::max(shape.param[0], 0.01f);
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
    RuntimeShapeData out{};
    out.type = shape.type;
    out.blendOp = shape.blendOp;
    out.flags = 0;
    out.scaleMode = shape.scaleMode;

    out.center[0] = shape.center[0];
    out.center[1] = shape.center[1];
    out.center[2] = shape.center[2];

    packPrimitiveParameters(shape, shape.type, out.param);
    packFractalExtraParameters(shape, shape.type, out.fractalExtra);

    out.curveNodeCount = 0;
    for (int i = 0; i < RuntimeShapeData::kMaxCurveNodes * 4; ++i) {
        out.curveNodes[i] = 0.0f;
    }

    if (shape.type == SHAPE_CURVE) {
        const int nodeCount = std::min(static_cast<int>(shape.curveNodes.size()), RuntimeShapeData::kMaxCurveNodes);
        out.curveNodeCount = std::max(nodeCount, 1);

        if (nodeCount <= 0) {
            out.curveNodes[0] = 0.0f;
            out.curveNodes[1] = 0.0f;
            out.curveNodes[2] = 0.0f;
            out.curveNodes[3] = std::max(shape.param[0], 0.05f);
        } else {
            for (int i = 0; i < nodeCount; ++i) {
                const CurveNode& node = shape.curveNodes[static_cast<std::size_t>(i)];
                const int base = i * 4;
                out.curveNodes[base + 0] = node.position[0];
                out.curveNodes[base + 1] = node.position[1];
                out.curveNodes[base + 2] = node.position[2];
                out.curveNodes[base + 3] = std::max(node.radius, 0.001f);
            }
        }
    }

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

    const int repeatX = std::max(shape.arrayRepeatCount[0], 1);
    const int repeatY = std::max(shape.arrayRepeatCount[1], 1);
    const int repeatZ = std::max(shape.arrayRepeatCount[2], 1);
    const bool arrayXActive = shape.arrayModifierEnabled && shape.arrayAxis[0] && repeatX > 1;
    const bool arrayYActive = shape.arrayModifierEnabled && shape.arrayAxis[1] && repeatY > 1;
    const bool arrayZActive = shape.arrayModifierEnabled && shape.arrayAxis[2] && repeatZ > 1;

    out.arrayAxis[0] = arrayXActive ? 1.0f : 0.0f;
    out.arrayAxis[1] = arrayYActive ? 1.0f : 0.0f;
    out.arrayAxis[2] = arrayZActive ? 1.0f : 0.0f;
    out.arraySpacing[0] = arrayXActive ? std::max(std::fabs(shape.arraySpacing[0]), 1e-4f) : 0.0f;
    out.arraySpacing[1] = arrayYActive ? std::max(std::fabs(shape.arraySpacing[1]), 1e-4f) : 0.0f;
    out.arraySpacing[2] = arrayZActive ? std::max(std::fabs(shape.arraySpacing[2]), 1e-4f) : 0.0f;
    out.arrayRepeatCount[0] = arrayXActive ? static_cast<float>(repeatX) : 1.0f;
    out.arrayRepeatCount[1] = arrayYActive ? static_cast<float>(repeatY) : 1.0f;
    out.arrayRepeatCount[2] = arrayZActive ? static_cast<float>(repeatZ) : 1.0f;
    out.arraySmoothness = shape.arrayModifierEnabled ? std::max(shape.arraySmoothness, 0.0f) : 0.0f;

    int sanitizedModifierStack[3] = { SHAPE_MODIFIER_BEND, SHAPE_MODIFIER_TWIST, SHAPE_MODIFIER_ARRAY };
    sanitizeModifierStack(shape.modifierStack, sanitizedModifierStack);
    out.modifierStack[0] = static_cast<float>(sanitizedModifierStack[0]);
    out.modifierStack[1] = static_cast<float>(sanitizedModifierStack[1]);
    out.modifierStack[2] = static_cast<float>(sanitizedModifierStack[2]);

    if (arrayXActive || arrayYActive || arrayZActive) {
        out.flags |= RUNTIME_SHAPE_HAS_ARRAY;
    } else {
        out.arraySpacing[0] = 0.0f;
        out.arraySpacing[1] = 0.0f;
        out.arraySpacing[2] = 0.0f;
        out.arrayRepeatCount[0] = 1.0f;
        out.arrayRepeatCount[1] = 1.0f;
        out.arrayRepeatCount[2] = 1.0f;
        out.arraySmoothness = 0.0f;
    }

    out.smoothness = std::max(shape.smoothness, 1e-4f);
    if (shape.scaleMode == SCALE_MODE_ELONGATE) {
        out.flags |= RUNTIME_SHAPE_SCALE_MODE_ELONGATE;
    }

    finalizeRuntimeShapeRadii(out, shape.displacementStrength);
    return out;
}

RuntimeShapeData buildRuntimeShapeDataForPrimitive(const Shape& shape, int primitiveType) {
    RuntimeShapeData out = buildRuntimeShapeData(shape);
    out.type = primitiveType;
    packPrimitiveParameters(shape, primitiveType, out.param);
    packFractalExtraParameters(shape, primitiveType, out.fractalExtra);
    finalizeRuntimeShapeRadii(out, shape.displacementStrength);
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
