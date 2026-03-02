#include "rmt/rendering/SceneHasher.h"

#include <algorithm>
#include <cmath>

#include "rmt/rendering/Renderer.h"
#include "RendererInternal.h"

namespace rmt {

using namespace renderer_internal;

namespace {

constexpr int kMaxPointLights = 16;

void hashString(std::uint64_t& hash, const std::string& value) {
    const std::size_t size = value.size();
    hashValue(hash, size);
    if (size > 0u) {
        hashRaw(hash, value.data(), size);
    }
}

} // namespace

void hashShapeMaterialState(std::uint64_t& hash, const Shape& shape) {
    hashValue(hash, shape.type);
    hashRaw(hash, shape.center, sizeof(shape.center));
    hashRaw(hash, shape.param, sizeof(shape.param));
    hashRaw(hash, shape.fractalExtra, sizeof(shape.fractalExtra));
    hashRaw(hash, shape.rotation, sizeof(shape.rotation));
    hashValue(hash, shape.extra);
    hashRaw(hash, shape.scale, sizeof(shape.scale));
    hashValue(hash, shape.scaleMode);
    hashRaw(hash, shape.elongation, sizeof(shape.elongation));

    float twist[3] = {
        shape.twistModifierEnabled ? shape.twist[0] : 0.0f,
        shape.twistModifierEnabled ? shape.twist[1] : 0.0f,
        shape.twistModifierEnabled ? shape.twist[2] : 0.0f
    };
    float bend[3] = {
        shape.bendModifierEnabled ? shape.bend[0] : 0.0f,
        shape.bendModifierEnabled ? shape.bend[1] : 0.0f,
        shape.bendModifierEnabled ? shape.bend[2] : 0.0f
    };
    float mirror[3] = {
        (shape.mirrorModifierEnabled && shape.mirrorAxis[0]) ? 1.0f : 0.0f,
        (shape.mirrorModifierEnabled && shape.mirrorAxis[1]) ? 1.0f : 0.0f,
        (shape.mirrorModifierEnabled && shape.mirrorAxis[2]) ? 1.0f : 0.0f
    };
    float mirrorOffset[3] = {
        (shape.mirrorModifierEnabled && shape.mirrorAxis[0]) ? shape.mirrorOffset[0] : 0.0f,
        (shape.mirrorModifierEnabled && shape.mirrorAxis[1]) ? shape.mirrorOffset[1] : 0.0f,
        (shape.mirrorModifierEnabled && shape.mirrorAxis[2]) ? shape.mirrorOffset[2] : 0.0f
    };
    const float mirrorSmoothness = shape.mirrorModifierEnabled ? std::max(shape.mirrorSmoothness, 0.0f) : 0.0f;
    float arrayAxis[3] = {
        (shape.arrayModifierEnabled && shape.arrayAxis[0]) ? 1.0f : 0.0f,
        (shape.arrayModifierEnabled && shape.arrayAxis[1]) ? 1.0f : 0.0f,
        (shape.arrayModifierEnabled && shape.arrayAxis[2]) ? 1.0f : 0.0f
    };
    float arraySpacing[3] = {
        shape.arrayModifierEnabled ? std::max(std::fabs(shape.arraySpacing[0]), 1e-4f) : 0.0f,
        shape.arrayModifierEnabled ? std::max(std::fabs(shape.arraySpacing[1]), 1e-4f) : 0.0f,
        shape.arrayModifierEnabled ? std::max(std::fabs(shape.arraySpacing[2]), 1e-4f) : 0.0f
    };
    float arrayRepeatCount[3] = {
        (shape.arrayModifierEnabled && shape.arrayAxis[0]) ? static_cast<float>(std::max(shape.arrayRepeatCount[0], 1)) : 1.0f,
        (shape.arrayModifierEnabled && shape.arrayAxis[1]) ? static_cast<float>(std::max(shape.arrayRepeatCount[1], 1)) : 1.0f,
        (shape.arrayModifierEnabled && shape.arrayAxis[2]) ? static_cast<float>(std::max(shape.arrayRepeatCount[2], 1)) : 1.0f
    };
    const float arraySmoothness = shape.arrayModifierEnabled ? std::max(shape.arraySmoothness, 0.0f) : 0.0f;
    float modifierStack[3] = {
        static_cast<float>(shape.modifierStack[0]),
        static_cast<float>(shape.modifierStack[1]),
        static_cast<float>(shape.modifierStack[2])
    };

    hashRaw(hash, twist, sizeof(twist));
    hashRaw(hash, bend, sizeof(bend));
    hashRaw(hash, mirror, sizeof(mirror));
    hashRaw(hash, mirrorOffset, sizeof(mirrorOffset));
    hashValue(hash, mirrorSmoothness);
    hashRaw(hash, arrayAxis, sizeof(arrayAxis));
    hashRaw(hash, arraySpacing, sizeof(arraySpacing));
    hashRaw(hash, arrayRepeatCount, sizeof(arrayRepeatCount));
    hashValue(hash, arraySmoothness);
    hashRaw(hash, modifierStack, sizeof(modifierStack));

    hashValue(hash, shape.blendOp);
    hashValue(hash, shape.smoothness);
    hashRaw(hash, shape.albedo, sizeof(shape.albedo));
    hashValue(hash, shape.metallic);
    hashValue(hash, shape.roughness);
    hashString(hash, shape.albedoTexturePath);
    hashString(hash, shape.roughnessTexturePath);
    hashString(hash, shape.metallicTexturePath);
    hashString(hash, shape.normalTexturePath);
    hashString(hash, shape.displacementTexturePath);
    hashValue(hash, shape.displacementStrength);
    hashRaw(hash, shape.emission, sizeof(shape.emission));
    hashValue(hash, shape.emissionStrength);
    hashValue(hash, shape.transmission);
    hashValue(hash, shape.ior);
    hashValue(hash, shape.dispersion);

    const int curveNodeCount = static_cast<int>(shape.curveNodes.size());
    hashValue(hash, curveNodeCount);
    for (const CurveNode& node : shape.curveNodes) {
        hashRaw(hash, node.position, sizeof(node.position));
        hashValue(hash, node.radius);
    }
}

std::uint64_t Renderer::computeShapeBufferHash(const std::vector<Shape>& shapes) const {
    std::uint64_t hash = 1469598103934665603ULL;

    const int shapeCount = static_cast<int>(shapes.size());
    hashValue(hash, shapeCount);
    for (int i = 0; i < shapeCount; ++i) {
        hashShapeMaterialState(hash, shapes[static_cast<std::size_t>(i)]);
    }

    return hash;
}

std::uint64_t Renderer::computeSceneHash(const std::vector<Shape>& shapes,
                                         const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                                         const float backgroundColor[3],
                                         float ambientIntensity, float directLightIntensity,
                                         const float cameraPos[3], const float cameraTarget[3],
                                         int display_w, int display_h,
                                         bool useGradientBackground,
                                         const RenderSettings& renderSettings,
                                         const TransformationState& transformState) const {
    std::uint64_t hash = 1469598103934665603ULL;
    const float cameraFovDegrees = safeCameraFovDegrees(renderSettings);
    const int cameraProjectionMode = safeCameraProjectionMode(renderSettings);
    const int pathTracerMaxBounces = safePathTracerMaxBounces(renderSettings);
    const bool guidedSamplingEnabled = renderSettings.pathTracerGuidedSamplingEnabled;
    const float guidedSamplingMix = safePathTracerGuidedSamplingMix(renderSettings);
    const float misPower = safePathTracerMisPower(renderSettings);
    const int russianRouletteStartBounce = safePathTracerRussianRouletteStartBounce(renderSettings);
    const float pathTracerResolutionScale = safePathTracerResolutionScale(renderSettings);
    const bool physicalCameraEnabled = usePathTracerPhysicalCamera(renderSettings);
    const float physicalCameraFocalLengthMm = physicalCameraEnabled ? safePathTracerCameraFocalLengthMm(renderSettings) : 0.0f;
    const float physicalCameraSensorWidthMm = physicalCameraEnabled ? safePathTracerCameraSensorWidthMm(renderSettings) : 0.0f;
    const float physicalCameraSensorHeightMm = physicalCameraEnabled ? safePathTracerCameraSensorHeightMm(renderSettings) : 0.0f;
    const float physicalCameraApertureFNumber = physicalCameraEnabled ? safePathTracerCameraApertureFNumber(renderSettings) : 0.0f;
    const float physicalCameraFocusDistance = physicalCameraEnabled ? safePathTracerCameraFocusDistance(renderSettings) : 0.0f;
    const int physicalCameraBladeCount = physicalCameraEnabled ? safePathTracerCameraBladeCount(renderSettings) : 0;
    const float physicalCameraBladeRotationRadians =
        physicalCameraEnabled ? safePathTracerCameraBladeRotationRadians(renderSettings) : 0.0f;
    const float physicalCameraAnamorphicRatio = physicalCameraEnabled ? safePathTracerCameraAnamorphicRatio(renderSettings) : 0.0f;
    const bool denoiserEnabled = renderSettings.denoiserEnabled;
    const int denoiseStartSample = safeDenoiseStartSample(renderSettings);
    const int denoiseInterval = safeDenoiseInterval(renderSettings);
    const float safeAmbientIntensity = std::max(ambientIntensity, 0.0f);
    const float safeDirectLightIntensity = std::max(directLightIntensity, 0.0f);

    const int shapeCount = static_cast<int>(shapes.size());
    hashValue(hash, shapeCount);
    hashRaw(hash, lightDir, sizeof(float) * 3u);
    hashRaw(hash, lightColor, sizeof(float) * 3u);
    hashRaw(hash, ambientColor, sizeof(float) * 3u);
    hashRaw(hash, backgroundColor, sizeof(float) * 3u);
    hashValue(hash, safeAmbientIntensity);
    hashValue(hash, safeDirectLightIntensity);
    hashRaw(hash, cameraPos, sizeof(float) * 3u);
    hashRaw(hash, cameraTarget, sizeof(float) * 3u);
    hashValue(hash, display_w);
    hashValue(hash, display_h);
    hashValue(hash, useGradientBackground);
    hashValue(hash, cameraFovDegrees);
    hashValue(hash, cameraProjectionMode);
    hashValue(hash, pathTracerMaxBounces);
    hashValue(hash, guidedSamplingEnabled);
    hashValue(hash, guidedSamplingMix);
    hashValue(hash, misPower);
    hashValue(hash, russianRouletteStartBounce);
    hashValue(hash, pathTracerResolutionScale);
    hashValue(hash, physicalCameraEnabled);
    hashValue(hash, physicalCameraFocalLengthMm);
    hashValue(hash, physicalCameraSensorWidthMm);
    hashValue(hash, physicalCameraSensorHeightMm);
    hashValue(hash, physicalCameraApertureFNumber);
    hashValue(hash, physicalCameraFocusDistance);
    hashValue(hash, physicalCameraBladeCount);
    hashValue(hash, physicalCameraBladeRotationRadians);
    hashValue(hash, physicalCameraAnamorphicRatio);
    hashValue(hash, denoiserEnabled);
    hashValue(hash, denoiseStartSample);
    hashValue(hash, denoiseInterval);
    const int pointLightCount = std::min(static_cast<int>(transformState.pointLights.size()), kMaxPointLights);
    hashValue(hash, pointLightCount);
    for (int i = 0; i < pointLightCount; ++i) {
        const PointLightState& pointLight = transformState.pointLights[static_cast<std::size_t>(i)];
        hashRaw(hash, pointLight.position, sizeof(float) * 3u);
        hashRaw(hash, pointLight.color, sizeof(float) * 3u);
        hashValue(hash, std::max(pointLight.intensity, 0.0f));
        hashValue(hash, std::max(pointLight.range, 0.01f));
        hashValue(hash, std::max(pointLight.radius, 0.0f));
    }

    for (int i = 0; i < shapeCount; ++i) {
        hashShapeMaterialState(hash, shapes[static_cast<std::size_t>(i)]);
    }

    return hash;
}

} // namespace rmt
