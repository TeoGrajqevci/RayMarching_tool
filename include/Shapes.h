#ifndef SHAPES_H
#define SHAPES_H

#include <string>
#include <vector>
#include <array>
#include <cstdint>

enum ShapeType {
    SHAPE_SPHERE = 0,
    SHAPE_BOX = 1,
    SHAPE_TORUS = 2,
    SHAPE_CYLINDER = 3,
    SHAPE_CONE = 4,
    SHAPE_MANDELBULB = 5
};
enum BlendMode { BLEND_NONE = 0, BLEND_SMOOTH_UNION = 1, BLEND_SMOOTH_SUBTRACTION = 2, BLEND_SMOOTH_INTERSECTION = 3 };
enum ScaleMode { SCALE_MODE_DEFORM = 0, SCALE_MODE_ELONGATE = 1 };

struct Shape {
    int type;
    float center[3];
    float param[3]; 
    float rotation[3]; 
    float extra; // Rounding radius
    float scale[3];  // Non-uniform scaling per axis
    int scaleMode; // 0: Deform scale, 1: Elongation
    float elongation[3];
    float twist[3]; // Twist amount per axis (X, Y, Z), applied cumulatively
    float bend[3];  // Bend amount per axis (X, Y, Z), applied cumulatively
    std::string name;
    int blendOp; // 0: None, 1: Smooth Union, 2: Smooth Subtraction, 3: Smooth Intersection
    float smoothness;
    float albedo[3];
    float metallic;
    float roughness;
    float emission[3] = { 0.0f, 0.0f, 0.0f };
    float emissionStrength = 0.0f;
    float transmission = 0.0f;
    float ior = 1.5f;
    bool bendModifierEnabled = false;
    bool twistModifierEnabled = false;
    bool mirrorModifierEnabled = false;
    bool mirrorAxis[3] = { false, false, false };
    float mirrorOffset[3] = { 0.0f, 0.0f, 0.0f };
    float mirrorSmoothness = 0.0f;
};

enum RuntimeShapeFlags {
    RUNTIME_SHAPE_HAS_ROTATION = 1 << 0,
    RUNTIME_SHAPE_HAS_SCALE = 1 << 1,
    RUNTIME_SHAPE_HAS_ELONGATION = 1 << 2,
    RUNTIME_SHAPE_HAS_ROUNDING = 1 << 3,
    RUNTIME_SHAPE_HAS_TWIST = 1 << 4,
    RUNTIME_SHAPE_HAS_BEND = 1 << 5,
    RUNTIME_SHAPE_SCALE_MODE_ELONGATE = 1 << 6,
    RUNTIME_SHAPE_HAS_MIRROR = 1 << 7
};

struct RuntimeShapeData {
    int type;
    int blendOp;
    int flags;
    int scaleMode;
    float center[3];
    float param[4];
    float invRotationRows[9];
    float scale[3];
    float elongation[3];
    float roundness;
    float twist[3];
    float bend[3];
    float mirror[3];
    float mirrorOffset[3];
    float mirrorSmoothness;
    float smoothness;
    float boundRadius;
    float influenceRadius;
};

struct ShapeEvalStats {
    std::uint64_t primitiveEvaluations;
    std::uint64_t shapeEvaluations;
    std::uint64_t sceneEvaluations;
};

RuntimeShapeData buildRuntimeShapeData(const Shape& shape);
std::vector<RuntimeShapeData> buildRuntimeShapeDataList(const std::vector<Shape>& shapes);
float evaluateRuntimeShapeDistance(const float p[3], const RuntimeShapeData& shapeData);
float evaluateRuntimeSceneDistance(const float p[3], const std::vector<RuntimeShapeData>& shapeDataList);
ShapeEvalStats getShapeEvalStats();
void resetShapeEvalStats();

float sdSphereCPU(const float p[3], const Shape &shape);
float sdBoxCPU(const float p[3], const Shape &shape);
float sdTorusCPU(const float p[3], const Shape &shape);
float sdCylinderCPU(const float p[3], const Shape &shape);
float sdConeCPU(const float p[3], const Shape &shape);
float sdMandelbulbCPU(const float p[3], const Shape& shape);
void applyRotationInverse(const float p[3], const float angles[3], float out[3]);
int pickRayMarchSDF(const float rayOrigin[3], const float rayDir[3],
                      const std::vector<Shape>& shapes,
                      float& outT);

#endif
