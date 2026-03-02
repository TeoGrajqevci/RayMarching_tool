#pragma once

#include "rmt/scene/ShapeEnums.h"

namespace rmt {

struct RuntimeShapeData {
    static constexpr int kMaxCurveNodes = 64;

    int type;
    int blendOp;
    int flags;
    int scaleMode;
    float center[3];
    float param[4];
    float fractalExtra[2];
    float invRotationRows[9];
    float scale[3];
    float elongation[3];
    float roundness;
    float twist[3];
    float bend[3];
    float mirror[3];
    float mirrorOffset[3];
    float mirrorSmoothness;
    float arrayAxis[3];
    float arraySpacing[3];
    float arrayRepeatCount[3];
    float arraySmoothness;
    float modifierStack[3];
    float smoothness;
    float boundRadius;
    float influenceRadius;
    int curveNodeCount;
    float curveNodes[kMaxCurveNodes * 4];
};

} // namespace rmt
