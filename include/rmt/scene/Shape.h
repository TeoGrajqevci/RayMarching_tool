#ifndef RMT_SCENE_SHAPE_H
#define RMT_SCENE_SHAPE_H

#include <string>
#include <vector>

#include "rmt/scene/ShapeEnums.h"
#include "rmt/scene/RuntimeShapeData.h"
#include "rmt/scene/EvalStats.h"
#include "rmt/scene/RuntimeShapeBuilder.h"
#include "rmt/scene/RuntimeSceneEvaluator.h"
#include "rmt/scene/primitives/PrimitiveDistance.h"

namespace rmt {

struct CurveNode {
    float position[3] = { 0.0f, 0.0f, 0.0f };
    float radius = 0.1f;
};

struct Shape {
    int type;
    float center[3];
    float param[3];
    float fractalExtra[2] = { 1.0f, 1.0f };
    float rotation[3];
    float extra;
    float scale[3];
    int scaleMode;
    float elongation[3];
    float twist[3];
    float bend[3];
    std::string name;
    std::string meshSourcePath;
    int blendOp;
    float smoothness;
    float albedo[3];
    float metallic;
    float roughness;
    std::string albedoTexturePath;
    std::string roughnessTexturePath;
    std::string metallicTexturePath;
    std::string normalTexturePath;
    std::string displacementTexturePath;
    float displacementStrength = 0.0f;
    float emission[3] = { 0.0f, 0.0f, 0.0f };
    float emissionStrength = 0.0f;
    float transmission = 0.0f;
    float ior = 1.5f;
    float dispersion = 0.0f;
    std::vector<CurveNode> curveNodes;
    bool bendModifierEnabled = false;
    bool twistModifierEnabled = false;
    bool mirrorModifierEnabled = false;
    bool mirrorAxis[3] = { false, false, false };
    float mirrorOffset[3] = { 0.0f, 0.0f, 0.0f };
    float mirrorSmoothness = 0.0f;
    bool arrayModifierEnabled = false;
    bool arrayAxis[3] = { false, false, false };
    float arraySpacing[3] = { 2.0f, 2.0f, 2.0f };
    int arrayRepeatCount[3] = { 3, 3, 3 };
    float arraySmoothness = 0.0f;
    int modifierStack[3] = { SHAPE_MODIFIER_BEND, SHAPE_MODIFIER_TWIST, SHAPE_MODIFIER_ARRAY };
};

int pickRayMarchSDF(const float rayOrigin[3], const float rayDir[3],
                    const std::vector<Shape>& shapes,
                    float& outT);

} // namespace rmt

#endif // RMT_SCENE_SHAPE_H
