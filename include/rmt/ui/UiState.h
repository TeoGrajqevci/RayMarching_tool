#pragma once

#include <cstddef>

#include "imgui.h"

#include "rmt/input/Input.h"
#include "rmt/scene/Shape.h"

namespace rmt {

struct UiRuntimeState {
    bool addShapePopupTriggered;
    ImVec2 addShapePopupPos;
    int globalScaleMode;
    bool showViewportGizmo;
    int gizmoOperation;
    int gizmoMode;
    bool elongateScaleDragActive;
    int elongateScaleDragShape;
    float elongateScaleStartScale[3];
    float elongateScaleStartElongation[3];
    bool curveNodeScaleDragActive;
    int curveNodeScaleDragShape;
    int curveNodeScaleDragNode;
    float curveNodeScaleStartRadius;
    bool gizmoAxisColorsSwapped;
    bool showExportDialog;
    char exportFilename[512];
    int exportResolution;
    float exportBoundingBox;
    float leftPaneWidthState;
    float rightPaneWidthState;
    float leftInspectorHeightState;
    float rightSceneHeightState;
    float rightModifierHeightState;
    int activeLayoutSplitter;
    float splitterStartMouseX;
    float splitterStartMouseY;
    float splitterStartValue;

    UiRuntimeState();
};

struct UiWorkspaceGeometry {
    int winWidth;
    int winHeight;
    float pad;
    float toolbarHeight;
    float bottomPaneHeight;
    float contentTop;
    float contentBottom;
    float contentHeight;

    float leftX;
    float leftY;
    float leftPaneWidth;
    float leftInspectorH;
    float leftLightingY;
    float leftLightingH;

    float rightX;
    float rightY;
    float rightPaneWidth;
    float rightSceneH;
    float rightModifierY;
    float rightModifierH;
    float rightShortcutsY;
    float rightShortcutsH;

    float centerX;
    float centerY;
    float centerW;
    float centerH;

    UiWorkspaceGeometry();
};

float clampUiFloat(float value, float minValue, float maxValue);
int clampUiInt(int value, int minValue, int maxValue);

void clearMirrorHelperSelection(TransformationState& transformState);
void selectMirrorHelperForShape(int shapeIndex, const Shape& shape, TransformationState& transformState);
void initShapeDefaults(Shape& shape, int type, int globalScaleMode, std::size_t shapeCount);

} // namespace rmt
