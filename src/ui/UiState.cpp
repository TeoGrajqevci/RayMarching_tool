#include "rmt/ui/UiState.h"

#include <algorithm>
#include <cstring>

namespace rmt {

UiRuntimeState::UiRuntimeState()
    : addShapePopupTriggered(false),
      addShapePopupPos(0.0f, 0.0f),
      globalScaleMode(SCALE_MODE_ELONGATE),
      showViewportGizmo(true),
      gizmoOperation(0),
      gizmoMode(0),
      elongateScaleDragActive(false),
      elongateScaleDragShape(-1),
      gizmoAxisColorsSwapped(false),
      showExportDialog(false),
      exportResolution(64),
      exportBoundingBox(10.0f),
      leftPaneWidthState(-1.0f),
      rightPaneWidthState(-1.0f),
      leftInspectorHeightState(-1.0f),
      rightSceneHeightState(-1.0f),
      rightModifierHeightState(-1.0f),
      activeLayoutSplitter(0),
      splitterStartMouseX(0.0f),
      splitterStartMouseY(0.0f),
      splitterStartValue(0.0f) {
    elongateScaleStartScale[0] = 1.0f;
    elongateScaleStartScale[1] = 1.0f;
    elongateScaleStartScale[2] = 1.0f;
    elongateScaleStartElongation[0] = 0.0f;
    elongateScaleStartElongation[1] = 0.0f;
    elongateScaleStartElongation[2] = 0.0f;
    std::strcpy(exportFilename, "exported_mesh.obj");
}

UiWorkspaceGeometry::UiWorkspaceGeometry()
    : winWidth(1),
      winHeight(1),
      pad(8.0f),
      toolbarHeight(46.0f),
      bottomPaneHeight(0.0f),
      contentTop(0.0f),
      contentBottom(0.0f),
      contentHeight(0.0f),
      leftX(0.0f),
      leftY(0.0f),
      leftPaneWidth(0.0f),
      leftInspectorH(0.0f),
      leftLightingY(0.0f),
      leftLightingH(0.0f),
      rightX(0.0f),
      rightY(0.0f),
      rightPaneWidth(0.0f),
      rightSceneH(0.0f),
      rightModifierY(0.0f),
      rightModifierH(0.0f),
      rightShortcutsY(0.0f),
      rightShortcutsH(0.0f),
      centerX(0.0f),
      centerY(0.0f),
      centerW(0.0f),
      centerH(0.0f) {}

float clampUiFloat(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

int clampUiInt(int value, int minValue, int maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

void clearMirrorHelperSelection(TransformationState& transformState) {
    transformState.mirrorHelperSelected = false;
    transformState.mirrorHelperShapeIndex = -1;
    transformState.mirrorHelperAxis = -1;
    transformState.mirrorHelperMoveModeActive = false;
    transformState.mirrorHelperMoveConstrained = false;
    transformState.mirrorHelperMoveAxis = -1;
}

void selectMirrorHelperForShape(int shapeIndex, const Shape& shape, TransformationState& transformState) {
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
}

void initShapeDefaults(Shape& newShape, int type, int globalScaleMode, std::size_t shapeCount) {
    newShape.type = type;
    newShape.center[0] = newShape.center[1] = newShape.center[2] = 0.0f;
    newShape.param[0] = newShape.param[1] = newShape.param[2] = 0.0f;
    newShape.fractalExtra[0] = 1.0f;
    newShape.fractalExtra[1] = 1.0f;
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
    newShape.name = std::to_string(shapeCount);
    newShape.albedo[0] = 1.0f;
    newShape.albedo[1] = 1.0f;
    newShape.albedo[2] = 1.0f;
    newShape.metallic = 0.0f;
    newShape.roughness = 0.05f;
    newShape.emission[0] = 0.0f;
    newShape.emission[1] = 0.0f;
    newShape.emission[2] = 0.0f;
    newShape.emissionStrength = 0.0f;
    newShape.transmission = 0.0f;
    newShape.ior = 1.5f;
}

} // namespace rmt
