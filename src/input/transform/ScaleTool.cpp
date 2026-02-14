#include "rmt/input/transform/ScaleTool.h"

#include <algorithm>

#include "TransformToolCommon.h"

namespace rmt {

void updateScaleTool(GLFWwindow* window,
                     std::vector<Shape>& shapes,
                     const std::vector<int>& selectedShapes,
                     TransformationState& ts,
                     const ImVec2& viewportPos,
                     const ImVec2& viewportSize) {
    if (!ts.scaleModeActive || selectedShapes.empty()) {
        return;
    }

    double currentMouseX = 0.0;
    double currentMouseY = 0.0;
    glfwGetCursorPos(window, &currentMouseX, &currentMouseY);
    const double deltaY = currentMouseY - ts.scaleStartMouseY;
    const float deformSensitivity = 0.005f;
    const float elongationSensitivity = 0.01f;
    const float scaleFactor = std::max(0.01f, 1.0f + static_cast<float>(deltaY) * deformSensitivity);
    const float elongationDelta = static_cast<float>(deltaY) * elongationSensitivity;

    for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
        const int idx = selectedShapes[i];

        shapes[idx].scale[0] = ts.scaleOriginalScales[i][0];
        shapes[idx].scale[1] = ts.scaleOriginalScales[i][1];
        shapes[idx].scale[2] = ts.scaleOriginalScales[i][2];

        shapes[idx].elongation[0] = ts.scaleOriginalElongations[i][0];
        shapes[idx].elongation[1] = ts.scaleOriginalElongations[i][1];
        shapes[idx].elongation[2] = ts.scaleOriginalElongations[i][2];

        if (shapes[idx].scaleMode == SCALE_MODE_ELONGATE) {
            if (ts.scaleConstrained && ts.scaleAxis >= 0 && ts.scaleAxis < 3) {
                shapes[idx].elongation[ts.scaleAxis] =
                    ts.scaleOriginalElongations[i][ts.scaleAxis] + elongationDelta;
            } else {
                shapes[idx].elongation[0] = ts.scaleOriginalElongations[i][0] + elongationDelta;
                shapes[idx].elongation[1] = ts.scaleOriginalElongations[i][1] + elongationDelta;
                shapes[idx].elongation[2] = ts.scaleOriginalElongations[i][2] + elongationDelta;
            }
        } else {
            if (ts.scaleConstrained && ts.scaleAxis >= 0 && ts.scaleAxis < 3) {
                shapes[idx].scale[ts.scaleAxis] =
                    std::max(0.01f, ts.scaleOriginalScales[i][ts.scaleAxis] * scaleFactor);
            } else {
                shapes[idx].scale[0] = std::max(0.01f, ts.scaleOriginalScales[i][0] * scaleFactor);
                shapes[idx].scale[1] = std::max(0.01f, ts.scaleOriginalScales[i][1] * scaleFactor);
                shapes[idx].scale[2] = std::max(0.01f, ts.scaleOriginalScales[i][2] * scaleFactor);
            }
        }
    }

    if (!ts.scaleConstrained && !ImGui::GetIO().WantCaptureKeyboard) {
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
            ts.scaleConstrained = true;
            ts.scaleAxis = 0;
            ts.activeAxis = 0;
            updateGuideAxisDirection(ts, shapes, selectedShapes, ts.scaleAxis);
            glfwGetCursorPos(window, &ts.scaleStartMouseX, &ts.scaleStartMouseY);
            for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
                const int idx = selectedShapes[i];
                ts.scaleOriginalScales[i][0] = shapes[idx].scale[0];
                ts.scaleOriginalScales[i][1] = shapes[idx].scale[1];
                ts.scaleOriginalScales[i][2] = shapes[idx].scale[2];
                ts.scaleOriginalElongations[i][0] = shapes[idx].elongation[0];
                ts.scaleOriginalElongations[i][1] = shapes[idx].elongation[1];
                ts.scaleOriginalElongations[i][2] = shapes[idx].elongation[2];
            }
        } else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
            ts.scaleConstrained = true;
            ts.scaleAxis = 1;
            ts.activeAxis = 1;
            updateGuideAxisDirection(ts, shapes, selectedShapes, ts.scaleAxis);
            glfwGetCursorPos(window, &ts.scaleStartMouseX, &ts.scaleStartMouseY);
            for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
                const int idx = selectedShapes[i];
                ts.scaleOriginalScales[i][0] = shapes[idx].scale[0];
                ts.scaleOriginalScales[i][1] = shapes[idx].scale[1];
                ts.scaleOriginalScales[i][2] = shapes[idx].scale[2];
                ts.scaleOriginalElongations[i][0] = shapes[idx].elongation[0];
                ts.scaleOriginalElongations[i][1] = shapes[idx].elongation[1];
                ts.scaleOriginalElongations[i][2] = shapes[idx].elongation[2];
            }
        } else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
            ts.scaleConstrained = true;
            ts.scaleAxis = 2;
            ts.activeAxis = 2;
            updateGuideAxisDirection(ts, shapes, selectedShapes, ts.scaleAxis);
            glfwGetCursorPos(window, &ts.scaleStartMouseX, &ts.scaleStartMouseY);
            for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
                const int idx = selectedShapes[i];
                ts.scaleOriginalScales[i][0] = shapes[idx].scale[0];
                ts.scaleOriginalScales[i][1] = shapes[idx].scale[1];
                ts.scaleOriginalScales[i][2] = shapes[idx].scale[2];
                ts.scaleOriginalElongations[i][0] = shapes[idx].elongation[0];
                ts.scaleOriginalElongations[i][1] = shapes[idx].elongation[1];
                ts.scaleOriginalElongations[i][2] = shapes[idx].elongation[2];
            }
        }
    }

    if (isViewportClickAllowedForTransform(window, viewportPos, viewportSize)) {
        ts.scaleModeActive = false;
        ts.scaleConstrained = false;
        ts.showAxisGuides = false;
        ts.activeAxis = -1;
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
            const int idx = selectedShapes[i];
            shapes[idx].scale[0] = ts.scaleOriginalScales[i][0];
            shapes[idx].scale[1] = ts.scaleOriginalScales[i][1];
            shapes[idx].scale[2] = ts.scaleOriginalScales[i][2];
            shapes[idx].elongation[0] = ts.scaleOriginalElongations[i][0];
            shapes[idx].elongation[1] = ts.scaleOriginalElongations[i][1];
            shapes[idx].elongation[2] = ts.scaleOriginalElongations[i][2];
        }
        ts.scaleModeActive = false;
        ts.scaleConstrained = false;
        ts.showAxisGuides = false;
        ts.activeAxis = -1;
    }
}

} // namespace rmt
