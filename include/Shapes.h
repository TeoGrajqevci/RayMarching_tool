#ifndef SHAPES_H
#define SHAPES_H

#include <string>
#include <vector>
#include <array>

enum ShapeType { SHAPE_SPHERE = 0, SHAPE_BOX = 1, SHAPE_ROUNDBOX = 2, SHAPE_TORUS = 3, SHAPE_CYLINDER = 4 };
enum BlendMode { BLEND_NONE = 0, BLEND_SMOOTH_UNION = 1, BLEND_SMOOTH_SUBTRACTION = 2, BLEND_SMOOTH_INTERSECTION = 3 };

struct Shape {
    int type;
    float center[3];
    float param[3]; 
    float rotation[3]; 
    float extra; 
    std::string name;
    int blendOp; // 0: None, 1: Smooth Union, 2: Smooth Subtraction, 3: Smooth Intersection
    float smoothness;
    float albedo[3];
    float metallic;
    float roughness;
};

float sdSphereCPU(const float p[3], const Shape &shape);
float sdBoxCPU(const float p[3], const Shape &shape);
float sdRoundBoxCPU(const float p[3], const Shape &shape);
float sdTorusCPU(const float p[3], const Shape &shape);
float sdCylinderCPU(const float p[3], const Shape &shape);
void applyRotationInverse(const float p[3], const float angles[3], float out[3]);
int pickRayMarchSDF(const float rayOrigin[3], const float rayDir[3],
                      const std::vector<Shape>& shapes,
                      float& outT);

#endif
