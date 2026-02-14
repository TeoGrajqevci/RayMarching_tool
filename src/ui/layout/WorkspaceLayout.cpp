#include "rmt/ui/layout/WorkspaceLayout.h"

#include <algorithm>

#include "imgui.h"

namespace rmt {

namespace {

bool pointInRect(const ImVec2& p, float minX, float minY, float maxX, float maxY) {
    return p.x >= minX && p.x <= maxX && p.y >= minY && p.y <= maxY;
}

} // namespace

void updateWorkspaceLayout(GLFWwindow* window,
                           bool showConsole,
                           bool showHelpPopup,
                           UiRuntimeState& uiState,
                           UiWorkspaceGeometry& geometry) {
    glfwGetWindowSize(window, &geometry.winWidth, &geometry.winHeight);

    geometry.pad = 8.0f;
    geometry.toolbarHeight = 46.0f;

    const float defaultLeftPaneWidth = std::min(std::max(geometry.winWidth * 0.20f, 220.0f), 360.0f);
    const float defaultRightPaneWidth = std::min(std::max(geometry.winWidth * 0.18f, 210.0f), 340.0f);
    geometry.bottomPaneHeight = showConsole ? std::min(std::max(geometry.winHeight * 0.24f, 160.0f), 320.0f) : 0.0f;

    geometry.contentTop = geometry.toolbarHeight + geometry.pad;
    geometry.contentBottom = static_cast<float>(geometry.winHeight) - geometry.pad - geometry.bottomPaneHeight;
    geometry.contentHeight = std::max(120.0f, geometry.contentBottom - geometry.contentTop);

    const float minLeftPaneWidth = 220.0f;
    const float minRightPaneWidth = 210.0f;
    const float minSectionHeight = 80.0f;
    const float minCenterWidth = std::max(
        120.0f,
        std::min(320.0f, static_cast<float>(geometry.winWidth) - 4.0f * geometry.pad - minLeftPaneWidth - minRightPaneWidth));

    if (uiState.leftPaneWidthState < 0.0f) {
        uiState.leftPaneWidthState = defaultLeftPaneWidth;
    }
    if (uiState.rightPaneWidthState < 0.0f) {
        uiState.rightPaneWidthState = defaultRightPaneWidth;
    }
    if (uiState.rightSceneHeightState < 0.0f) {
        uiState.rightSceneHeightState = geometry.contentHeight * 0.45f;
    }
    if (uiState.rightModifierHeightState < 0.0f) {
        uiState.rightModifierHeightState = geometry.contentHeight * 0.30f;
    }

    auto clampColumnWidths = [&]() {
        const float maxLeft = std::max(minLeftPaneWidth, static_cast<float>(geometry.winWidth) -
                                                         4.0f * geometry.pad - uiState.rightPaneWidthState - minCenterWidth);
        uiState.leftPaneWidthState = clampUiFloat(uiState.leftPaneWidthState, minLeftPaneWidth, maxLeft);

        const float maxRight = std::max(minRightPaneWidth, static_cast<float>(geometry.winWidth) -
                                                           4.0f * geometry.pad - uiState.leftPaneWidthState - minCenterWidth);
        uiState.rightPaneWidthState = clampUiFloat(uiState.rightPaneWidthState, minRightPaneWidth, maxRight);

        const float maxLeftAfterRight = std::max(minLeftPaneWidth, static_cast<float>(geometry.winWidth) -
                                                                   4.0f * geometry.pad - uiState.rightPaneWidthState - minCenterWidth);
        uiState.leftPaneWidthState = clampUiFloat(uiState.leftPaneWidthState, minLeftPaneWidth, maxLeftAfterRight);
    };

    auto clampRowHeights = [&]() {
        if (showHelpPopup) {
            const float maxRightScene = std::max(minSectionHeight, geometry.contentHeight - 2.0f * geometry.pad - 2.0f * minSectionHeight);
            uiState.rightSceneHeightState = clampUiFloat(uiState.rightSceneHeightState, minSectionHeight, maxRightScene);

            const float maxRightModifier = std::max(minSectionHeight, geometry.contentHeight -
                                                                      2.0f * geometry.pad - uiState.rightSceneHeightState - minSectionHeight);
            uiState.rightModifierHeightState = clampUiFloat(uiState.rightModifierHeightState, minSectionHeight, maxRightModifier);
        } else {
            const float maxRightScene = std::max(minSectionHeight, geometry.contentHeight - geometry.pad - minSectionHeight);
            uiState.rightSceneHeightState = clampUiFloat(uiState.rightSceneHeightState, minSectionHeight, maxRightScene);

            const float maxRightModifier = std::max(minSectionHeight, geometry.contentHeight - geometry.pad - uiState.rightSceneHeightState);
            uiState.rightModifierHeightState = clampUiFloat(uiState.rightModifierHeightState, minSectionHeight, maxRightModifier);
        }
    };

    clampColumnWidths();
    clampRowHeights();

    float leftPaneWidth = uiState.leftPaneWidthState;
    float rightPaneWidth = uiState.rightPaneWidthState;
    float leftInspectorH = geometry.contentHeight;
    float rightSceneH = uiState.rightSceneHeightState;
    float rightModifierH = 0.0f;

    float leftX = geometry.pad;
    float leftY = geometry.contentTop;
    float rightX = static_cast<float>(geometry.winWidth) - geometry.pad - rightPaneWidth;
    float rightY = geometry.contentTop;
    float rightModifierY = rightY + rightSceneH + geometry.pad;

    if (showHelpPopup) {
        rightModifierH = uiState.rightModifierHeightState;
    } else {
        rightModifierH = std::max(minSectionHeight, geometry.contentHeight - rightSceneH - geometry.pad);
    }

    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

    const bool hoverLeftVerticalSplitter = pointInRect(mouse,
                                                       leftX + leftPaneWidth,
                                                       geometry.contentTop,
                                                       leftX + leftPaneWidth + geometry.pad,
                                                       geometry.contentBottom);
    const bool hoverRightVerticalSplitter = pointInRect(mouse,
                                                        rightX - geometry.pad,
                                                        geometry.contentTop,
                                                        rightX,
                                                        geometry.contentBottom);
    const bool hoverLeftHorizontalSplitter = false;
    const bool hoverRightSceneModifierSplitter = pointInRect(mouse,
                                                             rightX,
                                                             rightY + rightSceneH,
                                                             rightX + rightPaneWidth,
                                                             rightY + rightSceneH + geometry.pad);
    const bool hoverRightModifierShortcutsSplitter =
        showHelpPopup &&
        pointInRect(mouse,
                    rightX,
                    rightModifierY + rightModifierH,
                    rightX + rightPaneWidth,
                    rightModifierY + rightModifierH + geometry.pad);

    if (uiState.activeLayoutSplitter == 0 && mouseClicked) {
        if (hoverLeftVerticalSplitter) {
            uiState.activeLayoutSplitter = 1;
            uiState.splitterStartMouseX = mouse.x;
            uiState.splitterStartValue = uiState.leftPaneWidthState;
        } else if (hoverRightVerticalSplitter) {
            uiState.activeLayoutSplitter = 2;
            uiState.splitterStartMouseX = mouse.x;
            uiState.splitterStartValue = uiState.rightPaneWidthState;
        } else if (hoverRightSceneModifierSplitter) {
            uiState.activeLayoutSplitter = 4;
            uiState.splitterStartMouseY = mouse.y;
            uiState.splitterStartValue = uiState.rightSceneHeightState;
        } else if (hoverRightModifierShortcutsSplitter) {
            uiState.activeLayoutSplitter = 5;
            uiState.splitterStartMouseY = mouse.y;
            uiState.splitterStartValue = uiState.rightModifierHeightState;
        }
    }

    if (uiState.activeLayoutSplitter != 0 && mouseDown) {
        if (uiState.activeLayoutSplitter == 1) {
            uiState.leftPaneWidthState = uiState.splitterStartValue + (mouse.x - uiState.splitterStartMouseX);
        } else if (uiState.activeLayoutSplitter == 2) {
            uiState.rightPaneWidthState = uiState.splitterStartValue - (mouse.x - uiState.splitterStartMouseX);
        } else if (uiState.activeLayoutSplitter == 4) {
            uiState.rightSceneHeightState = uiState.splitterStartValue + (mouse.y - uiState.splitterStartMouseY);
        } else if (uiState.activeLayoutSplitter == 5) {
            uiState.rightModifierHeightState = uiState.splitterStartValue + (mouse.y - uiState.splitterStartMouseY);
        }
        clampColumnWidths();
        clampRowHeights();
    }

    if (uiState.activeLayoutSplitter != 0 && !mouseDown) {
        uiState.activeLayoutSplitter = 0;
    }

    const bool onVerticalSplitter = hoverLeftVerticalSplitter ||
                                    hoverRightVerticalSplitter ||
                                    uiState.activeLayoutSplitter == 1 ||
                                    uiState.activeLayoutSplitter == 2;
    const bool onHorizontalSplitter = hoverLeftHorizontalSplitter ||
                                      hoverRightSceneModifierSplitter ||
                                      hoverRightModifierShortcutsSplitter ||
                                      uiState.activeLayoutSplitter == 4 ||
                                      uiState.activeLayoutSplitter == 5;

    if (onVerticalSplitter) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    } else if (onHorizontalSplitter) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }

    geometry.leftPaneWidth = uiState.leftPaneWidthState;
    geometry.rightPaneWidth = uiState.rightPaneWidthState;
    geometry.leftInspectorH = geometry.contentHeight;
    geometry.rightSceneH = uiState.rightSceneHeightState;

    geometry.leftX = geometry.pad;
    geometry.leftY = geometry.contentTop;
    geometry.leftLightingY = geometry.leftY + geometry.leftInspectorH;
    geometry.leftLightingH = 0.0f;

    geometry.rightX = static_cast<float>(geometry.winWidth) - geometry.pad - geometry.rightPaneWidth;
    geometry.rightY = geometry.contentTop;
    geometry.rightModifierY = geometry.rightY + geometry.rightSceneH + geometry.pad;
    geometry.rightShortcutsY = geometry.rightModifierY;

    if (showHelpPopup) {
        geometry.rightModifierH = uiState.rightModifierHeightState;
        geometry.rightShortcutsY = geometry.rightModifierY + geometry.rightModifierH + geometry.pad;
        geometry.rightShortcutsH = std::max(minSectionHeight,
                                            geometry.contentHeight - geometry.rightSceneH - geometry.rightModifierH - 2.0f * geometry.pad);
    } else {
        geometry.rightModifierH = std::max(minSectionHeight, geometry.contentHeight - geometry.rightSceneH - geometry.pad);
        geometry.rightShortcutsH = 0.0f;
    }

    geometry.centerX = geometry.leftX + geometry.leftPaneWidth + geometry.pad;
    geometry.centerY = geometry.contentTop;
    geometry.centerW = std::max(120.0f, geometry.rightX - geometry.pad - geometry.centerX);
    geometry.centerH = geometry.contentHeight;
}

