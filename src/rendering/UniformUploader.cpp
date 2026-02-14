#include "rmt/rendering/UniformUploader.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdio>

#include "rmt/rendering/Renderer.h"
#include "RendererInternal.h"

namespace rmt {

using namespace renderer_internal;

namespace {

constexpr int kMaxPointLights = 16;

} // namespace

void Renderer::uploadCommonUniforms(const Shader& activeShader,
                                    const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                                    const float backgroundColor[3],
                                    float ambientIntensity, float directLightIntensity,
                                    const float cameraPos[3], const float cameraTarget[3],
                                    int display_w, int display_h,
                                    bool useGradientBackground,
                                    const RenderSettings& renderSettings,
                                    const TransformationState& transformState) const {
    const float cameraFovDegrees = safeCameraFovDegrees(renderSettings);
    const int cameraProjectionMode = safeCameraProjectionMode(renderSettings);
    const float cameraOrthoScale = computeCameraOrthoScale(cameraPos, cameraTarget, cameraFovDegrees);

    activeShader.setVec2("iResolution", static_cast<float>(display_w), static_cast<float>(display_h));
    activeShader.setFloat("iTime", static_cast<float>(glfwGetTime()));
    activeShader.setVec3("lightDir", lightDir[0], lightDir[1], lightDir[2]);
    activeShader.setVec3("lightColor", lightColor[0], lightColor[1], lightColor[2]);
    activeShader.setVec3("ambientColor", ambientColor[0], ambientColor[1], ambientColor[2]);
    activeShader.setVec3("backgroundColor", backgroundColor[0], backgroundColor[1], backgroundColor[2]);
    activeShader.setFloat("ambientIntensity", std::max(ambientIntensity, 0.0f));
    activeShader.setFloat("directLightIntensity", std::max(directLightIntensity, 0.0f));
    activeShader.setVec3("cameraPos", cameraPos[0], cameraPos[1], cameraPos[2]);
    activeShader.setVec3("cameraTarget", cameraTarget[0], cameraTarget[1], cameraTarget[2]);
    activeShader.setFloat("uCameraFovDegrees", cameraFovDegrees);
    activeShader.setInt("uCameraProjectionMode", cameraProjectionMode);
    activeShader.setFloat("uCameraOrthoScale", cameraOrthoScale);
    activeShader.setInt("uBackgroundGradient", useGradientBackground ? 1 : 0);

    const int pointLightCount = std::min(static_cast<int>(transformState.pointLights.size()), kMaxPointLights);
    activeShader.setInt("uPointLightCount", pointLightCount);
    for (int i = 0; i < pointLightCount; ++i) {
        const PointLightState& pointLight = transformState.pointLights[static_cast<std::size_t>(i)];

        char name[64];
        std::snprintf(name, sizeof(name), "uPointLightPos[%d]", i);
        activeShader.setVec3(name, pointLight.position[0], pointLight.position[1], pointLight.position[2]);

        std::snprintf(name, sizeof(name), "uPointLightColor[%d]", i);
        activeShader.setVec3(name, pointLight.color[0], pointLight.color[1], pointLight.color[2]);

        std::snprintf(name, sizeof(name), "uPointLightIntensity[%d]", i);
        activeShader.setFloat(name, std::max(pointLight.intensity, 0.0f));

        std::snprintf(name, sizeof(name), "uPointLightRange[%d]", i);
        activeShader.setFloat(name, std::max(pointLight.range, 0.01f));

        std::snprintf(name, sizeof(name), "uPointLightRadius[%d]", i);
        activeShader.setFloat(name, std::max(pointLight.radius, 0.0f));
    }
}

} // namespace rmt
