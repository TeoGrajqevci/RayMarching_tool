#include "Shapes.h"
#include <cmath>
#include <algorithm>
#include <iostream>

void applyRotationInverse(const float p[3], const float angles[3], float out[3])
{
    float cx = cos(-angles[0]), sx = sin(-angles[0]);
    float cy = cos(-angles[1]), sy = sin(-angles[1]);
    float cz = cos(-angles[2]), sz = sin(-angles[2]);
    float rx[3] = { p[0],
                    cx * p[1] - sx * p[2],
                    sx * p[1] + cx * p[2] };
    float ry[3] = { cy * rx[0] + sy * rx[2],
                    rx[1],
                    -sy * rx[0] + cy * rx[2] };
    float rz[3] = { cz * ry[0] - sz * ry[1],
                    sz * ry[0] + cz * ry[1],
                    ry[2] };
    out[0] = rz[0]; out[1] = rz[1]; out[2] = rz[2];
}

float sdSphereCPU(const float p[3], const Shape &shape) {
    float dx = p[0] - shape.center[0];
    float dy = p[1] - shape.center[1];
    float dz = p[2] - shape.center[2];
    return std::sqrt(dx*dx + dy*dy + dz*dz) - shape.param[0];
}

float sdBoxCPU(const float p[3], const Shape &shape) {
    float rel[3] = { p[0] - shape.center[0],
                     p[1] - shape.center[1],
                     p[2] - shape.center[2] };
    float local[3];
    applyRotationInverse(rel, shape.rotation, local);
    float dx = std::fabs(local[0]) - shape.param[0];
    float dy = std::fabs(local[1]) - shape.param[1];
    float dz = std::fabs(local[2]) - shape.param[2];
    float ax = std::max(dx, 0.0f);
    float ay = std::max(dy, 0.0f);
    float az = std::max(dz, 0.0f);
    float outside = std::sqrt(ax*ax + ay*ay + az*az);
    float inside = std::min(std::max(dx, std::max(dy, dz)), 0.0f);
    return outside + inside;
}

float sdRoundBoxCPU(const float p[3], const Shape &shape) {
    float rel[3] = { p[0] - shape.center[0],
                     p[1] - shape.center[1],
                     p[2] - shape.center[2] };
    float local[3];
    applyRotationInverse(rel, shape.rotation, local);
    float dx = std::fabs(local[0]) - shape.param[0];
    float dy = std::fabs(local[1]) - shape.param[1];
    float dz = std::fabs(local[2]) - shape.param[2];
    float ax = std::max(dx, 0.0f);
    float ay = std::max(dy, 0.0f);
    float az = std::max(dz, 0.0f);
    float outside = std::sqrt(ax*ax + ay*ay + az*az);
    float inside = std::min(std::max(dx, std::max(dy, dz)), 0.0f);
    return outside + inside - shape.extra;
}

float sdTorusCPU(const float p[3], const Shape &shape) {
    float rel[3] = { p[0] - shape.center[0],
                     p[1] - shape.center[1],
                     p[2] - shape.center[2] };
    float local[3];
    applyRotationInverse(rel, shape.rotation, local);
    float q = std::sqrt(local[0]*local[0] + local[2]*local[2]) - shape.param[0];
    return std::sqrt(q*q + local[1]*local[1]) - shape.param[1];
}

float sdCylinderCPU(const float p[3], const Shape &shape) {
    float rel[3] = { p[0] - shape.center[0],
                     p[1] - shape.center[1],
                     p[2] - shape.center[2] };
    float local[3];
    applyRotationInverse(rel, shape.rotation, local);
    float q = std::sqrt(local[0]*local[0] + local[2]*local[2]) - shape.param[0];
    float d = std::fabs(local[1]) - shape.param[1];
    return std::min(std::max(q, d), 0.0f) + std::sqrt(std::max(q,0.0f)*std::max(q,0.0f) + std::max(d,0.0f)*std::max(d,0.0f));
}

int pickRayMarchSDF(const float rayOrigin[3], const float rayDir[3],
                      const std::vector<Shape>& shapes,
                      float& outT) {
    const int MAX_STEPS = 100;
    const float MAX_DIST = 100.0f;
    const float SURFACE_DIST = 0.001f;
    
    float t = 0.0f;
    int hitShape = -1;
    for (int i = 0; i < MAX_STEPS; i++) {
         float p[3] = { rayOrigin[0] + rayDir[0] * t,
                        rayOrigin[1] + rayDir[1] * t,
                        rayOrigin[2] + rayDir[2] * t };
         float minDist = MAX_DIST;
         int currentShape = -1;
         for (size_t j = 0; j < shapes.size(); j++) {
              float d = MAX_DIST;
              if (shapes[j].type == SHAPE_SPHERE)
                  d = sdSphereCPU(p, shapes[j]);
              else if (shapes[j].type == SHAPE_BOX)
                  d = sdBoxCPU(p, shapes[j]);
              else if (shapes[j].type == SHAPE_ROUNDBOX)
                  d = sdRoundBoxCPU(p, shapes[j]);
              else if (shapes[j].type == SHAPE_TORUS)
                  d = sdTorusCPU(p, shapes[j]);
              else if (shapes[j].type == SHAPE_CYLINDER)
                  d = sdCylinderCPU(p, shapes[j]);
              if (d < minDist) {
                  minDist = d;
                  currentShape = static_cast<int>(j);
              }
         }
         if (minDist < SURFACE_DIST) {
              hitShape = currentShape;
              break;
         }
         t += minDist;
         if (t > MAX_DIST)
             break;
    }
    outT = t;
    return hitShape;
}