#include "rmt/rendering/RendererState.h"

#include <algorithm>
#include <chrono>

#include "rmt/rendering/MirrorHelperUniforms.h"
#include "rmt/rendering/Renderer.h"
#include "RendererInternal.h"

namespace rmt {

using namespace renderer_internal;

void Renderer::renderSolid(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes,
                           const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                           const float backgroundColor[3],
                           float ambientIntensity, float directLightIntensity,
                           const float cameraPos[3], const float cameraTarget[3],
                           int display_w, int display_h,
                           bool altRenderMode, bool useGradientBackground,
                           const RenderSettings& renderSettings,
                           const TransformationState& transformState) {
    const std::chrono::steady_clock::time_point stageStart = std::chrono::steady_clock::now();
    ensureSceneBuffer(shapes);
    const std::chrono::steady_clock::time_point sceneUploadEnd = std::chrono::steady_clock::now();
    ensureMeshSdfBuffer();
    const std::chrono::steady_clock::time_point meshUploadEnd = std::chrono::steady_clock::now();

    Shader& shader = *solidShader;
    shader.use();
    shader.setInt("uRenderMode", altRenderMode ? 1 : 0);
    shader.setInt("uRayMarchQuality", clampi(renderSettings.rayMarchQuality, RAYMARCH_QUALITY_ULTRA, RAYMARCH_QUALITY_LOW));
    shader.setInt("uDebugOutputMode", renderSettings.debugOutputMode);
    shader.setInt("uOptimizedMarchEnabled", renderSettings.optimizedTracingEnabled ? 1 : 0);

    uploadCommonUniforms(shader,
                         lightDir, lightColor, ambientColor,
                         backgroundColor, ambientIntensity, directLightIntensity,
                         cameraPos, cameraTarget,
                         display_w, display_h,
                         useGradientBackground,
                         renderSettings,
                         transformState);

    bindSceneBuffer(shader, 0);

    const int maxSelected = std::min(static_cast<int>(selectedShapes.size()), kMaxSelected);
    shader.setInt("uSelectedCount", maxSelected);

    int selectedIndices[kMaxSelected];
    std::fill(selectedIndices, selectedIndices + kMaxSelected, -1);
    for (int i = 0; i < maxSelected; ++i) {
        selectedIndices[i] = selectedShapes[static_cast<std::size_t>(i)];
    }
    shader.setIntArray("uSelectedIndices", selectedIndices, kMaxSelected);

    shader.setFloat("uOutlineThickness", 0.015f);
    shader.setInt("uShowAxisGuides", transformState.showAxisGuides ? 1 : 0);
    shader.setInt("uActiveAxis", transformState.activeAxis);
    shader.setVec3("uGuideCenter",
                   transformState.guideCenter[0],
                   transformState.guideCenter[1],
                   transformState.guideCenter[2]);
    shader.setVec3("uGuideAxisDirection",
                   transformState.guideAxisDirection[0],
                   transformState.guideAxisDirection[1],
                   transformState.guideAxisDirection[2]);

    const MirrorHelperUniformState mirrorState =
        buildMirrorHelperUniformState(shapes, selectedShapes, transformState);
    uploadMirrorHelperUniforms(shader, mirrorState);
    const std::chrono::steady_clock::time_point setupEnd = std::chrono::steady_clock::now();

    const GLuint timerQuery = solidGpuTimer.queries[solidGpuTimer.writeIndex];
    glBeginQuery(GL_TIME_ELAPSED, timerQuery);
    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glEndQuery(GL_TIME_ELAPSED);
    const std::chrono::steady_clock::time_point drawEnd = std::chrono::steady_clock::now();

    solidGpuTimer.writeIndex = (solidGpuTimer.writeIndex + 1) % kTimerQueryCount;
    solidGpuTimer.initializedCount = std::min(solidGpuTimer.initializedCount + 1, kTimerQueryCount);

    cpuSceneUploadMs = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(sceneUploadEnd - stageStart).count();
    cpuMeshUploadMs = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(meshUploadEnd - sceneUploadEnd).count();
    cpuSetupMs = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(setupEnd - meshUploadEnd).count();
    cpuDrawSubmitMs = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(drawEnd - setupEnd).count();
}

} // namespace rmt
