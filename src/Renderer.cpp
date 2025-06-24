#include "Renderer.h"
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <GLFW/glfw3.h> // For glfwGetTime()

Renderer::Renderer(Shader& shader, GLuint vao)
    : shader(shader), quadVAO(vao)
{
}

void Renderer::renderScene(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes,
                     const float lightDir[3], const float lightColor[3], const float ambientColor[3],
                     const float cameraPos[3], const float cameraTarget[3],
                     int display_w, int display_h,
                     bool altRenderMode, bool useGradientBackground,
                     const TransformationState& transformState)
{
    shader.use();
    shader.setInt("uRenderMode", altRenderMode ? 1 : 0);
    shader.setVec2("iResolution", (float)display_w, (float)display_h);
    shader.setFloat("iTime", (float)glfwGetTime());
    shader.setVec3("lightDir", lightDir[0], lightDir[1], lightDir[2]);
    shader.setVec3("lightColor", lightColor[0], lightColor[1], lightColor[2]);
    shader.setVec3("ambientColor", ambientColor[0], ambientColor[1], ambientColor[2]);
    shader.setVec3("cameraPos", cameraPos[0], cameraPos[1], cameraPos[2]);
    shader.setVec3("cameraTarget", cameraTarget[0], cameraTarget[1], cameraTarget[2]);
    shader.setInt("uBackgroundGradient", useGradientBackground ? 1 : 0);

    // Prepare shape uniforms
    shader.setInt("shapeCount", static_cast<int>(shapes.size()));
    std::vector<int> types(shapes.size());
    std::vector<float> centers(shapes.size() * 3);
    std::vector<float> params(shapes.size() * 4);
    std::vector<float> rotations(shapes.size() * 3);
    std::vector<float> smoothnessArray(shapes.size());
    std::vector<int> blendOpArray(shapes.size());
    std::vector<float> albedos(shapes.size() * 3);
    std::vector<float> metallicArray(shapes.size());
    std::vector<float> roughnessArray(shapes.size());
    for (size_t i = 0; i < shapes.size(); i++) {
        types[i] = shapes[i].type;
        centers[i * 3 + 0] = shapes[i].center[0];
        centers[i * 3 + 1] = shapes[i].center[1];
        centers[i * 3 + 2] = shapes[i].center[2];
        if (shapes[i].type == SHAPE_SPHERE) {
            params[i * 4 + 0] = 0.0f;
            params[i * 4 + 1] = 0.0f;
            params[i * 4 + 2] = 0.0f;
            params[i * 4 + 3] = shapes[i].param[0];
            rotations[i * 3 + 0] = rotations[i * 3 + 1] = rotations[i * 3 + 2] = 0.0f;
        }
        else if (shapes[i].type == SHAPE_BOX) {
            params[i * 4 + 0] = shapes[i].param[0];
            params[i * 4 + 1] = shapes[i].param[1];
            params[i * 4 + 2] = shapes[i].param[2];
            params[i * 4 + 3] = 0.0f;
            rotations[i * 3 + 0] = shapes[i].rotation[0];
            rotations[i * 3 + 1] = shapes[i].rotation[1];
            rotations[i * 3 + 2] = shapes[i].rotation[2];
        }
        else if (shapes[i].type == SHAPE_ROUNDBOX) {
            params[i * 4 + 0] = shapes[i].param[0];
            params[i * 4 + 1] = shapes[i].param[1];
            params[i * 4 + 2] = shapes[i].param[2];
            params[i * 4 + 3] = shapes[i].extra;
            rotations[i * 3 + 0] = shapes[i].rotation[0];
            rotations[i * 3 + 1] = shapes[i].rotation[1];
            rotations[i * 3 + 2] = shapes[i].rotation[2];
        }
        else if (shapes[i].type == SHAPE_TORUS) {
            params[i * 4 + 0] = shapes[i].param[0];
            params[i * 4 + 1] = shapes[i].param[1];
            params[i * 4 + 2] = 0.0f;
            params[i * 4 + 3] = 0.0f;
            rotations[i * 3 + 0] = shapes[i].rotation[0];
            rotations[i * 3 + 1] = shapes[i].rotation[1];
            rotations[i * 3 + 2] = shapes[i].rotation[2];
        }
        else if (shapes[i].type == SHAPE_CYLINDER) {
            params[i * 4 + 0] = shapes[i].param[0];
            params[i * 4 + 1] = shapes[i].param[1];
            params[i * 4 + 2] = 0.0f;
            params[i * 4 + 3] = 0.0f;
            rotations[i * 3 + 0] = shapes[i].rotation[0];
            rotations[i * 3 + 1] = shapes[i].rotation[1];
            rotations[i * 3 + 2] = shapes[i].rotation[2];
        }
        smoothnessArray[i] = shapes[i].smoothness;
        blendOpArray[i] = shapes[i].blendOp;
        albedos[i * 3 + 0] = shapes[i].albedo[0];
        albedos[i * 3 + 1] = shapes[i].albedo[1];
        albedos[i * 3 + 2] = shapes[i].albedo[2];
        metallicArray[i] = shapes[i].metallic;
        roughnessArray[i] = shapes[i].roughness;
    }
    glUniform1iv(glGetUniformLocation(shader.ID, "shapeTypes"), static_cast<int>(types.size()), types.data());
    glUniform3fv(glGetUniformLocation(shader.ID, "shapeCenters"), static_cast<int>(centers.size()/3), centers.data());
    glUniform4fv(glGetUniformLocation(shader.ID, "shapeParams"), static_cast<int>(params.size()/4), params.data());
    glUniform3fv(glGetUniformLocation(shader.ID, "shapeRotations"), static_cast<int>(rotations.size()/3), rotations.data());
    glUniform1fv(glGetUniformLocation(shader.ID, "shapeSmoothness"), static_cast<int>(smoothnessArray.size()), smoothnessArray.data());
    glUniform1iv(glGetUniformLocation(shader.ID, "shapeBlendOp"), static_cast<int>(blendOpArray.size()), blendOpArray.data());
    glUniform3fv(glGetUniformLocation(shader.ID, "shapeAlbedos"), static_cast<int>(albedos.size()/3), albedos.data());
    glUniform1fv(glGetUniformLocation(shader.ID, "shapeMetallic"), static_cast<int>(metallicArray.size()), metallicArray.data());
    glUniform1fv(glGetUniformLocation(shader.ID, "shapeRoughness"), static_cast<int>(roughnessArray.size()), roughnessArray.data());

    // Selected shapes uniforms
    shader.setInt("uSelectedCount", static_cast<int>(selectedShapes.size()));
    int selTypes[50] = {0};
    float selCenters[150] = {0.0f};
    float selParams[200] = {0.0f};
    float selRotations[150] = {0.0f};
    for (size_t i = 0; i < selectedShapes.size() && i < 50; i++) {
        int idx = selectedShapes[i];
        selTypes[i] = shapes[idx].type;
        selCenters[i * 3 + 0] = shapes[idx].center[0];
        selCenters[i * 3 + 1] = shapes[idx].center[1];
        selCenters[i * 3 + 2] = shapes[idx].center[2];
        if (shapes[idx].type == SHAPE_SPHERE) {
            selParams[i * 4 + 0] = 0.0f;
            selParams[i * 4 + 1] = 0.0f;
            selParams[i * 4 + 2] = 0.0f;
            selParams[i * 4 + 3] = shapes[idx].param[0];
            selRotations[i * 3 + 0] = selRotations[i * 3 + 1] = selRotations[i * 3 + 2] = 0.0f;
        }
        else if (shapes[idx].type == SHAPE_BOX) {
            selParams[i * 4 + 0] = shapes[idx].param[0];
            selParams[i * 4 + 1] = shapes[idx].param[1];
            selParams[i * 4 + 2] = shapes[idx].param[2];
            selParams[i * 4 + 3] = 0.0f;
            selRotations[i * 3 + 0] = shapes[idx].rotation[0];
            selRotations[i * 3 + 1] = shapes[idx].rotation[1];
            selRotations[i * 3 + 2] = shapes[idx].rotation[2];
        }
        else if (shapes[idx].type == SHAPE_ROUNDBOX) {
            selParams[i * 4 + 0] = shapes[idx].param[0];
            selParams[i * 4 + 1] = shapes[idx].param[1];
            selParams[i * 4 + 2] = shapes[idx].param[2];
            selParams[i * 4 + 3] = shapes[idx].extra;
            selRotations[i * 3 + 0] = shapes[idx].rotation[0];
            selRotations[i * 3 + 1] = shapes[idx].rotation[1];
            selRotations[i * 3 + 2] = shapes[idx].rotation[2];
        }
        else if (shapes[idx].type == SHAPE_TORUS) {
            selParams[i * 4 + 0] = shapes[idx].param[0];
            selParams[i * 4 + 1] = shapes[idx].param[1];
            selParams[i * 4 + 2] = 0.0f;
            selParams[i * 4 + 3] = 0.0f;
            selRotations[i * 3 + 0] = shapes[idx].rotation[0];
            selRotations[i * 3 + 1] = shapes[idx].rotation[1];
            selRotations[i * 3 + 2] = shapes[idx].rotation[2];
        }
        else if (shapes[idx].type == SHAPE_CYLINDER) {
            selParams[i * 4 + 0] = shapes[idx].param[0];
            selParams[i * 4 + 1] = shapes[idx].param[1];
            selParams[i * 4 + 2] = 0.0f;
            selParams[i * 4 + 3] = 0.0f;
            selRotations[i * 3 + 0] = shapes[idx].rotation[0];
            selRotations[i * 3 + 1] = shapes[idx].rotation[1];
            selRotations[i * 3 + 2] = shapes[idx].rotation[2];
        }
    }
    glUniform1iv(glGetUniformLocation(shader.ID, "uSelectedTypes"), 50, selTypes);
    glUniform3fv(glGetUniformLocation(shader.ID, "uSelectedCenters"), 50, selCenters);
    glUniform4fv(glGetUniformLocation(shader.ID, "uSelectedParams"), 50, selParams);
    glUniform3fv(glGetUniformLocation(shader.ID, "uSelectedRotations"), 50, selRotations);
    shader.setFloat("uOutlineThickness", 0.015f);
    
    // Configuration des guides d'axe
    shader.setInt("uShowAxisGuides", transformState.showAxisGuides ? 1 : 0);
    shader.setInt("uActiveAxis", transformState.activeAxis);
    shader.setVec3("uGuideCenter", transformState.guideCenter[0], transformState.guideCenter[1], transformState.guideCenter[2]);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
