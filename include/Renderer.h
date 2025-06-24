#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <vector>
#include <array>
#include "Shader.h"
#include "Shapes.h"

class Renderer {
public:
    Renderer(Shader& shader, GLuint vao);
    void renderScene(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes,
                     const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                     const float cameraPos[3], const float cameraTarget[3],
                     int display_w, int display_h,
                     bool altRenderMode, bool useGradientBackground);
private:
    Shader& shader;
    GLuint quadVAO;
};

#endif // RENDERER_H
