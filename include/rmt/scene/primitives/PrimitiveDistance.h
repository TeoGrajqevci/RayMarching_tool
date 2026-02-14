#pragma once

#include "rmt/scene/RuntimeShapeData.h"

namespace rmt {

struct Shape;

float primitiveDistanceCPU(int primitiveType, const float p[3], const RuntimeShapeData& shapeData);
float primitiveDistanceScaledCPU(int primitiveType, const float p[3], const RuntimeShapeData& shapeData);
float primitiveRadiusForShape(int primitiveType, const float param[4]);

float evaluatePrimitiveDistanceCPU(const float p[3], const Shape& shape, int primitiveType);

void applyRotationInverse(const float p[3], const float angles[3], float out[3]);

float sdSphereCPU(const float p[3], const Shape& shape);
float sdBoxCPU(const float p[3], const Shape& shape);
float sdTorusCPU(const float p[3], const Shape& shape);
float sdCylinderCPU(const float p[3], const Shape& shape);
float sdConeCPU(const float p[3], const Shape& shape);
float sdMandelbulbCPU(const float p[3], const Shape& shape);
float sdMengerSpongeCPU(const float p[3], const Shape& shape);

} // namespace rmt
