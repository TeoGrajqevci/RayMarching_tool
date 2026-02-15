#ifndef RMT_INPUT_TRANSFORM_TOOL_COMMON_H
#define RMT_INPUT_TRANSFORM_TOOL_COMMON_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "imgui.h"

#include "rmt/input/Input.h"
#include "rmt/scene/Shape.h"

namespace rmt {

inline bool isViewportClickAllowedForTransform(GLFWwindow* window,
                                               const ImVec2& viewportPos,
                                               const ImVec2& viewportSize) {
    (void)viewportPos;
    (void)viewportSize;

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    return !shouldBlockViewportInput(mouseX, mouseY) &&
           glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
}

inline void rotateAroundAxis(float point[3], const float center[3], float axis[3], float angle) {
    const float translated[3] = {
        point[0] - center[0],
        point[1] - center[1],
        point[2] - center[2]
    };

    const float cosA = std::cos(angle);
    const float sinA = std::sin(angle);
    const float oneMinusCosA = 1.0f - cosA;

    const float axisLength = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    if (axisLength < 1e-6f) {
        return;
    }
    const float normalizedAxis[3] = {axis[0] / axisLength, axis[1] / axisLength, axis[2] / axisLength};

    float rotated[3];
    rotated[0] = translated[0] * (cosA + normalizedAxis[0] * normalizedAxis[0] * oneMinusCosA) +
                 translated[1] * (normalizedAxis[0] * normalizedAxis[1] * oneMinusCosA - normalizedAxis[2] * sinA) +
                 translated[2] * (normalizedAxis[0] * normalizedAxis[2] * oneMinusCosA + normalizedAxis[1] * sinA);

    rotated[1] = translated[0] * (normalizedAxis[1] * normalizedAxis[0] * oneMinusCosA + normalizedAxis[2] * sinA) +
                 translated[1] * (cosA + normalizedAxis[1] * normalizedAxis[1] * oneMinusCosA) +
                 translated[2] * (normalizedAxis[1] * normalizedAxis[2] * oneMinusCosA - normalizedAxis[0] * sinA);

    rotated[2] = translated[0] * (normalizedAxis[2] * normalizedAxis[0] * oneMinusCosA - normalizedAxis[1] * sinA) +
                 translated[1] * (normalizedAxis[2] * normalizedAxis[1] * oneMinusCosA + normalizedAxis[0] * sinA) +
                 translated[2] * (cosA + normalizedAxis[2] * normalizedAxis[2] * oneMinusCosA);

    point[0] = rotated[0] + center[0];
    point[1] = rotated[1] + center[1];
    point[2] = rotated[2] + center[2];
}

inline void setCartesianAxis(int axis, float out[3]) {
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
    if (axis >= 0 && axis < 3) {
        out[axis] = 1.0f;
    }
}

inline void normalize3(float v[3]) {
    const float len = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len > 1e-6f) {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
}

inline void rotateX(const float in[3], float angle, float out[3]) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    out[0] = in[0];
    out[1] = c * in[1] - s * in[2];
    out[2] = s * in[1] + c * in[2];
}

inline void rotateY(const float in[3], float angle, float out[3]) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    out[0] = c * in[0] + s * in[2];
    out[1] = in[1];
    out[2] = -s * in[0] + c * in[2];
}

inline void rotateZ(const float in[3], float angle, float out[3]) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    out[0] = c * in[0] - s * in[1];
    out[1] = s * in[0] + c * in[1];
    out[2] = in[2];
}

inline void rotateWorld(const float in[3], const float angles[3], float out[3]) {
    float tmp[3] = {in[0], in[1], in[2]};
    float next[3];
    if (angles[0] != 0.0f) {
        rotateX(tmp, angles[0], next);
        tmp[0] = next[0];
        tmp[1] = next[1];
        tmp[2] = next[2];
    }
    if (angles[1] != 0.0f) {
        rotateY(tmp, angles[1], next);
        tmp[0] = next[0];
        tmp[1] = next[1];
        tmp[2] = next[2];
    }
    if (angles[2] != 0.0f) {
        rotateZ(tmp, angles[2], next);
        tmp[0] = next[0];
        tmp[1] = next[1];
        tmp[2] = next[2];
    }
    out[0] = tmp[0];
    out[1] = tmp[1];
    out[2] = tmp[2];
}

inline void getLocalAxisWorldDirection(const float rotation[3], int axis, float out[3]) {
    float localAxis[3] = {0.0f, 0.0f, 0.0f};
    if (axis < 0 || axis > 2) {
        out[0] = 1.0f;
        out[1] = 0.0f;
        out[2] = 0.0f;
        return;
    }
    localAxis[axis] = 1.0f;
    rotateWorld(localAxis, rotation, out);
    normalize3(out);
}

inline void updateGuideAxisDirection(TransformationState& ts,
                                     const std::vector<Shape>& shapes,
                                     const std::vector<int>& selectedShapes,
                                     int axis) {
    setCartesianAxis(axis, ts.guideAxisDirection);
    if (!ts.useLocalSpace || selectedShapes.empty() || axis < 0 || axis > 2) {
        return;
    }
    const int shapeIndex = selectedShapes.front();
    if (shapeIndex < 0 || shapeIndex >= static_cast<int>(shapes.size())) {
        return;
    }
    getLocalAxisWorldDirection(shapes[shapeIndex].rotation, axis, ts.guideAxisDirection);
}

inline float constrainedAxisDragAmount(int axis, double deltaX, double deltaY, float sensitivity) {
    if (axis == 1) {
        return -static_cast<float>(deltaY) * sensitivity;
    }
    return static_cast<float>(deltaX) * sensitivity;
}

} // namespace rmt

#endif // RMT_INPUT_TRANSFORM_TOOL_COMMON_H
