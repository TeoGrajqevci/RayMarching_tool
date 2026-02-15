#include "rmt/app/EditorRuntime.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "ImGuizmo.h"
#include "imgui.h"

#include "rmt/app/OrbitCamera.h"
#include "rmt/app/Application.h"
#include "rmt/app/UndoRedoManager.h"
#include "rmt/input/Input.h"
#include "rmt/io/mesh/import/MeshImporter.h"
#include "rmt/platform/glfw/FileDropQueue.h"
#include "rmt/platform/imgui/ImGuiLayer.h"
#include "rmt/ui/UiFacade.h"

namespace rmt {

namespace {

const int kImportedMeshSdfResolution = 128;

} // namespace

EditorRuntimeState::EditorRuntimeState()
    : showGui(true),
      showHelpPopup(false),
      showConsole(false),
      editingShapeIndex(-1),
      renameBuffer() {}

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
    ts.sunLightSelected = false;
    ts.sunLightMoveModeActive = false;
    ts.sunLightMoveConstrained = false;
    ts.sunLightMoveAxis = -1;
    ts.sunLightHandleDragActive = false;
    ts.selectedPointLightIndex = -1;
    ts.pointLightMoveModeActive = false;
    ts.pointLightMoveConstrained = false;
    ts.pointLightMoveAxis = -1;
}

bool isInsideViewport(double x, double y, const ImVec2& viewportPos, const ImVec2& viewportSize) {
    return x >= static_cast<double>(viewportPos.x) &&
           y >= static_cast<double>(viewportPos.y) &&
           x <= static_cast<double>(viewportPos.x + viewportSize.x) &&
           y <= static_cast<double>(viewportPos.y + viewportSize.y);
}

