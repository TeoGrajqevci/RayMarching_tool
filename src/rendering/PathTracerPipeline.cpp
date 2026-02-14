#include "rmt/rendering/PathTracerPipeline.h"

#include <algorithm>
#include <chrono>
#include <iostream>

#include "rmt/rendering/MirrorHelperUniforms.h"
#include "rmt/rendering/Renderer.h"
#include "RendererInternal.h"

namespace rmt {

using namespace renderer_internal;

void Renderer::renderPathTracer(const std::vector<Shape>& shapes,
                                const std::vector<int>& selectedShapes,
                                const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                                const float backgroundColor[3],
                                float ambientIntensity, float directLightIntensity,
                                const float cameraPos[3], const float cameraTarget[3],
                                int display_w, int display_h,
                                bool useGradientBackground,
                                const RenderSettings& renderSettings,
                                const TransformationState& transformState) {
    const std::chrono::steady_clock::time_point stageStart = std::chrono::steady_clock::now();
    ensureSceneBuffer(shapes);
    const std::chrono::steady_clock::time_point sceneUploadEnd = std::chrono::steady_clock::now();
    ensureMeshSdfBuffer();
    const std::chrono::steady_clock::time_point meshUploadEnd = std::chrono::steady_clock::now();
    ensurePathTracerResources(display_w, display_h);
    const std::chrono::steady_clock::time_point resourceSetupEnd = std::chrono::steady_clock::now();

    const int pathTracerMaxBounces = safePathTracerMaxBounces(renderSettings);
    const bool denoiserEnabled = renderSettings.denoiserEnabled;
    const int denoiseStartSample = safeDenoiseStartSample(renderSettings);
    const int denoiseInterval = safeDenoiseInterval(renderSettings);
    const float cameraFovDegrees = safeCameraFovDegrees(renderSettings);
    const int cameraProjectionMode = safeCameraProjectionMode(renderSettings);
    const float cameraOrthoScale = computeCameraOrthoScale(cameraPos, cameraTarget, cameraFovDegrees);

    const std::uint64_t currentSceneHash = computeSceneHash(shapes,
                                                            lightDir, lightColor, ambientColor,
                                                            backgroundColor, ambientIntensity, directLightIntensity,
                                                            cameraPos, cameraTarget,
                                                            display_w, display_h,
                                                            useGradientBackground,
                                                            renderSettings,
                                                            transformState);
    if (!accumulationInitialized || currentSceneHash != lastSceneHash) {
        resetPathTracerAccumulation();
        lastSceneHash = currentSceneHash;
    }

    Shader& shader = *pathTracerShader;
    bool hasActiveMirrorModifier = false;
    for (const Shape& shape : shapes) {
        if (shape.mirrorModifierEnabled &&
            (shape.mirrorAxis[0] || shape.mirrorAxis[1] || shape.mirrorAxis[2])) {
            hasActiveMirrorModifier = true;
            break;
        }
    }

    GLint previousViewport[4] = {0, 0, 1, 1};
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    const int readIndex = accumulationWriteIndex;
    const int writeIndex = 1 - accumulationWriteIndex;

    glBindFramebuffer(GL_FRAMEBUFFER, accumulationFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumulationTextures[writeIndex], 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Path tracer framebuffer is incomplete." << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
        return;
    }
    glViewport(0, 0, display_w, display_h);

    shader.use();
    shader.setInt("uSampleCount", pathSampleCount);
    shader.setInt("uFrameIndex", pathSampleCount + 1);
    shader.setInt("uPrevAccum", 0);
    shader.setInt("uMaxBounces", pathTracerMaxBounces);
    shader.setInt("uUseFixedSeed", renderSettings.pathTracerUseFixedSeed ? 1 : 0);
    shader.setInt("uFixedSeed", std::max(renderSettings.pathTracerFixedSeed, 0));
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

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, accumulationTextures[readIndex]);
    bindSceneBuffer(shader, 1);
    if (hasActiveMirrorModifier) {
        shader.setInt("uAccelGridEnabled", 0);
    }
    const std::chrono::steady_clock::time_point setupEnd = std::chrono::steady_clock::now();

    const GLuint pathTimerQuery = pathGpuTimer.queries[pathGpuTimer.writeIndex];
    glBeginQuery(GL_TIME_ELAPSED, pathTimerQuery);
    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glEndQuery(GL_TIME_ELAPSED);
    const std::chrono::steady_clock::time_point drawEnd = std::chrono::steady_clock::now();

    pathGpuTimer.writeIndex = (pathGpuTimer.writeIndex + 1) % kTimerQueryCount;
    pathGpuTimer.initializedCount = std::min(pathGpuTimer.initializedCount + 1, kTimerQueryCount);

    cpuSceneUploadMs = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(sceneUploadEnd - stageStart).count();
    cpuMeshUploadMs = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(meshUploadEnd - sceneUploadEnd).count();
    cpuSetupMs = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(setupEnd - resourceSetupEnd).count();
    cpuDrawSubmitMs = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(drawEnd - setupEnd).count();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);

    accumulationWriteIndex = writeIndex;
    ++pathSampleCount;

    GLuint displayTexture = accumulationTextures[accumulationWriteIndex];
    if (!denoiserEnabled) {
        hasDenoisedFrame = false;
    }

