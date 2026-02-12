#include "GuiManager.h"
#include "MeshExporter.h"
#include "ImGuizmo.h"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <iostream>
#include <cmath>

namespace {

constexpr float kPi = 3.14159265358979323846f;

float length3(const float v[3]) {
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

void normalize3(float v[3]) {
    const float len = length3(v);
    if (len > 1e-6f) {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
}

void cross3(const float a[3], const float b[3], float out[3]) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

float dot3(const float a[3], const float b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void buildViewMatrix(const float eye[3], const float target[3], float out[16]) {
    // Match ImGuizmo's internal LookAt convention exactly.
    float x[3];
    float y[3] = { 0.0f, 1.0f, 0.0f };
    float z[3] = {
        eye[0] - target[0],
        eye[1] - target[1],
        eye[2] - target[2]
    };
    normalize3(z);
    normalize3(y);
    cross3(y, z, x);
    if (length3(x) < 1e-6f) {
        y[0] = 0.0f;
        y[1] = 0.0f;
        y[2] = 1.0f;
        cross3(y, z, x);
    }
    normalize3(x);
    cross3(z, x, y);
    normalize3(y);

    out[0] = x[0]; out[1] = y[0]; out[2] = z[0]; out[3] = 0.0f;
    out[4] = x[1]; out[5] = y[1]; out[6] = z[1]; out[7] = 0.0f;
    out[8] = x[2]; out[9] = y[2]; out[10] = z[2]; out[11] = 0.0f;
    out[12] = -dot3(x, eye);
    out[13] = -dot3(y, eye);
    out[14] = -dot3(z, eye);
    out[15] = 1.0f;
}

float clampf(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

int clampi(int value, int minValue, int maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

float computeOrthoScaleForCamera(const float cameraPos[3], const float cameraTarget[3], float fovDegrees) {
    const float dx = cameraPos[0] - cameraTarget[0];
    const float dy = cameraPos[1] - cameraTarget[1];
    const float dz = cameraPos[2] - cameraTarget[2];
    const float cameraDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
    const float safeFovDegrees = clampf(fovDegrees, 20.0f, 120.0f);
    const float halfFovRadians = safeFovDegrees * (kPi / 180.0f) * 0.5f;
    return std::max(0.05f, cameraDistance * std::tan(halfFovRadians));
}

void buildProjectionMatrix(float fovYRadians, float aspect, float nearPlane, float farPlane,
                           int projectionMode, float orthoScale, float out[16]) {
    std::fill(out, out + 16, 0.0f);
    const float safeAspect = std::max(aspect, 1e-5f);

    if (projectionMode == CAMERA_PROJECTION_ORTHOGRAPHIC) {
        const float safeOrthoScale = std::max(orthoScale, 1e-4f);
        const float right = safeOrthoScale * safeAspect;
        const float top = safeOrthoScale;

        out[0] = 1.0f / right;
        out[5] = 1.0f / top;
        out[10] = -2.0f / (farPlane - nearPlane);
        out[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        out[15] = 1.0f;
        return;
    }

    const float tanHalfFov = std::tan(fovYRadians * 0.5f);
    out[0] = 1.0f / (safeAspect * tanHalfFov);
    out[5] = 1.0f / tanHalfFov;
    out[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    out[11] = -1.0f;
    out[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
}

void composeShapeMatrix(const Shape& shape, float out[16]) {
    float translation[3] = { shape.center[0], shape.center[1], shape.center[2] };
    float rotationDeg[3] = {
        shape.rotation[0] * (180.0f / kPi),
        shape.rotation[1] * (180.0f / kPi),
        shape.rotation[2] * (180.0f / kPi)
    };
    float scale[3] = {
        std::max(0.01f, shape.scale[0]),
        std::max(0.01f, shape.scale[1]),
        std::max(0.01f, shape.scale[2])
    };
    ImGuizmo::RecomposeMatrixFromComponents(translation, rotationDeg, scale, out);
}

void decomposeShapeMatrix(const float matrix[16], Shape& shape) {
    float translation[3];
    float rotationDeg[3];
    float scale[3];
    ImGuizmo::DecomposeMatrixToComponents(matrix, translation, rotationDeg, scale);

    shape.center[0] = translation[0];
    shape.center[1] = translation[1];
    shape.center[2] = translation[2];
    shape.rotation[0] = rotationDeg[0] * (kPi / 180.0f);
    shape.rotation[1] = rotationDeg[1] * (kPi / 180.0f);
    shape.rotation[2] = rotationDeg[2] * (kPi / 180.0f);
    shape.scale[0] = std::max(0.01f, std::fabs(scale[0]));
    shape.scale[1] = std::max(0.01f, std::fabs(scale[1]));
    shape.scale[2] = std::max(0.01f, std::fabs(scale[2]));
}

ImGuizmo::OPERATION toGizmoOperation(int op) {
    if (op == 1) return ImGuizmo::ROTATE;
    if (op == 2) return ImGuizmo::SCALE;
    return ImGuizmo::TRANSLATE;
}

ImGuizmo::MODE toGizmoMode(int mode) {
    return mode == 0 ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
}

} // namespace

GUIManager::GUIManager()
    : viewportPos(0.0f, 0.0f), viewportSize(1.0f, 1.0f) { }
GUIManager::~GUIManager() { }

ImVec2 GUIManager::getViewportPos() const {
    return viewportPos;
}

ImVec2 GUIManager::getViewportSize() const {
    return viewportSize;
}

void GUIManager::newFrame() {
    ImGuiLayer::NewFrame();
}

void GUIManager::renderGUI(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes,
                           float lightDir[3], float lightColor[3], float ambientColor[3], float backgroundColor[3],
                           float& ambientIntensity, float& directLightIntensity,
                           bool& useGradientBackground,
                           bool& showGUI, bool& altRenderMode, bool& usePathTracer,
                           int& editingShapeIndex, char (&renameBuffer)[128],
                           bool& showHelpPopup, ImVec2& helpButtonPos,
                           bool& showConsole, TransformationState& transformState,
                           const float cameraPos[3], const float cameraTarget[3],
                           RenderSettings& renderSettings,
                           bool denoiserAvailable, bool denoiserUsingGPU,
                           int pathSampleCount)
{
    static bool addShapePopupTriggered = false;
    static ImVec2 addShapePopupPos;
    static int globalScaleMode = SCALE_MODE_ELONGATE;
    static bool showViewportGizmo = true;
    static int gizmoOperation = 0; // 0 translate, 1 rotate, 2 scale
    static int gizmoMode = 0;      // 0 world, 1 local
    static bool elongateScaleDragActive = false;
    static int elongateScaleDragShape = -1;
    static float elongateScaleStartScale[3] = {1.0f, 1.0f, 1.0f};
    static float elongateScaleStartElongation[3] = {0.0f, 0.0f, 0.0f};
    static bool gizmoAxisColorsSwapped = false;
    static bool showExportDialog = false;
    static char exportFilename[512] = "exported_mesh.obj";
    static int exportResolution = 64;
    static float exportBoundingBox = 10.0f;
    static float leftPaneWidthState = -1.0f;
    static float rightPaneWidthState = -1.0f;
    static float leftInspectorHeightState = -1.0f;
    static float rightSceneHeightState = -1.0f;
    static float rightModifierHeightState = -1.0f;
    static int activeLayoutSplitter = 0; // 0 none, 1 left-vertical, 2 right-vertical, 3 left-horizontal, 4 right-scene/modifier, 5 right-modifier/shortcuts
    static float splitterStartMouseX = 0.0f;
    static float splitterStartMouseY = 0.0f;
    static float splitterStartValue = 0.0f;

    renderSettings.cameraFovDegrees = clampf(renderSettings.cameraFovDegrees, 20.0f, 120.0f);
    renderSettings.cameraProjectionMode = clampi(renderSettings.cameraProjectionMode,
                                                 CAMERA_PROJECTION_PERSPECTIVE,
                                                 CAMERA_PROJECTION_ORTHOGRAPHIC);
    renderSettings.pathTracerMaxBounces = clampi(renderSettings.pathTracerMaxBounces, 1, 12);
    renderSettings.denoiseStartSample = clampi(renderSettings.denoiseStartSample, 1, 4096);
    renderSettings.denoiseInterval = clampi(renderSettings.denoiseInterval, 1, 1024);
    ambientIntensity = clampf(ambientIntensity, 0.0f, 8.0f);
    directLightIntensity = clampf(directLightIntensity, 0.0f, 8.0f);

    auto clearMirrorHelperSelection = [&]() {
        transformState.mirrorHelperSelected = false;
        transformState.mirrorHelperShapeIndex = -1;
        transformState.mirrorHelperAxis = -1;
        transformState.mirrorHelperMoveModeActive = false;
        transformState.mirrorHelperMoveConstrained = false;
        transformState.mirrorHelperMoveAxis = -1;
    };

    auto selectMirrorHelperForShape = [&](int shapeIndex, const Shape& shape) {
        if (!shape.mirrorModifierEnabled) {
            return;
        }
        int firstAxis = -1;
        for (int axis = 0; axis < 3; ++axis) {
            if (shape.mirrorAxis[axis]) {
                firstAxis = axis;
                break;
            }
        }
        if (firstAxis < 0) {
            return;
        }

        transformState.mirrorHelperSelected = true;
        transformState.mirrorHelperShapeIndex = shapeIndex;
        transformState.mirrorHelperAxis = firstAxis;
        transformState.mirrorHelperMoveModeActive = false;
        transformState.mirrorHelperMoveConstrained = false;
        transformState.mirrorHelperMoveAxis = -1;
    };

    if (!ImGui::GetIO().WantCaptureKeyboard &&
        glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS &&
        (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS))
    {
        if (!addShapePopupTriggered) {
            addShapePopupTriggered = true;
            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);
            addShapePopupPos = ImVec2((float)mouseX, (float)mouseY);
            ImGui::OpenPopup("Add Shape");
        }
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE) {
        addShapePopupTriggered = false;
    }

    if (!gizmoAxisColorsSwapped) {
        ImGuizmo::Style& gizmoStyle = ImGuizmo::GetStyle();
        std::swap(gizmoStyle.Colors[ImGuizmo::DIRECTION_Y], gizmoStyle.Colors[ImGuizmo::DIRECTION_Z]);
        std::swap(gizmoStyle.Colors[ImGuizmo::PLANE_Y], gizmoStyle.Colors[ImGuizmo::PLANE_Z]);
        gizmoAxisColorsSwapped = true;
    }

    ImGui::SetNextWindowPos(addShapePopupPos);
    if (ImGui::BeginPopup("Add Shape"))
    {
        auto initShapeDefaults = [&](Shape& newShape, int type) {
            newShape.type = type;
            newShape.center[0] = newShape.center[1] = newShape.center[2] = 0.0f;
            newShape.param[0] = newShape.param[1] = newShape.param[2] = 0.0f;
            newShape.rotation[0] = newShape.rotation[1] = newShape.rotation[2] = 0.0f;
            newShape.scale[0] = newShape.scale[1] = newShape.scale[2] = 1.0f;
            newShape.extra = 0.0f;
            newShape.scaleMode = globalScaleMode;
            newShape.elongation[0] = newShape.elongation[1] = newShape.elongation[2] = 0.0f;
            newShape.twist[0] = newShape.twist[1] = newShape.twist[2] = 0.0f;
            newShape.bend[0] = newShape.bend[1] = newShape.bend[2] = 0.0f;
            newShape.mirrorModifierEnabled = false;
            newShape.mirrorAxis[0] = false;
            newShape.mirrorAxis[1] = false;
            newShape.mirrorAxis[2] = false;
            newShape.mirrorOffset[0] = 0.0f;
            newShape.mirrorOffset[1] = 0.0f;
            newShape.mirrorOffset[2] = 0.0f;
            newShape.mirrorSmoothness = 0.0f;
            newShape.blendOp = BLEND_NONE;
            newShape.smoothness = 0.1f;
            newShape.name = std::to_string(shapes.size());
            newShape.albedo[0] = 1.0f; newShape.albedo[1] = 1.0f; newShape.albedo[2] = 1.0f;
            newShape.metallic = 0.0f;
            newShape.roughness = 0.05f;
            newShape.emission[0] = 0.0f; newShape.emission[1] = 0.0f; newShape.emission[2] = 0.0f;
            newShape.emissionStrength = 0.0f;
            newShape.transmission = 0.0f;
            newShape.ior = 1.5f;
        };

        if (ImGui::Selectable("Sphere"))
        {
            Shape newShape;
            initShapeDefaults(newShape, SHAPE_SPHERE);
            newShape.param[0] = 0.5f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("Box"))
        {
            Shape newShape;
            initShapeDefaults(newShape, SHAPE_BOX);
            newShape.param[0] = newShape.param[1] = newShape.param[2] = 0.5f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("Torus"))
        {
            Shape newShape;
            initShapeDefaults(newShape, SHAPE_TORUS);
            newShape.param[0] = 0.5f;
            newShape.param[1] = 0.2f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("Cylinder"))
        {
            Shape newShape;
            initShapeDefaults(newShape, SHAPE_CYLINDER);
            newShape.param[0] = 0.5f;
            newShape.param[1] = 0.5f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("Cone"))
        {
            Shape newShape;
            initShapeDefaults(newShape, SHAPE_CONE);
            newShape.param[0] = 0.5f;
            newShape.param[1] = 0.6f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("Mandelbulb"))
        {
            Shape newShape;
            initShapeDefaults(newShape, SHAPE_MANDELBULB);
            newShape.param[0] = 8.0f;   // Power
            newShape.param[1] = 10.0f;  // Iterations
            newShape.param[2] = 4.0f;   // Bailout
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Keep scale mode global across all shapes.
    for (Shape& shape : shapes) {
        shape.scaleMode = globalScaleMode;
    }

    int winWidth = 1;
    int winHeight = 1;
    glfwGetWindowSize(window, &winWidth, &winHeight);

    const float pad = 8.0f;
    const float toolbarHeight = 46.0f;
    const float defaultLeftPaneWidth = std::min(std::max(winWidth * 0.20f, 220.0f), 360.0f);
    const float defaultRightPaneWidth = std::min(std::max(winWidth * 0.18f, 210.0f), 340.0f);
    const float bottomPaneHeight = showConsole ? std::min(std::max(winHeight * 0.24f, 160.0f), 320.0f) : 0.0f;

    const float contentTop = toolbarHeight + pad;
    const float contentBottom = static_cast<float>(winHeight) - pad - bottomPaneHeight;
    const float contentHeight = std::max(120.0f, contentBottom - contentTop);
    const float minLeftPaneWidth = 220.0f;
    const float minRightPaneWidth = 210.0f;
    const float minSectionHeight = 80.0f;
    const float minCenterWidth = std::max(120.0f, std::min(320.0f, static_cast<float>(winWidth) - 4.0f * pad - minLeftPaneWidth - minRightPaneWidth));

    if (leftPaneWidthState < 0.0f) leftPaneWidthState = defaultLeftPaneWidth;
    if (rightPaneWidthState < 0.0f) rightPaneWidthState = defaultRightPaneWidth;
    if (leftInspectorHeightState < 0.0f) leftInspectorHeightState = contentHeight * 0.66f;
    if (rightSceneHeightState < 0.0f) rightSceneHeightState = contentHeight * 0.45f;
    if (rightModifierHeightState < 0.0f) rightModifierHeightState = contentHeight * 0.30f;

    auto clampColumnWidths = [&]() {
        auto clampf = [](float v, float lo, float hi) {
            return std::max(lo, std::min(v, hi));
        };
        const float maxLeft = std::max(minLeftPaneWidth, static_cast<float>(winWidth) - 4.0f * pad - rightPaneWidthState - minCenterWidth);
        leftPaneWidthState = clampf(leftPaneWidthState, minLeftPaneWidth, maxLeft);

        const float maxRight = std::max(minRightPaneWidth, static_cast<float>(winWidth) - 4.0f * pad - leftPaneWidthState - minCenterWidth);
        rightPaneWidthState = clampf(rightPaneWidthState, minRightPaneWidth, maxRight);

        const float maxLeftAfterRight = std::max(minLeftPaneWidth, static_cast<float>(winWidth) - 4.0f * pad - rightPaneWidthState - minCenterWidth);
        leftPaneWidthState = clampf(leftPaneWidthState, minLeftPaneWidth, maxLeftAfterRight);
    };

    auto clampRowHeights = [&]() {
        auto clampf = [](float v, float lo, float hi) {
            return std::max(lo, std::min(v, hi));
        };
        const float maxSectionHeight = std::max(minSectionHeight, contentHeight - pad - minSectionHeight);
        leftInspectorHeightState = clampf(leftInspectorHeightState, minSectionHeight, maxSectionHeight);
        if (showHelpPopup) {
            const float maxRightScene = std::max(minSectionHeight, contentHeight - 2.0f * pad - 2.0f * minSectionHeight);
            rightSceneHeightState = clampf(rightSceneHeightState, minSectionHeight, maxRightScene);

            const float maxRightModifier = std::max(minSectionHeight, contentHeight - 2.0f * pad - rightSceneHeightState - minSectionHeight);
            rightModifierHeightState = clampf(rightModifierHeightState, minSectionHeight, maxRightModifier);
        } else {
            const float maxRightScene = std::max(minSectionHeight, contentHeight - pad - minSectionHeight);
            rightSceneHeightState = clampf(rightSceneHeightState, minSectionHeight, maxRightScene);

            const float maxRightModifier = std::max(minSectionHeight, contentHeight - pad - rightSceneHeightState);
            rightModifierHeightState = clampf(rightModifierHeightState, minSectionHeight, maxRightModifier);
        }
    };

    clampColumnWidths();
    clampRowHeights();

    float leftPaneWidth = leftPaneWidthState;
    float rightPaneWidth = rightPaneWidthState;
    float leftInspectorH = leftInspectorHeightState;
    float rightSceneH = rightSceneHeightState;
    float rightModifierH = 0.0f;
    float rightShortcutsH = 0.0f;

    float leftX = pad;
    float leftY = contentTop;
    float leftLightingY = leftY + leftInspectorH + pad;
    float leftLightingH = std::max(80.0f, contentHeight - leftInspectorH - pad);

    float rightX = static_cast<float>(winWidth) - pad - rightPaneWidth;
    float rightY = contentTop;
    float rightModifierY = rightY + rightSceneH + pad;
    float rightShortcutsY = rightModifierY;

    if (showHelpPopup) {
        rightModifierH = rightModifierHeightState;
        rightShortcutsY = rightModifierY + rightModifierH + pad;
        rightShortcutsH = std::max(minSectionHeight, contentHeight - rightSceneH - rightModifierH - 2.0f * pad);
    } else {
        rightModifierH = std::max(minSectionHeight, contentHeight - rightSceneH - pad);
    }

    float centerX = leftX + leftPaneWidth + pad;
    float centerY = contentTop;
    float centerW = std::max(120.0f, rightX - pad - centerX);
    float centerH = contentHeight;

    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    auto pointInRect = [](const ImVec2& p, float minX, float minY, float maxX, float maxY) {
        return p.x >= minX && p.x <= maxX && p.y >= minY && p.y <= maxY;
    };

    bool hoverLeftVerticalSplitter = pointInRect(mouse, leftX + leftPaneWidth, contentTop, leftX + leftPaneWidth + pad, contentBottom);
    bool hoverRightVerticalSplitter = pointInRect(mouse, rightX - pad, contentTop, rightX, contentBottom);
    bool hoverLeftHorizontalSplitter = pointInRect(mouse, leftX, leftY + leftInspectorH, leftX + leftPaneWidth, leftY + leftInspectorH + pad);
    bool hoverRightSceneModifierSplitter = pointInRect(mouse, rightX, rightY + rightSceneH, rightX + rightPaneWidth, rightY + rightSceneH + pad);
    bool hoverRightModifierShortcutsSplitter = showHelpPopup && pointInRect(mouse, rightX, rightModifierY + rightModifierH, rightX + rightPaneWidth, rightModifierY + rightModifierH + pad);

    if (activeLayoutSplitter == 0 && mouseClicked) {
        if (hoverLeftVerticalSplitter) {
            activeLayoutSplitter = 1;
            splitterStartMouseX = mouse.x;
            splitterStartValue = leftPaneWidthState;
        } else if (hoverRightVerticalSplitter) {
            activeLayoutSplitter = 2;
            splitterStartMouseX = mouse.x;
            splitterStartValue = rightPaneWidthState;
        } else if (hoverLeftHorizontalSplitter) {
            activeLayoutSplitter = 3;
            splitterStartMouseY = mouse.y;
            splitterStartValue = leftInspectorHeightState;
        } else if (hoverRightSceneModifierSplitter) {
            activeLayoutSplitter = 4;
            splitterStartMouseY = mouse.y;
            splitterStartValue = rightSceneHeightState;
        } else if (hoverRightModifierShortcutsSplitter) {
            activeLayoutSplitter = 5;
            splitterStartMouseY = mouse.y;
            splitterStartValue = rightModifierHeightState;
        }
    }

    if (activeLayoutSplitter != 0 && mouseDown) {
        if (activeLayoutSplitter == 1) {
            leftPaneWidthState = splitterStartValue + (mouse.x - splitterStartMouseX);
        } else if (activeLayoutSplitter == 2) {
            rightPaneWidthState = splitterStartValue - (mouse.x - splitterStartMouseX);
        } else if (activeLayoutSplitter == 3) {
            leftInspectorHeightState = splitterStartValue + (mouse.y - splitterStartMouseY);
        } else if (activeLayoutSplitter == 4) {
            rightSceneHeightState = splitterStartValue + (mouse.y - splitterStartMouseY);
        } else if (activeLayoutSplitter == 5) {
            rightModifierHeightState = splitterStartValue + (mouse.y - splitterStartMouseY);
        }
        clampColumnWidths();
        clampRowHeights();
    }
    if (activeLayoutSplitter != 0 && !mouseDown) {
        activeLayoutSplitter = 0;
    }

    const bool onVerticalSplitter = hoverLeftVerticalSplitter || hoverRightVerticalSplitter || activeLayoutSplitter == 1 || activeLayoutSplitter == 2;
    const bool onHorizontalSplitter =
        hoverLeftHorizontalSplitter || hoverRightSceneModifierSplitter || hoverRightModifierShortcutsSplitter ||
        activeLayoutSplitter == 3 || activeLayoutSplitter == 4 || activeLayoutSplitter == 5;
    if (onVerticalSplitter) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    } else if (onHorizontalSplitter) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }

    leftPaneWidth = leftPaneWidthState;
    rightPaneWidth = rightPaneWidthState;
    leftInspectorH = leftInspectorHeightState;
    rightSceneH = rightSceneHeightState;

    leftX = pad;
    leftY = contentTop;
    leftLightingY = leftY + leftInspectorH + pad;
    leftLightingH = std::max(80.0f, contentHeight - leftInspectorH - pad);

    rightX = static_cast<float>(winWidth) - pad - rightPaneWidth;
    rightY = contentTop;
    rightModifierY = rightY + rightSceneH + pad;
    rightShortcutsY = rightModifierY;
    if (showHelpPopup) {
        rightModifierH = rightModifierHeightState;
        rightShortcutsY = rightModifierY + rightModifierH + pad;
        rightShortcutsH = std::max(minSectionHeight, contentHeight - rightSceneH - rightModifierH - 2.0f * pad);
    } else {
        rightModifierH = std::max(minSectionHeight, contentHeight - rightSceneH - pad);
        rightShortcutsH = 0.0f;
    }

    centerX = leftX + leftPaneWidth + pad;
    centerY = contentTop;
    centerW = std::max(120.0f, rightX - pad - centerX);
    centerH = contentHeight;

    ImDrawList* splitterDrawList = ImGui::GetForegroundDrawList();
    const ImU32 splitterColor = IM_COL32(120, 120, 120, 170);
    splitterDrawList->AddLine(ImVec2(leftX + leftPaneWidth + pad * 0.5f, contentTop), ImVec2(leftX + leftPaneWidth + pad * 0.5f, contentBottom), splitterColor, 1.0f);
    splitterDrawList->AddLine(ImVec2(rightX - pad * 0.5f, contentTop), ImVec2(rightX - pad * 0.5f, contentBottom), splitterColor, 1.0f);
    splitterDrawList->AddLine(ImVec2(leftX, leftY + leftInspectorH + pad * 0.5f), ImVec2(leftX + leftPaneWidth, leftY + leftInspectorH + pad * 0.5f), splitterColor, 1.0f);
    splitterDrawList->AddLine(ImVec2(rightX, rightY + rightSceneH + pad * 0.5f), ImVec2(rightX + rightPaneWidth, rightY + rightSceneH + pad * 0.5f), splitterColor, 1.0f);
    if (showHelpPopup) {
        splitterDrawList->AddLine(ImVec2(rightX, rightModifierY + rightModifierH + pad * 0.5f), ImVec2(rightX + rightPaneWidth, rightModifierY + rightModifierH + pad * 0.5f), splitterColor, 1.0f);
    }

    ImDrawList* viewportDrawList = nullptr;

    viewportPos = ImVec2(centerX, centerY);
    viewportSize = ImVec2(centerW, centerH);
    helpButtonPos = viewportPos;

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(winWidth), toolbarHeight), ImGuiCond_Always);
    ImGui::Begin("Workspace Tools", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
    if (ImGui::Button("Export OBJ")) {
        if (!shapes.empty()) {
            showExportDialog = true;
        }
    }
    ImGui::SameLine();
    const char* globalModeLabel = (globalScaleMode == SCALE_MODE_ELONGATE) ? "Mode: Elongate" : "Mode: Deform";
    if (ImGui::Button(globalModeLabel)) {
        globalScaleMode = (globalScaleMode == SCALE_MODE_ELONGATE) ? SCALE_MODE_DEFORM : SCALE_MODE_ELONGATE;
        for (Shape& shape : shapes) {
            shape.scaleMode = globalScaleMode;
        }
    }
    ImGui::SameLine();
    const char* rendererLabel = usePathTracer ? "Renderer: Path Tracer" : "Renderer: Ray March";
    if (ImGui::Button(rendererLabel)) {
        usePathTracer = !usePathTracer;
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Hide Overlays", &altRenderMode)) {}
    ImGui::SameLine();
    if (ImGui::Checkbox("Console", &showConsole)) {}
    ImGui::SameLine();
    if (ImGui::Checkbox("Shortcuts", &showHelpPopup)) {}
    ImGui::SameLine();
    ImGui::Checkbox("Gizmo", &showViewportGizmo);
    ImGui::SameLine();
    ImGui::TextUnformatted("Op:");
    ImGui::SameLine();
    const char* gizmoOps[] = { "Move", "Rotate", "Scale" };
    ImGui::SetNextItemWidth(85.0f);
    ImGui::Combo("##GizmoOperationTopBar", &gizmoOperation, gizmoOps, IM_ARRAYSIZE(gizmoOps));
    ImGui::SameLine();
    ImGui::TextUnformatted("Space:");
    ImGui::SameLine();
    const char* gizmoModes[] = { "World", "Local" };
    ImGui::SetNextItemWidth(80.0f);
    ImGui::Combo("##GizmoModeTopBar", &gizmoMode, gizmoModes, IM_ARRAYSIZE(gizmoModes));
    transformState.useLocalSpace = (gizmoMode == 1);
    ImGui::SameLine();
    if (ImGui::Button("Hide UI (H)")) {
        showGUI = false;
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(centerX, centerY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(centerW, centerH), ImGuiCond_Always);

    ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);
    {
        viewportDrawList = ImGui::GetWindowDrawList();
        ImVec2 contentStart = ImGui::GetCursorScreenPos();
        ImVec2 contentSize = ImGui::GetContentRegionAvail();
        contentSize.x = std::max(contentSize.x, 1.0f);
        contentSize.y = std::max(contentSize.y, 1.0f);
        viewportPos = contentStart;
        viewportSize = contentSize;
        helpButtonPos = contentStart;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 viewportMax(contentStart.x + contentSize.x, contentStart.y + contentSize.y);
        drawList->AddRect(contentStart, viewportMax, IM_COL32(120, 120, 120, 180));
        drawList->AddText(ImVec2(contentStart.x + 10.0f, contentStart.y + 8.0f), IM_COL32(220, 220, 220, 200), "Viewport");
        drawList->AddText(ImVec2(contentStart.x + 10.0f, contentStart.y + 26.0f), IM_COL32(190, 190, 190, 190), "LMB orbit | Shift + LMB pan | Shift + A add shape");
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(rightX, rightY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rightPaneWidth, rightSceneH), ImGuiCond_Always);
    ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::Text("Shapes: %d", static_cast<int>(shapes.size()));
    ImGui::Separator();
    for (size_t i = 0; i < shapes.size(); i++)
    {
        if (i > 0) ImGui::Separator();

        ImVec4 iconColor;
        switch (shapes[i].type) {
            case SHAPE_SPHERE:   iconColor = ImVec4(0.2f, 0.6f, 0.9f, 1.0f); break;
            case SHAPE_BOX:      iconColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f); break;
            case SHAPE_TORUS:    iconColor = ImVec4(0.9f, 0.9f, 0.3f, 1.0f); break;
            case SHAPE_CYLINDER: iconColor = ImVec4(0.7f, 0.3f, 0.9f, 1.0f); break;
            case SHAPE_CONE:     iconColor = ImVec4(0.3f, 0.9f, 0.7f, 1.0f); break;
            case SHAPE_MANDELBULB: iconColor = ImVec4(0.95f, 0.55f, 0.2f, 1.0f); break;
            default:             iconColor = ImVec4(1,1,1,1); break;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
        ImGui::Bullet();
        ImGui::PopStyleColor();

        if (shapes[i].blendOp != BLEND_NONE) {
            ImGui::SameLine();
            ImVec4 blendColor;
            switch (shapes[i].blendOp) {
                case BLEND_SMOOTH_UNION:        blendColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); break;
                case BLEND_SMOOTH_SUBTRACTION:  blendColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break;
                case BLEND_SMOOTH_INTERSECTION: blendColor = ImVec4(0.0f, 0.0f, 1.0f, 1.0f); break;
                default:                        blendColor = ImVec4(1,1,1,1); break;
            }
            ImGui::PushStyleColor(ImGuiCol_Text, blendColor);
            ImGui::Bullet();
            ImGui::PopStyleColor();
        }

        std::string shapeTypeName;
        switch (shapes[i].type) {
            case SHAPE_SPHERE:   shapeTypeName = "Sphere "; break;
            case SHAPE_BOX:      shapeTypeName = "Box "; break;
            case SHAPE_TORUS:    shapeTypeName = "Torus "; break;
            case SHAPE_CYLINDER: shapeTypeName = "Cylinder "; break;
            case SHAPE_CONE:     shapeTypeName = "Cone "; break;
            case SHAPE_MANDELBULB: shapeTypeName = "Mandelbulb "; break;
            default:             shapeTypeName = "Unknown "; break;
        }

        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (std::find(selectedShapes.begin(), selectedShapes.end(), i) != selectedShapes.end())
            node_flags |= ImGuiTreeNodeFlags_Selected;

        if (editingShapeIndex == i) {
            ImGui::SameLine();
            char tempBuffer[128];
            strncpy(tempBuffer, renameBuffer, sizeof(tempBuffer));
            if (ImGui::InputText("##rename", tempBuffer, sizeof(tempBuffer),
                ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                shapes[i].name = tempBuffer;
                editingShapeIndex = -1;
            }
            if (!ImGui::IsItemActive() && (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1))) {
                editingShapeIndex = -1;
            }
            ImGui::TreeNodeEx((shapeTypeName + shapes[i].name).c_str(), node_flags);
        } else {
            ImGui::TreeNodeEx((shapeTypeName + shapes[i].name).c_str(), node_flags);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDoubleClicked(0)) {
                editingShapeIndex = i;
                strncpy(renameBuffer, shapes[i].name.c_str(), sizeof(renameBuffer));
                renameBuffer[sizeof(renameBuffer)-1] = '\0';
            }
        }
        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("DND_SHAPE", &i, sizeof(size_t));
            ImGui::Text("Moving %s", (shapeTypeName + shapes[i].name).c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_SHAPE"))
            {
                size_t srcIndex = *(const size_t*)payload->Data;
                if (srcIndex != i)
                {
                    Shape draggedShape = shapes[srcIndex];
                    shapes.erase(shapes.begin() + srcIndex);
                    shapes.insert(shapes.begin() + i, draggedShape);
                }
            }
            ImGui::EndDragDropTarget();
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            editingShapeIndex = i;
            strncpy(renameBuffer, shapes[i].name.c_str(), sizeof(renameBuffer));
            renameBuffer[sizeof(renameBuffer)-1] = '\0';
        }
        if (ImGui::IsItemClicked())
        {
            if (ImGui::GetIO().KeyShift)
            {
                auto it = std::find(selectedShapes.begin(), selectedShapes.end(), i);
                if (it != selectedShapes.end())
                    selectedShapes.erase(it);
                else
                    selectedShapes.push_back(i);
            }
            else
            {
                selectedShapes.clear();
                selectedShapes.push_back(i);
            }
            clearMirrorHelperSelection();
        }
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(rightX, rightModifierY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rightPaneWidth, rightModifierH), ImGuiCond_Always);
    ImGui::Begin("Modifier", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    if (ImGui::BeginTabBar("ModifierTabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Modifiers")) {
            if (!selectedShapes.empty()) {
                int selIdx = selectedShapes[0];
                if (selIdx >= 0 && selIdx < static_cast<int>(shapes.size())) {
                    Shape& selectedShape = shapes[selIdx];

                    int addModifierSelection = 0;
                    const char* modifierItems[] = { "Add modifier...", "Bend", "Twist", "Mirror" };
                    if (ImGui::Combo("##ModifierAddSelector", &addModifierSelection, modifierItems, IM_ARRAYSIZE(modifierItems))) {
                        if (addModifierSelection == 1) {
                            selectedShape.bendModifierEnabled = true;
                        } else if (addModifierSelection == 2) {
                            selectedShape.twistModifierEnabled = true;
                        } else if (addModifierSelection == 3) {
                            selectedShape.mirrorModifierEnabled = true;
                            if (!selectedShape.mirrorAxis[0] && !selectedShape.mirrorAxis[1] && !selectedShape.mirrorAxis[2]) {
                                selectedShape.mirrorAxis[0] = true;
                            }
                            selectedShapes.clear();
                            selectedShapes.push_back(selIdx);
                            selectMirrorHelperForShape(selIdx, selectedShape);
                        }
                    }

                    bool hasAnyModifier = false;

                    if (selectedShape.bendModifierEnabled) {
                        hasAnyModifier = true;
                        ImGui::Separator();
                        ImGui::TextUnformatted("Bend");
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove##ModifierBend")) {
                            selectedShape.bendModifierEnabled = false;
                            selectedShape.bend[0] = 0.0f;
                            selectedShape.bend[1] = 0.0f;
                            selectedShape.bend[2] = 0.0f;
                        }
                        if (selectedShape.bendModifierEnabled) {
                            ImGui::DragFloat3("Amount (XYZ)##ModifierBendAmount", selectedShape.bend, 0.01f, -20.0f, 20.0f);
                        }
                    }

                    if (selectedShape.twistModifierEnabled) {
                        hasAnyModifier = true;
                        ImGui::Separator();
                        ImGui::TextUnformatted("Twist");
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove##ModifierTwist")) {
                            selectedShape.twistModifierEnabled = false;
                            selectedShape.twist[0] = 0.0f;
                            selectedShape.twist[1] = 0.0f;
                            selectedShape.twist[2] = 0.0f;
                        }
                        if (selectedShape.twistModifierEnabled) {
                            ImGui::DragFloat3("Amount (XYZ)##ModifierTwistAmount", selectedShape.twist, 0.01f, -20.0f, 20.0f);
                        }
                    }

                    if (selectedShape.mirrorModifierEnabled) {
                        hasAnyModifier = true;
                        ImGui::Separator();
                        ImGui::TextUnformatted("Mirror");
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove##ModifierMirror")) {
                            selectedShape.mirrorModifierEnabled = false;
                            selectedShape.mirrorAxis[0] = false;
                            selectedShape.mirrorAxis[1] = false;
                            selectedShape.mirrorAxis[2] = false;
                            selectedShape.mirrorSmoothness = 0.0f;
                            clearMirrorHelperSelection();
                        }
                        if (selectedShape.mirrorModifierEnabled) {
                            const bool hadAnyAxis = selectedShape.mirrorAxis[0] ||
                                                    selectedShape.mirrorAxis[1] ||
                                                    selectedShape.mirrorAxis[2];
                            ImGui::Checkbox("X##ModifierMirrorAxisX", &selectedShape.mirrorAxis[0]);
                            ImGui::SameLine();
                            ImGui::Checkbox("Y##ModifierMirrorAxisY", &selectedShape.mirrorAxis[1]);
                            ImGui::SameLine();
                            ImGui::Checkbox("Z##ModifierMirrorAxisZ", &selectedShape.mirrorAxis[2]);
                            ImGui::DragFloat("Smoothness##ModifierMirrorSmoothness",
                                             &selectedShape.mirrorSmoothness,
                                             0.005f, 0.0f, 2.0f);
                            const bool hasAnyAxis = selectedShape.mirrorAxis[0] ||
                                                    selectedShape.mirrorAxis[1] ||
                                                    selectedShape.mirrorAxis[2];
                            if (!hadAnyAxis && hasAnyAxis) {
                                selectMirrorHelperForShape(selIdx, selectedShape);
                            }
                            if (!selectedShape.mirrorAxis[0] &&
                                !selectedShape.mirrorAxis[1] &&
                                !selectedShape.mirrorAxis[2] &&
                                transformState.mirrorHelperSelected &&
                                transformState.mirrorHelperShapeIndex == selIdx) {
                                clearMirrorHelperSelection();
                            }
                        }
                    }

                    if (!hasAnyModifier) {
                        ImGui::Spacing();
                        ImGui::TextDisabled("No active modifiers.");
                    }
                } else {
                    selectedShapes.clear();
                    clearMirrorHelperSelection();
                    ImGui::TextDisabled("Select a shape in the Scene panel.");
                }
            } else {
                ImGui::TextDisabled("Select a shape in the Scene panel.");
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Image")) {
            ImGui::TextUnformatted("Camera");
            ImGui::SliderFloat("FOV (Degrees)##ImageParamsFov",
                               &renderSettings.cameraFovDegrees, 20.0f, 120.0f, "%.1f");

            const char* projectionItems[] = { "Perspective", "Orthographic" };
            ImGui::Combo("Projection##ImageParamsProjection",
                         &renderSettings.cameraProjectionMode,
                         projectionItems, IM_ARRAYSIZE(projectionItems));

            const float cameraDx = cameraPos[0] - cameraTarget[0];
            const float cameraDy = cameraPos[1] - cameraTarget[1];
            const float cameraDz = cameraPos[2] - cameraTarget[2];
            const float cameraDistance = std::sqrt(cameraDx * cameraDx + cameraDy * cameraDy + cameraDz * cameraDz);
            const float orthoScale = computeOrthoScaleForCamera(cameraPos, cameraTarget, renderSettings.cameraFovDegrees);
            if (renderSettings.cameraProjectionMode == CAMERA_PROJECTION_ORTHOGRAPHIC) {
                ImGui::Text("Orthographic Size: %.3f", orthoScale);
            }
            ImGui::Text("Camera Distance: %.3f", cameraDistance);

            ImGui::Separator();
            ImGui::TextUnformatted("Path Tracer");
            ImGui::SliderInt("Max Bounces##ImageParamsPathBounces", &renderSettings.pathTracerMaxBounces, 1, 12);
            ImGui::Text("Accumulated Samples: %d", std::max(pathSampleCount, 0));

            ImGui::Separator();
            ImGui::TextUnformatted("Denoiser");
            if (denoiserAvailable) {
                ImGui::Text("Status: Available (%s)", denoiserUsingGPU ? "GPU" : "CPU");
            } else {
                ImGui::TextDisabled("Status: Unavailable");
            }

            if (!denoiserAvailable) {
                ImGui::BeginDisabled();
            }
            ImGui::Checkbox("Enable Denoiser##ImageParamsDenoiserEnabled", &renderSettings.denoiserEnabled);
            if (renderSettings.denoiserEnabled) {
                ImGui::SliderInt("Start Sample##ImageParamsDenoiseStart",
                                 &renderSettings.denoiseStartSample, 1, 256);
                ImGui::SliderInt("Interval##ImageParamsDenoiseInterval",
                                 &renderSettings.denoiseInterval, 1, 64);
            }
            if (!denoiserAvailable) {
                ImGui::EndDisabled();
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(leftX, leftY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(leftPaneWidth, leftInspectorH), ImGuiCond_Always);
    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    if (!selectedShapes.empty()) {
        int selIdx = selectedShapes[0];
        if (selIdx >= 0 && selIdx < static_cast<int>(shapes.size())) {
            Shape &selectedShape = shapes[selIdx];
            if (ImGui::BeginTabBar("ShapeParametersTabs", ImGuiTabBarFlags_None))
            {
                if (ImGui::BeginTabItem("Transform"))
                {
                    ImGui::DragFloat3("Center", selectedShape.center, 0.01f);
                    switch(selectedShape.type)
                    {
                        case SHAPE_SPHERE:
                            ImGui::DragFloat("Radius", &selectedShape.param[0], 0.01f);
                            selectedShape.param[0] = std::max(selectedShape.param[0], 0.001f);
                            break;
                        case SHAPE_BOX:
                            ImGui::DragFloat3("Half Extents", selectedShape.param, 0.01f);
                            selectedShape.param[0] = std::max(selectedShape.param[0], 0.001f);
                            selectedShape.param[1] = std::max(selectedShape.param[1], 0.001f);
                            selectedShape.param[2] = std::max(selectedShape.param[2], 0.001f);
                            break;
                        case SHAPE_TORUS:
                            ImGui::DragFloat("Major Radius", &selectedShape.param[0], 0.01f);
                            ImGui::DragFloat("Minor Radius", &selectedShape.param[1], 0.01f);
                            selectedShape.param[0] = std::max(selectedShape.param[0], 0.001f);
                            selectedShape.param[1] = std::max(selectedShape.param[1], 0.001f);
                            break;
                        case SHAPE_CYLINDER:
                            ImGui::DragFloat("Radius", &selectedShape.param[0], 0.01f);
                            ImGui::DragFloat("Half Height", &selectedShape.param[1], 0.01f);
                            selectedShape.param[0] = std::max(selectedShape.param[0], 0.001f);
                            selectedShape.param[1] = std::max(selectedShape.param[1], 0.001f);
                            break;
                        case SHAPE_CONE:
                            ImGui::DragFloat("Base Radius", &selectedShape.param[0], 0.01f);
                            ImGui::DragFloat("Half Height", &selectedShape.param[1], 0.01f);
                            selectedShape.param[0] = std::max(selectedShape.param[0], 0.001f);
                            selectedShape.param[1] = std::max(selectedShape.param[1], 0.001f);
                            break;
                        case SHAPE_MANDELBULB: {
                            ImGui::DragFloat("Power", &selectedShape.param[0], 0.05f, 2.0f, 16.0f);
                            int iterations = std::max(1, static_cast<int>(selectedShape.param[1] + 0.5f));
                            if (ImGui::SliderInt("Iterations", &iterations, 1, 64)) {
                                selectedShape.param[1] = static_cast<float>(iterations);
                            }
                            ImGui::DragFloat("Bailout", &selectedShape.param[2], 0.05f, 2.0f, 20.0f);
                            selectedShape.param[0] = std::max(selectedShape.param[0], 2.0f);
                            selectedShape.param[1] = std::max(selectedShape.param[1], 1.0f);
                            selectedShape.param[2] = std::max(selectedShape.param[2], 2.0f);
                            break;
                        }
                        default:
                            ImGui::Text("Unknown shape type.");
                            break;
                    }

                    {
                        float rotDeg[3] = { selectedShape.rotation[0] * 57.2958f,
                                            selectedShape.rotation[1] * 57.2958f,
                                            selectedShape.rotation[2] * 57.2958f };
                        if (ImGui::DragFloat3("Rotation", rotDeg, 0.1f))
                        {
                            selectedShape.rotation[0] = rotDeg[0] * 0.0174533f;
                            selectedShape.rotation[1] = rotDeg[1] * 0.0174533f;
                            selectedShape.rotation[2] = rotDeg[2] * 0.0174533f;
                        }
                    }

                    selectedShape.scaleMode = globalScaleMode;
                    ImGui::Text("Scale Mode (Global): %s", globalScaleMode == SCALE_MODE_ELONGATE ? "Elongate" : "Deform");

                    if (globalScaleMode == SCALE_MODE_ELONGATE) {
                        ImGui::DragFloat3("Elongation", selectedShape.elongation, 0.01f, -100.0f, 100.0f);
                    } else {
                        ImGui::DragFloat3("Scale", selectedShape.scale, 0.01f, 0.01f, 100.0f);
                    }

                    selectedShape.scale[0] = std::max(selectedShape.scale[0], 0.01f);
                    selectedShape.scale[1] = std::max(selectedShape.scale[1], 0.01f);
                    selectedShape.scale[2] = std::max(selectedShape.scale[2], 0.01f);

                    ImGui::DragFloat("Rounding", &selectedShape.extra, 0.005f, 0.0f, 2.0f);
                    selectedShape.extra = std::max(selectedShape.extra, 0.0f);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Material"))
                {
                    ImGui::ColorEdit3("Albedo", selectedShape.albedo);
                    ImGui::DragFloat("Metallic", &selectedShape.metallic, 0.01f, 0.0f, 1.0f);
                    ImGui::DragFloat("Roughness", &selectedShape.roughness, 0.01f, 0.0f, 1.0f);
                    ImGui::ColorEdit3("Emission Color", selectedShape.emission);
                    ImGui::DragFloat("Emission Strength", &selectedShape.emissionStrength, 0.05f, 0.0f, 100.0f);
                    ImGui::SliderFloat("Transmission", &selectedShape.transmission, 0.0f, 1.0f);
                    ImGui::DragFloat("IOR", &selectedShape.ior, 0.01f, 1.0f, 2.5f);
                    selectedShape.transmission = std::max(0.0f, std::min(selectedShape.transmission, 1.0f));
                    selectedShape.ior = std::max(selectedShape.ior, 1.0f);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Blend"))
                {
                    const char* blendModeItems[] = { "None", "Smooth Union", "Smooth Subtraction", "Smooth Intersection" };
                    ImGui::Combo("Blend Mode", &selectedShape.blendOp, blendModeItems, IM_ARRAYSIZE(blendModeItems));
                    ImGui::DragFloat("Smoothness", &selectedShape.smoothness, 0.01f, 0.001f, 1.0f);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        } else {
            selectedShapes.clear();
            clearMirrorHelperSelection();
            ImGui::TextDisabled("Select a shape in the Scene panel.");
        }
    } else {
        ImGui::TextDisabled("Select a shape in the Scene panel.");
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(leftX, leftLightingY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(leftPaneWidth, leftLightingH), ImGuiCond_Always);
    ImGui::Begin("Lighting", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    if (ImGui::BeginTabBar("LightingParametersTabs", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("Direct Light"))
        {
            ImGui::PushItemWidth(-1.0f);

            ImGui::TextUnformatted("Light Direction");
            ImGui::DragFloat3("##LightingLightDirection", lightDir, 0.01f);

            ImGui::TextUnformatted("Light Color");
            ImGui::ColorEdit3("##LightingLightColor", lightColor);
            ImGui::SliderFloat("Direct Intensity##LightingDirectIntensity", &directLightIntensity, 0.0f, 8.0f);

            ImGui::TextUnformatted("Ambient Color");
            ImGui::ColorEdit3("##LightingAmbientColor", ambientColor);
            ImGui::SliderFloat("Ambient Intensity##LightingAmbientIntensity", &ambientIntensity, 0.0f, 8.0f);

            ImGui::TextUnformatted("Background Color");
            ImGui::ColorEdit3("##LightingBackgroundColor", backgroundColor);
            // ImGui::Checkbox("Background Gradient##LightingBackgroundGradient", &useGradientBackground);

            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();

    if (showHelpPopup) {
        ImGui::SetNextWindowPos(ImVec2(rightX, rightShortcutsY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(rightPaneWidth, rightShortcutsH), ImGuiCond_Always);
        if (ImGui::Begin("Shortcuts", &showHelpPopup, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Transform Operations:");
            ImGui::BulletText("Shift + D  Duplicate selected shapes");
            ImGui::BulletText("X  Delete selection or constrain to X axis");
            ImGui::BulletText("C  Focus camera on last selected shape");
            ImGui::BulletText("G / R / S  Translate / Rotate / Scale mode");
            ImGui::BulletText("Use gizmo handles directly in viewport");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Constraints:");
            ImGui::BulletText("X / Y / Z during transform constrains axis");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Interface:");
            ImGui::BulletText("H toggles interface visibility");
            ImGui::BulletText("ESC cancels active transform");
            ImGui::BulletText("Shift + Click toggles multi-selection");
        }
        ImGui::End();
    }

    // Export dialog
    if (showExportDialog) {
        ImGui::SetNextWindowPos(ImVec2(winWidth * 0.5f, winHeight * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(520, 430), ImGuiCond_Appearing);
        ImGui::Begin("Export Settings", &showExportDialog, ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Export Mesh to OBJ Format");
        ImGui::Separator();

        // File path section
        ImGui::Text("File Path:");
        ImGui::InputText("##filepath", exportFilename, sizeof(exportFilename));
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Specify full path or just filename\nExample: /Users/yourname/Desktop/mesh.obj\nor just: my_mesh.obj");
        }

        // Quick path buttons
        ImGui::Text("Quick paths:");
        if (ImGui::Button("Desktop", ImVec2(70, 20))) {
            strcpy(exportFilename, "~/Desktop/exported_mesh.obj");
        }
        ImGui::SameLine();
        if (ImGui::Button("Documents", ImVec2(80, 20))) {
            strcpy(exportFilename, "~/Documents/exported_mesh.obj");
        }
        ImGui::SameLine();
        if (ImGui::Button("Current Dir", ImVec2(80, 20))) {
            strcpy(exportFilename, "exported_mesh.obj");
        }

        // Auto-add .obj extension if not present
        std::string currentFilename = std::string(exportFilename);
        bool hasObjExtension = currentFilename.length() >= 4 &&
                              currentFilename.substr(currentFilename.length() - 4) == ".obj";
        if (!hasObjExtension && !currentFilename.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Note: .obj extension will be added automatically");
        }

        ImGui::Spacing();

        // Resolution settings
        ImGui::Text("Resolution (higher = more detail, slower):");
        ImGui::SliderInt("##resolution", &exportResolution, 16, 512);
        ImGui::Text("Grid size: %dx%dx%d (%d voxels)", exportResolution, exportResolution, exportResolution,
                   exportResolution * exportResolution * exportResolution);

        ImGui::Spacing();

        // Bounding box settings
        ImGui::Text("Bounding Box Size:");
        ImGui::SliderFloat("##boundingbox", &exportBoundingBox, 1.0f, 100.0f, "%.1f");
        ImGui::TextDisabled("Size of the export volume around your shapes");

        ImGui::Separator();

        // Quality presets
        ImGui::Text("Quality Presets:");
        ImGui::Columns(2, "presets", false);

        if (ImGui::Button("Draft (16x16x16)", ImVec2(140, 25))) {
            exportResolution = 16;
        }
        if (ImGui::Button("Low (32x32x32)", ImVec2(140, 25))) {
            exportResolution = 32;
        }

        ImGui::NextColumn();

        if (ImGui::Button("Medium (64x64x64)", ImVec2(140, 25))) {
            exportResolution = 64;
        }
        if (ImGui::Button("High (128x128x128)", ImVec2(140, 25))) {
            exportResolution = 128;
        }

        ImGui::Columns(1);

        if (ImGui::Button("Very High (256x256x256)", ImVec2(200, 25))) {
            exportResolution = 256;
        }
        ImGui::SameLine();
        if (ImGui::Button("Ultra (512x512x512)", ImVec2(200, 25))) {
            exportResolution = 512;
        }

        ImGui::Separator();

        // Performance warning
        float estimatedTime = (exportResolution * exportResolution * exportResolution) / 500000.0f;
        long long memoryUsage = (long long)exportResolution * exportResolution * exportResolution * 4;

        if (exportResolution >= 256) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.0f, 1.0f), "WARNING: Very high resolution!");
            ImGui::Text("This may take several minutes and use significant memory.");
        } else if (exportResolution >= 128) {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "Warning: High resolution may take time.");
        }

        ImGui::Text("Estimated time: %.1f seconds", estimatedTime);
        ImGui::Text("Memory usage: %.1f MB", memoryUsage / (1024.0f * 1024.0f));

        ImGui::Separator();

        // Export/Cancel buttons
        bool canExport = strlen(exportFilename) > 0;
        if (!canExport) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
        }

        if (ImGui::Button("Export", ImVec2(120, 35)) && canExport) {
            std::string finalFilename = std::string(exportFilename);

            // Add .obj extension if not present
            if (finalFilename.length() < 4 || finalFilename.substr(finalFilename.length() - 4) != ".obj") {
                finalFilename += ".obj";
            }

            Console::getInstance().addLog("Starting export...", 0);
            Console::getInstance().addLog("File: " + finalFilename, 0);
            Console::getInstance().addLog("Resolution: " + std::to_string(exportResolution) + "x" +
                                        std::to_string(exportResolution) + "x" + std::to_string(exportResolution), 0);
            Console::getInstance().addLog("Bounding box: " + std::to_string(exportBoundingBox), 0);

            // Show console automatically when starting export
            showConsole = true;

            // Set up logging callback
            MeshExporter::setLogCallback([](const std::string& message, int level) {
                Console::getInstance().addLog(message, level);
            });

            bool success = MeshExporter::exportToOBJ(shapes, finalFilename, exportResolution, exportBoundingBox);
            if (success) {
                Console::getInstance().addLog("Mesh exported successfully to: " + finalFilename, 0);
            } else {
                Console::getInstance().addLog("Failed to export mesh!", 2);
            }
            showExportDialog = false;
        }

        if (!canExport) {
            ImGui::PopStyleVar();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 35))) {
            showExportDialog = false;
        }

        if (!canExport) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Please enter a filename to export");
        }

        ImGui::End();
    }

    // Draw transform gizmo on top of the viewport for the first selected shape.
    if (showViewportGizmo && !selectedShapes.empty()) {
        const int selIdx = selectedShapes[0];
        if (selIdx >= 0 && selIdx < static_cast<int>(shapes.size())) {
            if (viewportSize.x > 1.0f && viewportSize.y > 1.0f) {
                float view[16];
                float projection[16];

                buildViewMatrix(cameraPos, cameraTarget, view);
                const float gizmoFovRadians = renderSettings.cameraFovDegrees * (kPi / 180.0f);
                const float gizmoOrthoScale = computeOrthoScaleForCamera(cameraPos, cameraTarget, renderSettings.cameraFovDegrees);
                buildProjectionMatrix(gizmoFovRadians, viewportSize.x / viewportSize.y, 0.01f, 200.0f,
                                      renderSettings.cameraProjectionMode, gizmoOrthoScale, projection);

                ImGuizmo::BeginFrame();
                ImGuizmo::SetOrthographic(renderSettings.cameraProjectionMode == CAMERA_PROJECTION_ORTHOGRAPHIC);
                ImGuizmo::AllowAxisFlip(false);
                ImGuizmo::SetDrawlist(viewportDrawList);
                ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

                const bool helperSelectionActive =
                    transformState.mirrorHelperSelected &&
                    transformState.mirrorHelperShapeIndex == selIdx &&
                    shapes[selIdx].mirrorModifierEnabled &&
                    (shapes[selIdx].mirrorAxis[0] ||
                     shapes[selIdx].mirrorAxis[1] ||
                     shapes[selIdx].mirrorAxis[2]);

                if (helperSelectionActive) {
                    if (elongateScaleDragActive) {
                        elongateScaleDragActive = false;
                        elongateScaleDragShape = -1;
                    }

                    float helperTranslation[3] = {
                        shapes[selIdx].mirrorOffset[0],
                        shapes[selIdx].mirrorOffset[1],
                        shapes[selIdx].mirrorOffset[2]
                    };
                    float helperRotationDeg[3] = {0.0f, 0.0f, 0.0f};
                    float helperScale[3] = {1.0f, 1.0f, 1.0f};
                    float helperModel[16];
                    ImGuizmo::RecomposeMatrixFromComponents(helperTranslation, helperRotationDeg, helperScale, helperModel);

                    if (ImGuizmo::Manipulate(view, projection, ImGuizmo::TRANSLATE, ImGuizmo::WORLD, helperModel)) {
                        float manipulatedTranslation[3];
                        float manipulatedRotationDeg[3];
                        float manipulatedScale[3];
                        ImGuizmo::DecomposeMatrixToComponents(helperModel, manipulatedTranslation, manipulatedRotationDeg, manipulatedScale);
                        const float deltaX = manipulatedTranslation[0] - shapes[selIdx].mirrorOffset[0];
                        const float deltaY = manipulatedTranslation[1] - shapes[selIdx].mirrorOffset[1];
                        const float deltaZ = manipulatedTranslation[2] - shapes[selIdx].mirrorOffset[2];
                        shapes[selIdx].mirrorOffset[0] = manipulatedTranslation[0];
                        shapes[selIdx].mirrorOffset[1] = manipulatedTranslation[1];
                        shapes[selIdx].mirrorOffset[2] = manipulatedTranslation[2];
                        shapes[selIdx].center[0] += deltaX;
                        shapes[selIdx].center[1] += deltaY;
                        shapes[selIdx].center[2] += deltaZ;
                    }
                } else {
                    float model[16];
                    composeShapeMatrix(shapes[selIdx], model);

                    const ImGuizmo::OPERATION operation = toGizmoOperation(gizmoOperation);
                    ImGuizmo::MODE mode = toGizmoMode(gizmoMode);
                    if (operation == ImGuizmo::SCALE) {
                        mode = ImGuizmo::LOCAL;
                    }
                    const bool elongateScaleMode = (operation == ImGuizmo::SCALE && globalScaleMode == SCALE_MODE_ELONGATE);
                    if (!elongateScaleMode && elongateScaleDragActive) {
                        elongateScaleDragActive = false;
                        elongateScaleDragShape = -1;
                    }

                    const bool manipulated = ImGuizmo::Manipulate(view, projection, operation, mode, model);
                    const bool usingGizmo = ImGuizmo::IsUsing();

                    if (elongateScaleMode) {
                        if (usingGizmo && (!elongateScaleDragActive || elongateScaleDragShape != selIdx)) {
                            elongateScaleDragActive = true;
                            elongateScaleDragShape = selIdx;
                            elongateScaleStartScale[0] = std::max(0.01f, shapes[selIdx].scale[0]);
                            elongateScaleStartScale[1] = std::max(0.01f, shapes[selIdx].scale[1]);
                            elongateScaleStartScale[2] = std::max(0.01f, shapes[selIdx].scale[2]);
                            elongateScaleStartElongation[0] = shapes[selIdx].elongation[0];
                            elongateScaleStartElongation[1] = shapes[selIdx].elongation[1];
                            elongateScaleStartElongation[2] = shapes[selIdx].elongation[2];
                        }

                        if (usingGizmo && elongateScaleDragShape == selIdx) {
                            float translation[3];
                            float rotationDeg[3];
                            float manipulatedScale[3];
                            ImGuizmo::DecomposeMatrixToComponents(model, translation, rotationDeg, manipulatedScale);
                            shapes[selIdx].elongation[0] = elongateScaleStartElongation[0] + (manipulatedScale[0] - elongateScaleStartScale[0]);
                            shapes[selIdx].elongation[1] = elongateScaleStartElongation[1] + (manipulatedScale[1] - elongateScaleStartScale[1]);
                            shapes[selIdx].elongation[2] = elongateScaleStartElongation[2] + (manipulatedScale[2] - elongateScaleStartScale[2]);
                        } else if (!usingGizmo && elongateScaleDragActive) {
                            elongateScaleDragActive = false;
                            elongateScaleDragShape = -1;
                        }
                    } else if (manipulated) {
                        decomposeShapeMatrix(model, shapes[selIdx]);
                    }
                }
            }
        }
    }

    // Render console panel
    if (showConsole) {
        const float consoleY = bottomPaneHeight > 0.0f ? (contentBottom + pad) : std::max(pad, static_cast<float>(winHeight) - 180.0f);
        const float consoleH = bottomPaneHeight > 0.0f ? std::max(120.0f, bottomPaneHeight - pad) : 160.0f;
        ImGui::SetNextWindowPos(ImVec2(pad, consoleY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(std::max(200.0f, static_cast<float>(winWidth) - 2.0f * pad), consoleH), ImGuiCond_Always);
        Console::getInstance().render(&showConsole);
    }
}

// Console implementation
Console::Console() {
    startTime = ImGui::GetTime();
}

void Console::addLog(const std::string& message, int level) {
    LogEntry entry;
    entry.message = message;
    entry.timestamp = ImGui::GetTime() - startTime;
    entry.level = level;
    logs.push_back(entry);
    
    // Keep a reasonable number of logs
    if (logs.size() > 1000) {
        logs.erase(logs.begin(), logs.begin() + 100);
    }
}

void Console::clear() {
    logs.clear();
    startTime = ImGui::GetTime();
}

ImVec4 Console::getLogColor(int level) {
    switch (level) {
        case 0: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White for info
        case 1: return ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow for warning
        case 2: return ImVec4(1.0f, 0.4f, 0.4f, 1.0f); // Red for error
        default: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void Console::render(bool* p_open) {
    if (!ImGui::Begin("Export Console", p_open, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }
    
    // Options menu
    if (ImGui::BeginMenuBar()) {
        if (ImGui::Button("Clear")) {
            clear();
        }
        ImGui::Separator();
        ImGui::Checkbox("Auto-scroll", &autoScroll);
        ImGui::Separator();
        
        // Log level filters
        static bool showInfo = true, showWarning = true, showError = true;
        ImGui::Text("Show:");
        ImGui::SameLine();
        if (ImGui::Checkbox("Info", &showInfo)) {}
        ImGui::SameLine();
        if (ImGui::Checkbox("Warnings", &showWarning)) {}
        ImGui::SameLine();
        if (ImGui::Checkbox("Errors", &showError)) {}
        
        ImGui::EndMenuBar();
    }
    
    // Reserve enough left-over height for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
        
        static bool showInfo = true, showWarning = true, showError = true;
        
        for (const auto& entry : logs) {
            // Filter by log level
            if ((entry.level == 0 && !showInfo) || 
                (entry.level == 1 && !showWarning) || 
                (entry.level == 2 && !showError)) {
                continue;
            }
            
            ImVec4 color = getLogColor(entry.level);
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            
            // Format: [timestamp] level_icon message
            const char* icon = entry.level == 0 ? "ℹ️" : (entry.level == 1 ? "⚠️" : "❌");
            ImGui::Text("[%.2fs] %s %s", entry.timestamp, icon, entry.message.c_str());
            
            ImGui::PopStyleColor();
        }
        
        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    
    ImGui::Separator();
    
    // Status line
    int infoCount = 0, warningCount = 0, errorCount = 0;
    for (const auto& entry : logs) {
        if (entry.level == 0) infoCount++;
        else if (entry.level == 1) warningCount++;
        else if (entry.level == 2) errorCount++;
    }
    
    ImGui::Text("Total: %d | Info: %d | Warnings: %d | Errors: %d", 
                (int)logs.size(), infoCount, warningCount, errorCount);
    
    ImGui::End();
}
