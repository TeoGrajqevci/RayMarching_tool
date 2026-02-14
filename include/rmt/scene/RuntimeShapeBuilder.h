#pragma once

#include <vector>

#include "rmt/scene/RuntimeShapeData.h"

namespace rmt {

struct Shape;

void packPrimitiveParameters(const Shape& shape, int primitiveType, float out[4]);
void packFractalExtraParameters(const Shape& shape, int primitiveType, float out[2]);

RuntimeShapeData buildRuntimeShapeData(const Shape& shape);
RuntimeShapeData buildRuntimeShapeDataForPrimitive(const Shape& shape, int primitiveType);
std::vector<RuntimeShapeData> buildRuntimeShapeDataList(const std::vector<Shape>& shapes);

} // namespace rmt
