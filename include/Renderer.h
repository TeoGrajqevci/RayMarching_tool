#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <vector>
#include <array>
#include "Shader.h"
#include "Texture.h"
#include "Shapes.h"

class Renderer {
public:
    Renderer(Shader& shader, GLuint vao, Texture& hdrTexture);
    void renderScene(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes,
                     const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                     const float cameraPos[3], const float cameraTarget[3],
                     int display_w, int display_h,
                     float hdrIntensity, bool useHdrBackground, bool useHdrLighting,
                     bool hdrLoaded, bool altRenderMode, bool useGradientBackground);
private:
    Shader& shader;
    GLuint quadVAO;
    Texture& hdrTexture;
};

#endif // RENDERER_H