#ifdef RMT_HAS_OIDN
    if (denoiserEnabled &&
        denoiserAvailable &&
        pathSampleCount >= denoiseStartSample &&
        ((pathSampleCount - denoiseStartSample) % denoiseInterval == 0) &&
        denoiseReadbackPBOs[0] != 0 && denoiseReadbackPBOs[1] != 0) {

        const std::size_t byteSize =
            static_cast<std::size_t>(accumulationWidth) * static_cast<std::size_t>(accumulationHeight) * 3u * sizeof(float);

        const int submitIndex = denoiseReadbackIndex;
        const int processIndex = 1 - denoiseReadbackIndex;

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glBindTexture(GL_TEXTURE_2D, accumulationTextures[accumulationWriteIndex]);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, denoiseReadbackPBOs[submitIndex]);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, nullptr);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        denoiseReadbackReady[submitIndex] = true;

        if (denoiseReadbackReady[processIndex]) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, denoiseReadbackPBOs[processIndex]);
            void* mappedData = glMapBufferRange(GL_PIXEL_PACK_BUFFER,
                                                0,
                                                static_cast<GLsizeiptr>(byteSize),
                                                GL_MAP_READ_BIT);
            if (mappedData != nullptr) {
                try {
                    if (!oidnColorBuffer || !oidnOutputBuffer || oidnBufferByteSize != byteSize) {
                        const oidn::Storage storageMode = denoiserUsingGPU ? oidn::Storage::Device : oidn::Storage::Host;
                        oidnColorBuffer = oidnDevice.newBuffer(byteSize, storageMode);
                        oidnOutputBuffer = oidnDevice.newBuffer(byteSize, storageMode);
                        oidnBufferByteSize = byteSize;
                    }

                    oidnColorBuffer.write(0, byteSize, mappedData);

                    oidnFilter.setImage("color", oidnColorBuffer, oidn::Format::Float3,
                                        static_cast<std::size_t>(accumulationWidth),
                                        static_cast<std::size_t>(accumulationHeight));
                    oidnFilter.setImage("output", oidnOutputBuffer, oidn::Format::Float3,
                                        static_cast<std::size_t>(accumulationWidth),
                                        static_cast<std::size_t>(accumulationHeight));
                    oidnFilter.set("hdr", true);
                    oidnFilter.commit();
                    oidnFilter.execute();
                    oidnOutputBuffer.read(0, byteSize, denoiseOutput.data());

                    const char* errorMessage = nullptr;
                    if (oidnDevice.getError(errorMessage) == oidn::Error::None) {
                        const GLuint denoiseTimerQuery = denoiseGpuTimer.queries[denoiseGpuTimer.writeIndex];
                        glBeginQuery(GL_TIME_ELAPSED, denoiseTimerQuery);
                        glBindTexture(GL_TEXTURE_2D, denoisedTexture);
                        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                                        accumulationWidth, accumulationHeight,
                                        GL_RGB, GL_FLOAT,
                                        denoiseOutput.data());
                        glEndQuery(GL_TIME_ELAPSED);

                        denoiseGpuTimer.writeIndex = (denoiseGpuTimer.writeIndex + 1) % kTimerQueryCount;
                        denoiseGpuTimer.initializedCount = std::min(denoiseGpuTimer.initializedCount + 1, kTimerQueryCount);

                        hasDenoisedFrame = true;
                    } else {
                        std::cerr << "OIDN execution error: "
                                  << (errorMessage != nullptr ? errorMessage : "unknown error")
                                  << std::endl;
                        denoiserAvailable = false;
                        hasDenoisedFrame = false;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "OIDN execution exception: " << e.what() << std::endl;
                    denoiserAvailable = false;
                    hasDenoisedFrame = false;
                }

                glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            }

            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            denoiseReadbackReady[processIndex] = false;
        }

        denoiseReadbackIndex = processIndex;
    }
#endif

    if (hasDenoisedFrame) {
        displayTexture = denoisedTexture;
    }

    const MirrorHelperUniformState mirrorState =
        buildMirrorHelperUniformState(shapes, selectedShapes, transformState);

    Shader& display = *pathTracerDisplayShader;
    display.use();
    display.setInt("uTexture", 0);
    display.setVec2("uResolution", static_cast<float>(display_w), static_cast<float>(display_h));
    display.setVec3("uCameraPos", cameraPos[0], cameraPos[1], cameraPos[2]);
    display.setVec3("uCameraTarget", cameraTarget[0], cameraTarget[1], cameraTarget[2]);
    display.setFloat("uCameraFovDegrees", cameraFovDegrees);
    display.setInt("uCameraProjectionMode", cameraProjectionMode);
    display.setFloat("uCameraOrthoScale", cameraOrthoScale);
    uploadMirrorHelperUniforms(display, mirrorState);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, displayTexture);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

} // namespace rmt
