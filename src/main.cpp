#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <array>
#include <sstream>
#include <cstddef>

#include "Shader.h"
#include "Texture.h"
#include "ImGuiLayer.h"
#include "Shapes.h"
#include "Input.h"         
#include "Utilities.h"
#include "GuiManager.h"    

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Renderer.h"

float camDistance = 3.0f;
Texture hdrTexture;
bool hdrLoaded = false;


void updateCameraPosition(float camDistance, float camTheta, float camPhi, const float cameraTarget[3], float cameraPos[3])
{
    sphericalToCartesian(camDistance, camTheta, camPhi, cameraTarget, cameraPos);
}


int main()
{
    // --- 1. Initialize GLFW and GLAD ---
    if (!glfwInit())
    {
         std::cerr << "Failed to initialize GLFW\n";
         return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1200, 900, "", NULL, NULL);
    if (!window)
    {
         std::cerr << "Failed to create GLFW window\n";
         glfwTerminate();
         return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetDropCallback(window, drop_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
         std::cerr << "Failed to initialize GLAD\n";
         return -1;
    }

    // --- 2. Initialize ImGui ---
    ImGuiLayer::Init(window);

    // --- 3. Compile shader ---
    Shader shader("shaders/vertex.glsl", "shaders/Solid_renderer.glsl");
    // Shader shader("shaders/vertex.glsl", "shaders/Pathtracer.glsl");

    // --- 4. Set up full-screen quad ---
    float quadVertices[] = {
         -1.0f, -1.0f,
          1.0f, -1.0f,
          1.0f,  1.0f,
         -1.0f,  1.0f
    };
    unsigned int quadIndices[] = {
         0, 1, 2,
         2, 3, 0
    };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
      glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- 5. Lighting parameters ---
    float lightDir[3] = { 0.5f, 0.8f, 1.0f };
    float lightColor[3] = { 1.0f, 1.0f, 1.0f };
    float ambientColor[3] = { 0.17f, 0.17f, 0.17f };

    // --- 6. Camera orbit parameters ---
    float camTheta = 0.0f;
    float camPhi = asinf(1.0f / 3.0f);
    float cameraTarget[3] = { 0.0f, 0.0f, 0.0f };
    float cameraPos[3] = { 0.0f, 0.0f, 3.0f };

    static bool cameraDragging = false;
    static double lastMouseX = 0, lastMouseY = 0;
    bool mouseWasPressed = false;

    // --- 7. Initialize shapes and state variables ---
    std::vector<Shape> shapes;
    Shape initialSphere;
    initialSphere.type = SHAPE_SPHERE;
    initialSphere.center[0] = initialSphere.center[1] = initialSphere.center[2] = 0.0f;
    initialSphere.param[0] = 0.5f;
    initialSphere.param[1] = initialSphere.param[2] = 0.0f;
    initialSphere.rotation[0] = initialSphere.rotation[1] = initialSphere.rotation[2] = 0.0f;
    initialSphere.extra = 0.0f;
    initialSphere.name = "0";
    initialSphere.blendOp = BLEND_NONE;
    initialSphere.smoothness = 0.1f;
    initialSphere.albedo[0] = 1.0f; initialSphere.albedo[1] = 1.0f; initialSphere.albedo[2] = 1.0f;
    initialSphere.metallic = 0.0f;
    initialSphere.roughness = 0.05f;
    shapes.push_back(initialSphere);

    std::vector<int> selectedShapes;
    bool useGradientBackground = false;
    static bool altRenderMode = false;
    static int editingShapeIndex = -1;
    static char renameBuffer[128] = "";
    bool showGUI = true;
    bool useHdrBackground = true;
    bool useHdrLighting = true;
    char hdrFilePath[512] = "";
    float hdrIntensity = 1.0f;
    bool showHelpPopup = false;
    ImVec2 helpButtonPos;

    InputManager inputManager;
    GUIManager guiManager;
    TransformationState transformState;

    // --- 8. Main render loop ---
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        inputManager.processGeneralInput(window, shapes, selectedShapes, cameraTarget, showGUI, altRenderMode);
        inputManager.processTransformationModeActivation(window, shapes, selectedShapes, transformState);
        inputManager.processTransformationUpdates(window, shapes, selectedShapes, transformState, cameraPos, cameraTarget);
        inputManager.processMousePickingAndCameraDrag(window, shapes, selectedShapes, cameraPos, cameraTarget,
                                                      camTheta, camPhi, cameraDragging, lastMouseX, lastMouseY, mouseWasPressed);

        updateCameraPosition(camDistance, camTheta, camPhi, cameraTarget, cameraPos);

        if (showGUI)
        {
            guiManager.newFrame();
            guiManager.renderGUI(window, shapes, selectedShapes, lightDir, lightColor, ambientColor,
                                 useGradientBackground, hdrFilePath, hdrTexture, hdrLoaded,
                                 hdrIntensity, useHdrBackground, useHdrLighting,
                                 showGUI, altRenderMode, editingShapeIndex, renameBuffer,
                                 showHelpPopup, helpButtonPos);
        }

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        Renderer renderer(shader, VAO, hdrTexture);
        renderer.renderScene(shapes, selectedShapes, lightDir, lightColor, ambientColor,
                             cameraPos, cameraTarget, display_w, display_h,
                             hdrIntensity, useHdrBackground, useHdrLighting, hdrLoaded, altRenderMode, useGradientBackground);

        if (showGUI)
            ImGuiLayer::Render();

        glfwSwapBuffers(window);
    }

    ImGuiLayer::Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
