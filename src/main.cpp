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
#include <cstdlib>

#include "ImGuiLayer.h"
#include "Shapes.h"
#include "Input.h"         
#include "Utilities.h"
#include "GuiManager.h"    
#include "UndoRedo.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuizmo.h"

#include "Renderer.h"

float camDistance = 3.0f;

namespace {

struct BenchmarkOptions {
    int evenMixShapeCount;

    BenchmarkOptions() : evenMixShapeCount(0) {}
};

bool parseIntArgument(const std::string& value, int& outValue) {
    if (value.empty()) {
        return false;
    }
    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != '\0') {
        return false;
    }
    if (parsed <= 0 || parsed > 2000000) {
        return false;
    }
    outValue = static_cast<int>(parsed);
    return true;
}

BenchmarkOptions parseBenchmarkOptions(int argc, char** argv) {
    BenchmarkOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i] != nullptr ? argv[i] : "");
        const std::string prefix = "--benchmark-even-mix=";
        if (arg.find(prefix) == 0) {
            int value = 0;
            if (parseIntArgument(arg.substr(prefix.size()), value)) {
                options.evenMixShapeCount = value;
            }
            continue;
        }

        if (arg == "--benchmark-even-mix" && i + 1 < argc) {
            int value = 0;
            if (parseIntArgument(std::string(argv[i + 1]), value)) {
                options.evenMixShapeCount = value;
            }
            ++i;
            continue;
        }
    }
    return options;
}

std::vector<Shape> buildEvenMixBenchmarkScene(int shapeCount) {
    std::vector<Shape> shapes;
    if (shapeCount <= 0) {
        return shapes;
    }
    shapes.reserve(static_cast<std::size_t>(shapeCount));

    const float spacing = 1.35f;
    const int side = std::max(1, static_cast<int>(std::ceil(std::cbrt(static_cast<float>(shapeCount)))));
    const float centerOffset = 0.5f * static_cast<float>(side - 1);

    for (int i = 0; i < shapeCount; ++i) {
        Shape shape;
        shape.type = i % 6;

        const int gx = i % side;
        const int gy = (i / side) % side;
        const int gz = i / (side * side);

        shape.center[0] = (static_cast<float>(gx) - centerOffset) * spacing;
        shape.center[1] = (static_cast<float>(gy) - centerOffset) * spacing * 0.6f;
        shape.center[2] = (static_cast<float>(gz) - centerOffset) * spacing;

        shape.rotation[0] = 0.07f * static_cast<float>((i * 3) % 11);
        shape.rotation[1] = 0.05f * static_cast<float>((i * 5) % 13);
        shape.rotation[2] = 0.03f * static_cast<float>((i * 7) % 17);

        shape.scale[0] = shape.scale[1] = shape.scale[2] = 1.0f;
        shape.scaleMode = SCALE_MODE_ELONGATE;
        shape.elongation[0] = shape.elongation[1] = shape.elongation[2] = 0.0f;
        shape.extra = 0.0f;
        shape.twist[0] = shape.twist[1] = shape.twist[2] = 0.0f;
        shape.bend[0] = shape.bend[1] = shape.bend[2] = 0.0f;
        shape.twistModifierEnabled = false;
        shape.bendModifierEnabled = false;
        shape.mirrorModifierEnabled = false;
        shape.mirrorAxis[0] = false;
        shape.mirrorAxis[1] = false;
        shape.mirrorAxis[2] = false;
        shape.mirrorOffset[0] = 0.0f;
        shape.mirrorOffset[1] = 0.0f;
        shape.mirrorOffset[2] = 0.0f;
        shape.mirrorSmoothness = 0.0f;

        if (shape.type == SHAPE_SPHERE) {
            shape.param[0] = 0.45f;
            shape.param[1] = 0.0f;
            shape.param[2] = 0.0f;
            shape.albedo[0] = 0.90f; shape.albedo[1] = 0.78f; shape.albedo[2] = 0.75f;
        } else if (shape.type == SHAPE_BOX) {
            shape.param[0] = 0.38f;
            shape.param[1] = 0.42f;
            shape.param[2] = 0.36f;
            shape.albedo[0] = 0.72f; shape.albedo[1] = 0.85f; shape.albedo[2] = 0.95f;
        } else if (shape.type == SHAPE_TORUS) {
            shape.param[0] = 0.42f;
            shape.param[1] = 0.14f;
            shape.param[2] = 0.0f;
            shape.albedo[0] = 0.86f; shape.albedo[1] = 0.95f; shape.albedo[2] = 0.77f;
        } else if (shape.type == SHAPE_CYLINDER) {
            shape.param[0] = 0.30f;
            shape.param[1] = 0.50f;
            shape.param[2] = 0.0f;
            shape.albedo[0] = 0.83f; shape.albedo[1] = 0.78f; shape.albedo[2] = 0.96f;
        } else if (shape.type == SHAPE_CONE) {
            shape.param[0] = 0.36f;
            shape.param[1] = 0.55f;
            shape.param[2] = 0.0f;
            shape.albedo[0] = 0.74f; shape.albedo[1] = 0.94f; shape.albedo[2] = 0.84f;
        } else {
            shape.param[0] = 8.0f;
            shape.param[1] = 10.0f;
            shape.param[2] = 4.0f;
            shape.albedo[0] = 0.95f; shape.albedo[1] = 0.62f; shape.albedo[2] = 0.28f;
        }

        const int blendCycle = i % 3;
        if (blendCycle == 0) {
            shape.blendOp = BLEND_NONE;
        } else if (blendCycle == 1) {
            shape.blendOp = BLEND_SMOOTH_SUBTRACTION;
        } else {
            shape.blendOp = BLEND_SMOOTH_INTERSECTION;
        }

        shape.smoothness = 0.12f;
        shape.metallic = 0.0f;
        shape.roughness = 0.05f;
        shape.emission[0] = 0.0f;
        shape.emission[1] = 0.0f;
        shape.emission[2] = 0.0f;
        shape.emissionStrength = 0.0f;
        shape.transmission = 0.0f;
        shape.ior = 1.5f;
        shape.name = std::to_string(i);
        shapes.push_back(shape);
    }

    return shapes;
}