void drawWorkspaceSplitters(const UiWorkspaceGeometry& geometry,
                            bool showHelpPopup) {
    ImDrawList* splitterDrawList = ImGui::GetForegroundDrawList();
    const ImU32 splitterColor = IM_COL32(120, 120, 120, 170);

    splitterDrawList->AddLine(
        ImVec2(geometry.leftX + geometry.leftPaneWidth + geometry.pad * 0.5f, geometry.contentTop),
        ImVec2(geometry.leftX + geometry.leftPaneWidth + geometry.pad * 0.5f, geometry.contentBottom),
        splitterColor,
        1.0f);

    splitterDrawList->AddLine(
        ImVec2(geometry.rightX - geometry.pad * 0.5f, geometry.contentTop),
        ImVec2(geometry.rightX - geometry.pad * 0.5f, geometry.contentBottom),
        splitterColor,
        1.0f);

    splitterDrawList->AddLine(
        ImVec2(geometry.rightX, geometry.rightY + geometry.rightSceneH + geometry.pad * 0.5f),
        ImVec2(geometry.rightX + geometry.rightPaneWidth, geometry.rightY + geometry.rightSceneH + geometry.pad * 0.5f),
        splitterColor,
        1.0f);

    if (showHelpPopup) {
        splitterDrawList->AddLine(
            ImVec2(geometry.rightX, geometry.rightModifierY + geometry.rightModifierH + geometry.pad * 0.5f),
            ImVec2(geometry.rightX + geometry.rightPaneWidth, geometry.rightModifierY + geometry.rightModifierH + geometry.pad * 0.5f),
            splitterColor,
            1.0f);
    }
}

} // namespace rmt
