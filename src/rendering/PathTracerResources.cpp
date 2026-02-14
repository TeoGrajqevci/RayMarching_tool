#include "rmt/rendering/PathTracerResources.h"

#include <algorithm>
#include <iostream>

#include "rmt/rendering/Renderer.h"

namespace rmt {

void Renderer::ensurePathTracerResources(int display_w, int display_h) {
    display_w = std::max(display_w, 1);
    display_h = std::max(display_h, 1);

    if (display_w == accumulationWidth && display_h == accumulationHeight &&
        accumulationTextures[0] != 0 && accumulationTextures[1] != 0 && denoisedTexture != 0) {
        return;
    }

    accumulationWidth = display_w;
    accumulationHeight = display_h;

    if (accumulationTextures[0] == 0 || accumulationTextures[1] == 0) {
        glGenTextures(2, accumulationTextures);
    }
    if (denoisedTexture == 0) {
        glGenTextures(1, &denoisedTexture);
    }

    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, accumulationTextures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, accumulationWidth, accumulationHeight, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    glBindTexture(GL_TEXTURE_2D, denoisedTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, accumulationWidth, accumulationHeight, 0, GL_RGB, GL_FLOAT, nullptr);

    const std::size_t rgbFloatCount = static_cast<std::size_t>(accumulationWidth) *
                                      static_cast<std::size_t>(accumulationHeight) * 3u;
    denoiseOutput.assign(rgbFloatCount, 0.0f);

    const std::size_t readbackByteSize = rgbFloatCount * sizeof(float);
    if (denoiseReadbackPBOs[0] == 0 || denoiseReadbackPBOs[1] == 0) {
        glGenBuffers(2, denoiseReadbackPBOs);
    }

    for (int i = 0; i < 2; ++i) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, denoiseReadbackPBOs[i]);
        glBufferData(GL_PIXEL_PACK_BUFFER, static_cast<GLsizeiptr>(readbackByteSize), nullptr, GL_STREAM_READ);
        denoiseReadbackReady[i] = false;
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    denoiseReadbackIndex = 0;

#ifdef RMT_HAS_OIDN
    if (denoiserAvailable) {
        const oidn::Storage storageMode = denoiserUsingGPU ? oidn::Storage::Device : oidn::Storage::Host;

        oidnColorBuffer = oidnDevice.newBuffer(readbackByteSize, storageMode);
        oidnOutputBuffer = oidnDevice.newBuffer(readbackByteSize, storageMode);
        oidnBufferByteSize = readbackByteSize;

        const char* deviceError = nullptr;
        if (!oidnColorBuffer || !oidnOutputBuffer ||
            oidnDevice.getError(deviceError) != oidn::Error::None) {
            std::cerr << "OIDN buffer allocation failed: "
                      << (deviceError != nullptr ? deviceError : "unknown error")
                      << std::endl;
            denoiserAvailable = false;
            denoiserUsingGPU = false;
            oidnColorBuffer.release();
            oidnOutputBuffer.release();
            oidnBufferByteSize = 0;
        }
    }
#endif

    resetPathTracerAccumulation();
}

void Renderer::resetPathTracerAccumulation() {
    if (accumulationWidth <= 0 || accumulationHeight <= 0 ||
        accumulationTextures[0] == 0 || accumulationTextures[1] == 0 || denoisedTexture == 0) {
        return;
    }

    GLint previousFBO = 0;
    GLint previousViewport[4] = {0, 0, 1, 1};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    glBindFramebuffer(GL_FRAMEBUFFER, accumulationFBO);
    glViewport(0, 0, accumulationWidth, accumulationHeight);

    for (int i = 0; i < 2; ++i) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumulationTextures[i], 0);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, denoisedTexture, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFBO));
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);

    accumulationWriteIndex = 0;
    pathSampleCount = 0;
    accumulationInitialized = true;
    hasDenoisedFrame = false;
    denoiseReadbackReady[0] = false;
    denoiseReadbackReady[1] = false;
    denoiseReadbackIndex = 0;
}

} // namespace rmt
