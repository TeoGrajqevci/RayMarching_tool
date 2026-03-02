#include "rmt/ui/UiFacade.h"

#include <algorithm>

#include "rmt/ui/gizmo/GizmoController.h"
#include "rmt/ui/layout/WorkspaceLayout.h"
#include "rmt/ui/panels/ExportDialogPanel.h"
#include "rmt/ui/panels/InspectorPanel.h"
#include "rmt/ui/panels/ModifierPanel.h"
#include "rmt/ui/panels/ScenePanel.h"
#include "rmt/ui/panels/ShortcutsPanel.h"
#include "rmt/ui/panels/ToolbarPanel.h"
#include "rmt/ui/panels/ViewportPanel.h"
#include "rmt/ui/style/ImGuiTheme.h"

namespace rmt {

namespace {

bool tryAddCurveNodeWithShortcut(std::vector<Shape>& shapes,
                                 const std::vector<int>& selectedShapes,
                                 TransformationState& transformState) {
    if (selectedShapes.size() != 1) {
        return false;
    }

    const int shapeIndex = selectedShapes[0];
    if (shapeIndex < 0 || shapeIndex >= static_cast<int>(shapes.size())) {
        return false;
    }

    Shape& shape = shapes[shapeIndex];
    if (shape.type != SHAPE_CURVE) {
        return false;
    }
    if (!transformState.curveEditMode) {
        return false;
    }
    if (!transformState.curveNodeSelected ||
        transformState.curveNodeShapeIndex != shapeIndex ||
        transformState.curveNodeIndex < 0 ||
        transformState.curveNodeIndex >= static_cast<int>(shape.curveNodes.size())) {
        return false;
    }

    const int selectedNodeIndex = transformState.curveNodeIndex;
    const CurveNode& selectedNode = shape.curveNodes[static_cast<std::size_t>(selectedNodeIndex)];

    CurveNode newNode;
    if (selectedNodeIndex + 1 < static_cast<int>(shape.curveNodes.size())) {
        const CurveNode& nextNode = shape.curveNodes[static_cast<std::size_t>(selectedNodeIndex + 1)];
        newNode.position[0] = 0.5f * (selectedNode.position[0] + nextNode.position[0]);
        newNode.position[1] = 0.5f * (selectedNode.position[1] + nextNode.position[1]);
        newNode.position[2] = 0.5f * (selectedNode.position[2] + nextNode.position[2]);
        newNode.radius = 0.5f * (selectedNode.radius + nextNode.radius);
    } else {
        newNode.position[0] = selectedNode.position[0] + 0.3f;
        newNode.position[1] = selectedNode.position[1];
        newNode.position[2] = selectedNode.position[2];
        newNode.radius = selectedNode.radius;
    }

    shape.curveNodes.insert(shape.curveNodes.begin() + selectedNodeIndex + 1, newNode);
    transformState.curveNodeIndex = selectedNodeIndex + 1;
    transformState.curveNodeShapeIndex = shapeIndex;
    transformState.curveNodeSelected = true;
    return true;
}

void renderAddShapePopup(UiRuntimeState& uiState,
                         std::vector<Shape>& shapes,
                         std::vector<int>& selectedShapes,
                         TransformationState& transformState) {
    ImGui::SetNextWindowPos(uiState.addShapePopupPos);
    if (!ImGui::BeginPopup("Add Shape")) {
        return;
    }

    if (ImGui::BeginMenu("Shapes")) {
        if (ImGui::MenuItem("Sphere")) {
            Shape newShape;
            initShapeDefaults(newShape, SHAPE_SPHERE, uiState.globalScaleMode, shapes.size());
            newShape.param[0] = 0.5f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("Box")) {
            Shape newShape;
            initShapeDefaults(newShape, SHAPE_BOX, uiState.globalScaleMode, shapes.size());
            newShape.param[0] = newShape.param[1] = newShape.param[2] = 0.5f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("Torus")) {
            Shape newShape;
            initShapeDefaults(newShape, SHAPE_TORUS, uiState.globalScaleMode, shapes.size());
            newShape.param[0] = 0.5f;
            newShape.param[1] = 0.2f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("Cylinder")) {
            Shape newShape;
            initShapeDefaults(newShape, SHAPE_CYLINDER, uiState.globalScaleMode, shapes.size());
            newShape.param[0] = 0.5f;
            newShape.param[1] = 0.5f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("Cone")) {
            Shape newShape;
            initShapeDefaults(newShape, SHAPE_CONE, uiState.globalScaleMode, shapes.size());
            newShape.param[0] = 0.5f;
            newShape.param[1] = 0.6f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("Mandelbulb")) {
            Shape newShape;
            initShapeDefaults(newShape, SHAPE_MANDELBULB, uiState.globalScaleMode, shapes.size());
            newShape.param[0] = 8.0f;
            newShape.param[1] = 10.0f;
            newShape.param[2] = 4.0f;
            newShape.fractalExtra[0] = 1.0f;
            newShape.fractalExtra[1] = 1.0f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("Menger Sponge")) {
            Shape newShape;
            initShapeDefaults(newShape, SHAPE_MENGER_SPONGE, uiState.globalScaleMode, shapes.size());
            newShape.param[0] = 0.8f;
            newShape.param[1] = 4.0f;
            newShape.param[2] = 3.0f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("Curve")) {
            Shape newShape;
            initShapeDefaults(newShape, SHAPE_CURVE, uiState.globalScaleMode, shapes.size());
            newShape.param[0] = 0.12f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Light")) {
        if (ImGui::MenuItem("Sun")) {
            if (!transformState.sunLightInitialized) {
                transformState.sunLightPosition[0] = 1.5f;
                transformState.sunLightPosition[1] = 2.0f;
                transformState.sunLightPosition[2] = 2.5f;
                transformState.sunLightInitialized = true;
            }
            transformState.sunLightEnabled = true;
            transformState.sunLightSelected = true;
            transformState.sunLightMoveModeActive = false;
            transformState.sunLightMoveConstrained = false;
            transformState.sunLightMoveAxis = -1;
            transformState.sunLightHandleDragActive = false;

            transformState.selectedPointLightIndex = -1;
            transformState.pointLightMoveModeActive = false;
            transformState.pointLightMoveConstrained = false;
            transformState.pointLightMoveAxis = -1;
            selectedShapes.clear();
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("Point")) {
            PointLightState newPointLight;
            const int pointIndex = static_cast<int>(transformState.pointLights.size());
            newPointLight.position[0] = 0.35f * static_cast<float>(pointIndex);
            newPointLight.position[1] = 1.5f;
            newPointLight.position[2] = 0.35f * static_cast<float>(pointIndex);
            transformState.pointLights.push_back(newPointLight);
            transformState.selectedPointLightIndex = static_cast<int>(transformState.pointLights.size()) - 1;
            transformState.pointLightMoveModeActive = false;
            transformState.pointLightMoveConstrained = false;
            transformState.pointLightMoveAxis = -1;

            transformState.sunLightSelected = false;
            transformState.sunLightMoveModeActive = false;
            transformState.sunLightMoveConstrained = false;
            transformState.sunLightMoveAxis = -1;
            transformState.sunLightHandleDragActive = false;

            selectedShapes.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::MenuItem("Rect", nullptr, false, false);
        ImGui::EndMenu();
    }

    ImGui::EndPopup();
}

} // namespace

GUIManager::GUIManager()
    : viewportPos(0.0f, 0.0f), viewportSize(1.0f, 1.0f), viewportHovered(false), uiState() {}

GUIManager::~GUIManager() {}

ImVec2 GUIManager::getViewportPos() const {
    return viewportPos;
}

ImVec2 GUIManager::getViewportSize() const {
    return viewportSize;
}

bool GUIManager::isViewportHovered() const {
    return viewportHovered;
}

void GUIManager::newFrame() {
    ImGuiLayer::NewFrame();
}

void GUIManager::renderGUI(GLFWwindow* window,
                           std::vector<Shape>& shapes,
                           std::vector<int>& selectedShapes,
                           float lightDir[3],
                           float lightColor[3],
                           float ambientColor[3],
                           float backgroundColor[3],
                           float& ambientIntensity,
                           float& directLightIntensity,
                           bool& useGradientBackground,
                           bool& showGUI,
                           bool& altRenderMode,
                           bool& usePathTracer,
                           int& editingShapeIndex,
                           char (&renameBuffer)[128],
                           bool& showHelpPopup,
                           ImVec2& helpButtonPos,
                           bool& showConsole,
                           TransformationState& transformState,
                           const float cameraPos[3],
                           const float cameraTarget[3],
                           float& camTheta,
                           float& camPhi,
                           RenderSettings& renderSettings,
                           bool denoiserAvailable,
                           bool denoiserUsingGPU,
                           int pathSampleCount,
                           const std::vector<DroppedFileEvent>& droppedFiles,
                           std::vector<char>& consumedDropFlags) {
    renderSettings.cameraFovDegrees = clampUiFloat(renderSettings.cameraFovDegrees, 20.0f, 120.0f);
    renderSettings.cameraProjectionMode = clampUiInt(renderSettings.cameraProjectionMode,
                                                     CAMERA_PROJECTION_PERSPECTIVE,
                                                     CAMERA_PROJECTION_ORTHOGRAPHIC);
    renderSettings.pathTracerMaxBounces = clampUiInt(renderSettings.pathTracerMaxBounces, 1, 12);
    renderSettings.pathTracerGuidedSamplingMix = clampUiFloat(renderSettings.pathTracerGuidedSamplingMix, 0.0f, 0.95f);
    renderSettings.pathTracerMisPower = clampUiFloat(renderSettings.pathTracerMisPower, 1.0f, 4.0f);
    renderSettings.pathTracerRussianRouletteStartBounce = clampUiInt(renderSettings.pathTracerRussianRouletteStartBounce, 1, 12);
    renderSettings.pathTracerResolutionScale = clampUiFloat(renderSettings.pathTracerResolutionScale, 0.5f, 1.0f);
    renderSettings.pathTracerCameraFocalLengthMm = clampUiFloat(renderSettings.pathTracerCameraFocalLengthMm, 8.0f, 400.0f);
    renderSettings.pathTracerCameraSensorWidthMm = clampUiFloat(renderSettings.pathTracerCameraSensorWidthMm, 4.0f, 70.0f);
    renderSettings.pathTracerCameraSensorHeightMm = clampUiFloat(renderSettings.pathTracerCameraSensorHeightMm, 3.0f, 70.0f);
    renderSettings.pathTracerCameraApertureFNumber = clampUiFloat(renderSettings.pathTracerCameraApertureFNumber, 0.7f, 64.0f);
    renderSettings.pathTracerCameraFocusDistance = clampUiFloat(renderSettings.pathTracerCameraFocusDistance, 0.05f, 5000.0f);
    renderSettings.pathTracerCameraBladeCount = clampUiInt(renderSettings.pathTracerCameraBladeCount, 0, 12);
    renderSettings.pathTracerCameraBladeRotationDegrees =
        clampUiFloat(renderSettings.pathTracerCameraBladeRotationDegrees, -360.0f, 360.0f);
    renderSettings.pathTracerCameraAnamorphicRatio = clampUiFloat(renderSettings.pathTracerCameraAnamorphicRatio, 0.25f, 4.0f);
    renderSettings.pathTracerCameraChromaticAberration = clampUiFloat(renderSettings.pathTracerCameraChromaticAberration, 0.0f, 8.0f);
    renderSettings.pathTracerCameraShutterSeconds = clampUiFloat(renderSettings.pathTracerCameraShutterSeconds, 1.0f / 8000.0f, 30.0f);
    renderSettings.pathTracerCameraIso = clampUiFloat(renderSettings.pathTracerCameraIso, 25.0f, 25600.0f);
    renderSettings.pathTracerCameraExposureCompensationEv =
        clampUiFloat(renderSettings.pathTracerCameraExposureCompensationEv, -10.0f, 10.0f);
    renderSettings.denoiseStartSample = clampUiInt(renderSettings.denoiseStartSample, 1, 4096);
    renderSettings.denoiseInterval = clampUiInt(renderSettings.denoiseInterval, 1, 1024);
    ambientIntensity = std::max(ambientIntensity, 0.0f);
    directLightIntensity = std::max(directLightIntensity, 0.0f);

    if (!ImGui::GetIO().WantCaptureKeyboard &&
        glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS &&
        (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
         glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)) {
        if (!uiState.addShapePopupTriggered) {
            uiState.addShapePopupTriggered = true;
            if (!tryAddCurveNodeWithShortcut(shapes, selectedShapes, transformState)) {
                double mouseX = 0.0;
                double mouseY = 0.0;
                glfwGetCursorPos(window, &mouseX, &mouseY);
                uiState.addShapePopupPos = ImVec2(static_cast<float>(mouseX), static_cast<float>(mouseY));
                ImGui::OpenPopup("Add Shape");
            }
        }
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE) {
        uiState.addShapePopupTriggered = false;
    }

    ensureViewportGizmoStyle(uiState);
    renderAddShapePopup(uiState, shapes, selectedShapes, transformState);

    for (Shape& shape : shapes) {
        shape.scaleMode = uiState.globalScaleMode;
    }

    UiWorkspaceGeometry geometry;
    updateWorkspaceLayout(window, showConsole, showHelpPopup, uiState, geometry);
    drawWorkspaceSplitters(geometry, showHelpPopup);

    renderToolbarPanel(shapes,
                       uiState,
                       transformState,
                       showGUI,
                       altRenderMode,
                       usePathTracer,
                       showConsole,
                       showHelpPopup,
                       geometry.winWidth);

    UiWorkspaceGeometry viewportGeometry = geometry;
    viewportGeometry.centerX = geometry.leftX;
    viewportGeometry.centerW = std::max(120.0f, geometry.rightX - geometry.pad - viewportGeometry.centerX);

    ImDrawList* viewportDrawList = renderViewportPanel(viewportGeometry,
                                                       viewportPos,
                                                       viewportSize,
                                                       helpButtonPos,
                                                       viewportHovered);

    renderScenePanel(geometry,
                     shapes,
                     selectedShapes,
                     editingShapeIndex,
                     renameBuffer,
                     transformState);

    renderModifierPanel(geometry,
                        shapes,
                        selectedShapes,
                        transformState,
                        cameraPos,
                        cameraTarget,
                        lightDir,
                        lightColor,
                        ambientColor,
                        backgroundColor,
                        ambientIntensity,
                        directLightIntensity,
                        renderSettings,
                        denoiserAvailable,
                        denoiserUsingGPU,
                        pathSampleCount);

    renderInspectorPanel(viewportPos,
                         viewportSize,
                         shapes,
                         selectedShapes,
                         transformState,
                         uiState.globalScaleMode,
                         droppedFiles,
                         consumedDropFlags);

    renderShortcutsPanel(geometry, showHelpPopup);

    renderExportDialogPanel(shapes,
                            uiState,
                            showConsole,
                            geometry.winWidth,
                            geometry.winHeight);

    renderViewportGizmo(viewportDrawList,
                        viewportPos,
                        viewportSize,
                        shapes,
                        selectedShapes,
                        transformState,
                        lightDir,
                        cameraPos,
                        cameraTarget,
                        camTheta,
                        camPhi,
                        renderSettings,
                        uiState);

    if (showConsole) {
        const float consoleY = geometry.bottomPaneHeight > 0.0f
            ? (geometry.contentBottom + geometry.pad)
            : std::max(geometry.pad, static_cast<float>(geometry.winHeight) - 180.0f);
        const float consoleH = geometry.bottomPaneHeight > 0.0f
            ? std::max(120.0f, geometry.bottomPaneHeight - geometry.pad)
            : 160.0f;

        ImGui::SetNextWindowPos(ImVec2(geometry.pad, consoleY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(std::max(200.0f, static_cast<float>(geometry.winWidth) - 2.0f * geometry.pad),
                                        consoleH),
                                 ImGuiCond_Always);
        Console::getInstance().render(&showConsole);
    }
}

} // namespace rmt