void resetTransformInteractionState(TransformationState& ts) {
    ts.translationModeActive = false;
    ts.rotationModeActive = false;
    ts.scaleModeActive = false;
    ts.translationConstrained = false;
    ts.rotationConstrained = false;
    ts.scaleConstrained = false;
    ts.translationAxis = -1;
    ts.rotationAxis = -1;
    ts.scaleAxis = -1;
    ts.translationOriginalCenters.clear();
    ts.rotationOriginalRotations.clear();
    ts.rotationOriginalCenters.clear();
    ts.scaleOriginalScales.clear();
    ts.scaleOriginalElongations.clear();
    ts.showAxisGuides = false;
    ts.activeAxis = -1;
    ts.guideCenter[0] = 0.0f;
    ts.guideCenter[1] = 0.0f;
    ts.guideCenter[2] = 0.0f;
    ts.guideAxisDirection[0] = 1.0f;
    ts.guideAxisDirection[1] = 0.0f;
    ts.guideAxisDirection[2] = 0.0f;
    ts.rotationCenter[0] = 0.0f;
    ts.rotationCenter[1] = 0.0f;
    ts.rotationCenter[2] = 0.0f;
    ts.translationKeyHandled = false;
    ts.rotationKeyHandled = false;
    ts.scaleKeyHandled = false;
    ts.mirrorHelperSelected = false;
    ts.mirrorHelperShapeIndex = -1;
    ts.mirrorHelperAxis = -1;
    ts.mirrorHelperMoveModeActive = false;
    ts.mirrorHelperMoveConstrained = false;
    ts.mirrorHelperMoveAxis = -1;
}

} // namespace


void updateCameraPosition(float camDistance, float camTheta, float camPhi, const float cameraTarget[3], float cameraPos[3])
{
    sphericalToCartesian(camDistance, camTheta, camPhi, cameraTarget, cameraPos);
}


