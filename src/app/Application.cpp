#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "rmt/app/Application.h"
#include "rmt/app/EditorRuntime.h"
#include "rmt/app/UndoRedoManager.h"
#include "rmt/app/benchmark/BenchmarkRunner.h"
#include "rmt/app/benchmark/BenchmarkScenes.h"
#include "rmt/app/cli/BenchmarkOptionsParser.h"
#include "rmt/input/Input.h"
#include "rmt/platform/glfw/FileDropQueue.h"
#include "rmt/platform/imgui/ImGuiLayer.h"
#include "rmt/rendering/Renderer.h"
#include "rmt/scene/Shape.h"
#include "rmt/ui/UiFacade.h"

namespace rmt {

float camDistance = 3.0f;

int runApplication(int argc, char** argv) {
    const BenchmarkOptions benchmarkOptions = parseBenchmarkOptions(argc, argv);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    if (benchmarkOptions.runSuite && benchmarkOptions.hiddenWindow) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }

    GLFWwindow* window = glfwCreateWindow(1600, 1000, "", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetDropCallback(window, rmt::fileDropCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    if (!benchmarkOptions.runSuite) {
        ImGuiLayer::Init(window);
    }

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

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    Renderer renderer(vao);

    float lightDir[3] = { 0.5f, 0.8f, 1.0f };
    float lightColor[3] = { 1.0f, 1.0f, 1.0f };
    float ambientColor[3] = { 57.0f / 255.0f, 57.0f / 255.0f, 57.0f / 255.0f };
    float backgroundColor[3] = { 0.0f, 0.0f, 0.0f };
    float ambientIntensity = 0.5f;
    float directLightIntensity = 3.6f;

    float camTheta = 0.0f;
    float camPhi = asinf(1.0f / 3.0f);
    float cameraTarget[3] = { 0.0f, 0.0f, 0.0f };
    float cameraPos[3] = { 0.0f, 0.0f, 3.0f };

    if (benchmarkOptions.runSuite) {
        const int benchmarkExitCode = runBenchmarkSuite(window,
                                                        renderer,
                                                        benchmarkOptions,
                                                        lightDir,
                                                        lightColor,
                                                        ambientColor,
                                                        backgroundColor,
                                                        ambientIntensity,
                                                        directLightIntensity);
        glfwDestroyWindow(window);
        glfwTerminate();
        return benchmarkExitCode;
    }

    std::vector<Shape> shapes;
    Shape initialSphere;
    initialSphere.type = SHAPE_SPHERE;
    initialSphere.center[0] = initialSphere.center[1] = initialSphere.center[2] = 0.0f;
    initialSphere.param[0] = 0.5f;
    initialSphere.param[1] = initialSphere.param[2] = 0.0f;
    initialSphere.rotation[0] = initialSphere.rotation[1] = initialSphere.rotation[2] = 0.0f;
    initialSphere.scale[0] = initialSphere.scale[1] = initialSphere.scale[2] = 1.0f;
    initialSphere.fractalExtra[0] = 1.0f;
    initialSphere.fractalExtra[1] = 1.0f;
    initialSphere.extra = 0.0f;
    initialSphere.scaleMode = SCALE_MODE_DEFORM;
    initialSphere.elongation[0] = initialSphere.elongation[1] = initialSphere.elongation[2] = 0.0f;
    initialSphere.twist[0] = initialSphere.twist[1] = initialSphere.twist[2] = 0.0f;
    initialSphere.bend[0] = initialSphere.bend[1] = initialSphere.bend[2] = 0.0f;
    initialSphere.mirrorModifierEnabled = false;
    initialSphere.mirrorAxis[0] = false;
    initialSphere.mirrorAxis[1] = false;
    initialSphere.mirrorAxis[2] = false;
    initialSphere.mirrorOffset[0] = 0.0f;
    initialSphere.mirrorOffset[1] = 0.0f;
    initialSphere.mirrorOffset[2] = 0.0f;
    initialSphere.mirrorSmoothness = 0.0f;
    initialSphere.arrayModifierEnabled = false;
    initialSphere.arrayAxis[0] = false;
    initialSphere.arrayAxis[1] = false;
    initialSphere.arrayAxis[2] = false;
    initialSphere.arraySpacing[0] = 2.0f;
    initialSphere.arraySpacing[1] = 2.0f;
    initialSphere.arraySpacing[2] = 2.0f;
    initialSphere.arrayRepeatCount[0] = 3;
    initialSphere.arrayRepeatCount[1] = 3;
    initialSphere.arrayRepeatCount[2] = 3;
    initialSphere.arraySmoothness = 0.0f;
    initialSphere.modifierStack[0] = SHAPE_MODIFIER_BEND;
    initialSphere.modifierStack[1] = SHAPE_MODIFIER_TWIST;
    initialSphere.modifierStack[2] = SHAPE_MODIFIER_ARRAY;
    initialSphere.name = "0";
    initialSphere.blendOp = BLEND_NONE;
    initialSphere.smoothness = 0.1f;
    initialSphere.albedo[0] = 1.0f;
    initialSphere.albedo[1] = 1.0f;
    initialSphere.albedo[2] = 1.0f;
    initialSphere.metallic = 0.0f;
    initialSphere.roughness = 0.05f;
    initialSphere.emission[0] = 0.0f;
    initialSphere.emission[1] = 0.0f;
    initialSphere.emission[2] = 0.0f;
    initialSphere.emissionStrength = 0.0f;
    initialSphere.transmission = 0.0f;
    initialSphere.ior = 1.5f;
    initialSphere.dispersion = 0.0f;
    shapes.push_back(initialSphere);

    if (benchmarkOptions.evenMixShapeCount > 0) {
        shapes = buildEvenMixBenchmarkScene(benchmarkOptions.evenMixShapeCount);
        std::cout << "Benchmark scene enabled: even-mix profile with "
                  << shapes.size() << " shapes" << std::endl;
    }

    std::vector<int> selectedShapes;
    bool useGradientBackground = false;
    bool altRenderMode = false;
    bool usePathTracer = false;
    RenderSettings renderSettings;

    InputManager inputManager;
    GUIManager guiManager;
    UndoRedoManager undoRedoManager;
    EditorRuntimeState runtimeState;

    const int loopExitCode = runEditorRuntimeLoop(window,
                                                  renderer,
                                                  shapes,
                                                  selectedShapes,
                                                  lightDir,
                                                  lightColor,
                                                  ambientColor,
                                                  backgroundColor,
                                                  ambientIntensity,
                                                  directLightIntensity,
                                                  cameraPos,
                                                  cameraTarget,
                                                  camTheta,
                                                  camPhi,
                                                  useGradientBackground,
                                                  altRenderMode,
                                                  usePathTracer,
                                                  renderSettings,
                                                  inputManager,
                                                  guiManager,
                                                  undoRedoManager,
                                                  runtimeState);

    ImGuiLayer::Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return loopExitCode;
}

} // namespace rmt