int runEditorRuntimeLoop(GLFWwindow* window,
                         Renderer& renderer,
                         std::vector<Shape>& shapes,
                         std::vector<int>& selectedShapes,
                         float lightDir[3],
                         float lightColor[3],
                         float ambientColor[3],
                         float backgroundColor[3],
                         float& ambientIntensity,
                         float& directLightIntensity,
                         float cameraPos[3],
                         float cameraTarget[3],
                         float& camTheta,
                         float& camPhi,
                         bool& useGradientBackground,
                         bool& altRenderMode,
                         bool& usePathTracer,
                         RenderSettings& renderSettings,
                         InputManager& inputManager,
                         GUIManager& guiManager,
                         UndoRedoManager& undoRedoManager,
                         EditorRuntimeState& runtimeState) {
    static bool cameraDragging = false;
    static double lastMouseX = 0.0;
    static double lastMouseY = 0.0;
    bool mouseWasPressed = false;

    TransformationState transformState;
    if (!transformState.sunLightInitialized) {
        transformState.sunLightPosition[0] = 1.5f;
        transformState.sunLightPosition[1] = 2.0f;
        transformState.sunLightPosition[2] = 2.5f;
        transformState.sunLightInitialized = true;
    }
    for (PointLightState& pointLight : transformState.pointLights) {
        pointLight.color[0] = 1.0f;
        pointLight.color[1] = 1.0f;
        pointLight.color[2] = 1.0f;
    }
    undoRedoManager.initialize(shapes, selectedShapes);
    bool undoShortcutHandled = false;
    bool redoShortcutHandled = false;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImVec2 viewportPos(0.0f, 0.0f);
        ImVec2 viewportSize(0.0f, 0.0f);
        if (runtimeState.showGui) {
            viewportPos = guiManager.getViewportPos();
            viewportSize = guiManager.getViewportSize();
        }

        bool viewportHovered = runtimeState.showGui ? guiManager.isViewportHovered() : true;
        bool uiWantsMouseCapture = runtimeState.showGui && ImGui::GetIO().WantCaptureMouse;
        if (!runtimeState.showGui) {
            int winWidth = 1;
            int winHeight = 1;
            glfwGetWindowSize(window, &winWidth, &winHeight);
            viewportPos = ImVec2(0.0f, 0.0f);
            viewportSize = ImVec2(static_cast<float>(std::max(1, winWidth)),
                                  static_cast<float>(std::max(1, winHeight)));
        }
        updateViewportInputContext(viewportPos.x,
                                   viewportPos.y,
                                   viewportSize.x,
                                   viewportSize.y,
                                   viewportHovered,
                                   uiWantsMouseCapture);

        inputManager.processGeneralInput(window,
                                         shapes,
                                         selectedShapes,
                                         cameraTarget,
                                         runtimeState.showGui,
                                         altRenderMode,
                                         transformState);
        inputManager.processTransformationModeActivation(window, shapes, selectedShapes, transformState);
        inputManager.processTransformationUpdates(window,
                                                  shapes,
                                                  selectedShapes,
                                                  transformState,
                                                  lightDir,
                                                  cameraPos,
                                                  cameraTarget,
                                                  viewportPos,
                                                  viewportSize);
        updateOrbitCameraPosition(camDistance, camTheta, camPhi, cameraTarget, cameraPos);

        ImVec2 helpButtonPos;
        if (runtimeState.showGui) {
            guiManager.newFrame();
            guiManager.renderGUI(window,
                                 shapes,
                                 selectedShapes,
                                 lightDir,
                                 lightColor,
                                 ambientColor,
                                 backgroundColor,
                                 ambientIntensity,
                                 directLightIntensity,
                                 useGradientBackground,
                                 runtimeState.showGui,
                                 altRenderMode,
                                 usePathTracer,
                                 runtimeState.editingShapeIndex,
                                 runtimeState.renameBuffer,
                                 runtimeState.showHelpPopup,
                                 helpButtonPos,
                                 runtimeState.showConsole,
                                 transformState,
                                 cameraPos,
                                 cameraTarget,
                                 renderSettings,
                                 renderer.isDenoiserAvailable(),
                                 renderer.isDenoiserUsingGPU(),
                                 renderer.getPathSampleCount());
        }

        if (runtimeState.showGui) {
            viewportPos = guiManager.getViewportPos();
            viewportSize = guiManager.getViewportSize();
        }

        const std::vector<rmt::DroppedFileEvent> droppedFiles = rmt::consumeDroppedFileEvents();
        if (!droppedFiles.empty()) {
            for (std::size_t dropIndex = 0; dropIndex < droppedFiles.size(); ++dropIndex) {
                const rmt::DroppedFileEvent& dropped = droppedFiles[dropIndex];
                if (dropped.path.empty()) {
                    continue;
                }

                if (runtimeState.showGui && !isInsideViewport(dropped.mouseX, dropped.mouseY, viewportPos, viewportSize)) {
                    Console::getInstance().addLog("Ignored drop outside viewport: " + dropped.path, 1);
                    continue;
                }

                if (!isSupportedMeshImportFile(dropped.path)) {
                    Console::getInstance().addLog("Unsupported dropped file (only .obj/.fbx): " + dropped.path, 1);
                    continue;
                }

                Console::getInstance().addLog("Importing mesh: " + dropped.path, 0);
                MeshImportResult importResult = importMeshAsSDFShape(dropped.path, kImportedMeshSdfResolution);
                if (!importResult.success) {
                    Console::getInstance().addLog("Import failed: " + importResult.message, 2);
                    runtimeState.showConsole = true;
                    continue;
                }

                Shape importedShape = importResult.shape;
                const float dropStackOffset = 0.35f * static_cast<float>(dropIndex);
                importedShape.center[0] = cameraTarget[0] + dropStackOffset;
                importedShape.center[1] = cameraTarget[1];
                importedShape.center[2] = cameraTarget[2];
                importedShape.name = importedShape.name + " " + std::to_string(shapes.size());
                shapes.push_back(importedShape);
                selectedShapes.clear();
                selectedShapes.push_back(static_cast<int>(shapes.size()) - 1);
                resetTransformInteractionState(transformState);
                Console::getInstance().addLog(importResult.message, 0);
                runtimeState.showConsole = true;
            }
        }

        inputManager.processMousePickingAndCameraDrag(window,
                                                      shapes,
                                                      selectedShapes,
                                                      cameraPos,
                                                      cameraTarget,
                                                      camTheta,
                                                      camPhi,
                                                      cameraDragging,
                                                      lastMouseX,
                                                      lastMouseY,
                                                      mouseWasPressed,
                                                      transformState,
                                                      lightDir,
                                                      viewportPos,
                                                      viewportSize);
        updateOrbitCameraPosition(camDistance, camTheta, camPhi, cameraTarget, cameraPos);

        const bool leftCtrlDown = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
        const bool rightCtrlDown = glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        const bool ctrlDown = leftCtrlDown || rightCtrlDown;
        const bool shiftDown = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
                               (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
        const bool zDown = glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS;
        const bool yDown = glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS;
        const bool redoChordDown = yDown || (shiftDown && zDown);
        const bool undoChordDown = zDown && !shiftDown;
        const bool keyboardCapturedByUI = runtimeState.showGui && ImGui::GetIO().WantCaptureKeyboard;
        const bool transformInteractionActive = transformState.translationModeActive ||
                                               transformState.rotationModeActive ||
                                               transformState.scaleModeActive ||
                                               transformState.mirrorHelperMoveModeActive ||
                                               transformState.sunLightMoveModeActive ||
                                               transformState.sunLightHandleDragActive ||
                                               transformState.pointLightMoveModeActive;
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

        const bool uiInteractionActive = runtimeState.showGui && (ImGui::IsAnyItemActive() || ImGuizmo::IsUsing());
        undoRedoManager.capture(shapes, selectedShapes, uiInteractionActive || transformInteractionActive);

        int displayW = 1;
        int displayH = 1;
        glfwGetFramebufferSize(window, &displayW, &displayH);
        int renderX = 0;
        int renderY = 0;
        int renderW = std::max(1, displayW);
        int renderH = std::max(1, displayH);

        if (runtimeState.showGui) {
            const ImVec2 currentViewportPos = guiManager.getViewportPos();
            const ImVec2 currentViewportSize = guiManager.getViewportSize();
            int winWidth = 1;
            int winHeight = 1;
            glfwGetWindowSize(window, &winWidth, &winHeight);
            const float scaleX = static_cast<float>(displayW) / std::max(1, winWidth);
            const float scaleY = static_cast<float>(displayH) / std::max(1, winHeight);
            const int vpX = static_cast<int>(currentViewportPos.x * scaleX);
            const int vpYFromTop = static_cast<int>(currentViewportPos.y * scaleY);
            const int vpW = std::max(1, static_cast<int>(currentViewportSize.x * scaleX));
            const int vpH = std::max(1, static_cast<int>(currentViewportSize.y * scaleY));
            const int vpY = displayH - (vpYFromTop + vpH);

            renderX = std::max(0, std::min(vpX, std::max(0, displayW - 1)));
            renderY = std::max(0, std::min(vpY, std::max(0, displayH - 1)));
            renderW = std::max(1, std::min(vpW, displayW - renderX));
            renderH = std::max(1, std::min(vpH, displayH - renderY));
        }

        glViewport(0, 0, displayW, displayH);
        glClearColor(0.06f, 0.06f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(renderX, renderY, renderW, renderH);

        renderer.renderScene(shapes,
                             selectedShapes,
                             lightDir,
                             lightColor,
                             ambientColor,
                             backgroundColor,
                             ambientIntensity,
                             transformState.sunLightEnabled ? directLightIntensity : 0.0f,
                             cameraPos,
                             cameraTarget,
                             renderW,
                             renderH,
                             altRenderMode,
                             useGradientBackground,
                             usePathTracer,
                             renderSettings,
                             transformState);

        if (runtimeState.showGui) {
            ImGuiLayer::Render();
        }

        glfwSwapBuffers(window);
    }

    return 0;
}

} // namespace rmt
