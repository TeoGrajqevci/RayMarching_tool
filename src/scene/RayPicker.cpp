#include "rmt/scene/RayPicker.h"

#include <algorithm>
#include <cmath>

namespace rmt {

int pickRayMarchSDF(const float rayOrigin[3], const float rayDir[3],
                    const std::vector<Shape>& shapes,
                    float& outT) {
    const int maxPickSteps = 128;
    const float maxPickDist = 100.0f;
    const float pickSurfaceDist = 0.001f;

    const std::vector<RuntimeShapeData> runtimeShapes = buildRuntimeShapeDataList(shapes);
    if (runtimeShapes.empty()) {
        outT = 0.0f;
        return -1;
    }

    float t = 0.0f;
    int hitShape = -1;

    for (int step = 0; step < maxPickSteps; ++step) {
        const float p[3] = {
            rayOrigin[0] + rayDir[0] * t,
            rayOrigin[1] + rayDir[1] * t,
            rayOrigin[2] + rayDir[2] * t,
        };

        const float sceneDist = evaluateRuntimeSceneDistance(p, runtimeShapes);
        if (sceneDist < pickSurfaceDist) {
            float bestDist = maxPickDist;
            for (std::size_t i = 0; i < runtimeShapes.size(); ++i) {
                const RuntimeShapeData& shapeData = runtimeShapes[i];
                const float dx = p[0] - shapeData.center[0];
                const float dy = p[1] - shapeData.center[1];
                const float dz = p[2] - shapeData.center[2];
                const float lb = std::sqrt(dx * dx + dy * dy + dz * dz) - shapeData.influenceRadius;
                if (lb >= bestDist) {
                    continue;
                }

                const float d = evaluateRuntimeShapeDistance(p, shapeData);
                if (d < bestDist) {
                    bestDist = d;
                    hitShape = static_cast<int>(i);
                }
            }
            break;
        }

        t += std::max(sceneDist, pickSurfaceDist * 0.25f);
        if (t > maxPickDist) {
            break;
        }
    }

    outT = t;
    return hitShape;
}

} // namespace rmt