int main(int argc, char** argv)
{
    const BenchmarkOptions benchmarkOptions = parseBenchmarkOptions(argc, argv);

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

    GLFWwindow* window = glfwCreateWindow(1600, 1000, "", NULL, NULL);
    if (!window)
    {
         std::cerr << "Failed to create GLFW window\n";
         glfwTerminate();
         return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
         std::cerr << "Failed to initialize GLAD\n";
         return -1;
    }

    // --- 2. Initialize ImGui ---
    ImGuiLayer::Init(window);

    // --- 3. Set up full-screen quad ---
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

    Renderer renderer(VAO);

    // --- 4. Lighting parameters ---
    float lightDir[3] = { 0.5f, 0.8f, 1.0f };
    float lightColor[3] = { 1.0f, 1.0f, 1.0f };
    float ambientColor[3] = { 57.0f / 255.0f, 57.0f / 255.0f, 57.0f / 255.0f };
    float backgroundColor[3] = { 0.0f, 0.0f, 0.0f };
    float ambientIntensity = 0.5f;
    float directLightIntensity = 3.6f;

    // --- 5. Camera orbit parameters ---
    float camTheta = 0.0f;
    float camPhi = asinf(1.0f / 3.0f);
    float cameraTarget[3] = { 0.0f, 0.0f, 0.0f };
    float cameraPos[3] = { 0.0f, 0.0f, 3.0f };

    static bool cameraDragging = false;
    static double lastMouseX = 0, lastMouseY = 0;
    bool mouseWasPressed = false;

    // --- 6. Initialize shapes and state variables ---
    std::vector<Shape> shapes;
    Shape initialSphere;
    initialSphere.type = SHAPE_SPHERE;
    initialSphere.center[0] = initialSphere.center[1] = initialSphere.center[2] = 0.0f;
    initialSphere.param[0] = 0.5f;
    initialSphere.param[1] = initialSphere.param[2] = 0.0f;
    initialSphere.rotation[0] = initialSphere.rotation[1] = initialSphere.rotation[2] = 0.0f;
    initialSphere.scale[0] = initialSphere.scale[1] = initialSphere.scale[2] = 1.0f;
    initialSphere.extra = 0.0f;
    initialSphere.scaleMode = SCALE_MODE_ELONGATE;
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
    initialSphere.name = "0";
    initialSphere.blendOp = BLEND_NONE;
    initialSphere.smoothness = 0.1f;
    initialSphere.albedo[0] = 1.0f; initialSphere.albedo[1] = 1.0f; initialSphere.albedo[2] = 1.0f;
    initialSphere.metallic = 0.0f;
    initialSphere.roughness = 0.05f;
    initialSphere.emission[0] = 0.0f; initialSphere.emission[1] = 0.0f; initialSphere.emission[2] = 0.0f;
    initialSphere.emissionStrength = 0.0f;
    initialSphere.transmission = 0.0f;
    initialSphere.ior = 1.5f;
    shapes.push_back(initialSphere);

    if (benchmarkOptions.evenMixShapeCount > 0) {
        shapes = buildEvenMixBenchmarkScene(benchmarkOptions.evenMixShapeCount);
        std::cout << "Benchmark scene enabled: even-mix profile with "
                  << shapes.size() << " shapes" << std::endl;
    }

    std::vector<int> selectedShapes;
    bool useGradientBackground = false;
    static bool altRenderMode = false;
    static bool usePathTracer = false;
    RenderSettings renderSettings;
    static int editingShapeIndex = -1;
    static char renameBuffer[128] = "";
    bool showGUI = true;
    bool showHelpPopup = false;
    bool showConsole = false;
    ImVec2 helpButtonPos;

    InputManager inputManager;
    GUIManager guiManager;
    TransformationState transformState;
    UndoRedoManager undoRedoManager;
    undoRedoManager.initialize(shapes, selectedShapes);
    bool undoShortcutHandled = false;
    bool redoShortcutHandled = false;

    // --- 7. Main render loop ---
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImVec2 viewportPos(0.0f, 0.0f);
        ImVec2 viewportSize(0.0f, 0.0f);
        if (showGUI) {
            viewportPos = guiManager.getViewportPos();
            viewportSize = guiManager.getViewportSize();
        }

        inputManager.processGeneralInput(window, shapes, selectedShapes, cameraTarget, showGUI, altRenderMode, transformState);
        inputManager.processTransformationModeActivation(window, shapes, selectedShapes, transformState);
        inputManager.processTransformationUpdates(window, shapes, selectedShapes, transformState, cameraPos, cameraTarget,
                                                 viewportPos, viewportSize);
        updateCameraPosition(camDistance, camTheta, camPhi, cameraTarget, cameraPos);

        if (showGUI)
        {
            guiManager.newFrame();
            guiManager.renderGUI(window, shapes, selectedShapes, lightDir, lightColor, ambientColor, backgroundColor,
                                 ambientIntensity, directLightIntensity,
                                 useGradientBackground,
                                 showGUI, altRenderMode, usePathTracer, editingShapeIndex, renameBuffer,
                                 showHelpPopup, helpButtonPos, showConsole, transformState,
                                 cameraPos, cameraTarget,
                                 renderSettings,
                                 renderer.isDenoiserAvailable(),
                                 renderer.isDenoiserUsingGPU(),
                                 renderer.getPathSampleCount());
        }

        if (showGUI) {
            viewportPos = guiManager.getViewportPos();
            viewportSize = guiManager.getViewportSize();
        }
        inputManager.processMousePickingAndCameraDrag(window, shapes, selectedShapes, cameraPos, cameraTarget,
                                                      camTheta, camPhi, cameraDragging, lastMouseX, lastMouseY,
                                                      mouseWasPressed, transformState, viewportPos, viewportSize);
        updateCameraPosition(camDistance, camTheta, camPhi, cameraTarget, cameraPos);

        const bool leftCtrlDown = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
        const bool rightCtrlDown = glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        const bool ctrlDown = leftCtrlDown || rightCtrlDown;
        const bool shiftDown = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
                               (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
        const bool zDown = glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS;
        const bool yDown = glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS;
        const bool redoChordDown = yDown || (shiftDown && zDown);
        const bool undoChordDown = zDown && !shiftDown;
        const bool keyboardCapturedByUI = showGUI && ImGui::GetIO().WantCaptureKeyboard;
        const bool transformInteractionActive =
            transformState.translationModeActive ||
            transformState.rotationModeActive ||
            transformState.scaleModeActive ||
            transformState.mirrorHelperMoveModeActive;
        const bool shortcutsAllowed = !keyboardCapturedByUI && !transformInteractionActive;

        if (shortcutsAllowed && ctrlDown && undoChordDown && !undoShortcutHandled) {
            if (undoRedoManager.undo(shapes, selectedShapes)) {
                resetTransformInteractionState(transformState);
            }
            undoShortcutHandled = true;
        }
        if (shortcutsAllowed && ctrlDown && redoChordDown && !redoShortcutHandled) {
            if (undoRedoManager.redo(shapes, selectedShapes)) {
                resetTransformInteractionState(transformState);
            }
            redoShortcutHandled = true;
        }
        if (!(ctrlDown && undoChordDown)) {
            undoShortcutHandled = false;
        }
        if (!(ctrlDown && redoChordDown)) {
            redoShortcutHandled = false;
        }

        const bool uiInteractionActive = showGUI && (ImGui::IsAnyItemActive() || ImGuizmo::IsUsing());
        undoRedoManager.capture(shapes, selectedShapes, uiInteractionActive || transformInteractionActive);

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        int renderX = 0;
        int renderY = 0;
        int renderW = std::max(1, display_w);
        int renderH = std::max(1, display_h);
        if (showGUI) {
            const ImVec2 currentViewportPos = guiManager.getViewportPos();
            const ImVec2 currentViewportSize = guiManager.getViewportSize();
            int winWidth, winHeight;
            glfwGetWindowSize(window, &winWidth, &winHeight);
            const float scaleX = (float)display_w / std::max(1, winWidth);
            const float scaleY = (float)display_h / std::max(1, winHeight);
            int vpX = static_cast<int>(currentViewportPos.x * scaleX);
            int vpYFromTop = static_cast<int>(currentViewportPos.y * scaleY);
            int vpW = std::max(1, static_cast<int>(currentViewportSize.x * scaleX));
            int vpH = std::max(1, static_cast<int>(currentViewportSize.y * scaleY));
            int vpY = display_h - (vpYFromTop + vpH);

            renderX = std::max(0, std::min(vpX, std::max(0, display_w - 1)));
            renderY = std::max(0, std::min(vpY, std::max(0, display_h - 1)));
            renderW = std::max(1, std::min(vpW, display_w - renderX));
            renderH = std::max(1, std::min(vpH, display_h - renderY));
        }

        glViewport(0, 0, display_w, display_h);
        glClearColor(0.06f, 0.06f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(renderX, renderY, renderW, renderH);

        renderer.renderScene(shapes, selectedShapes, lightDir, lightColor, ambientColor,
                             backgroundColor, ambientIntensity, directLightIntensity,
                             cameraPos, cameraTarget, renderW, renderH,
                             altRenderMode, useGradientBackground, usePathTracer,
                             renderSettings, transformState);

        if (showGUI)
            ImGuiLayer::Render();

        glfwSwapBuffers(window);
    }

    ImGuiLayer::Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
