#include "rmt/ui/style/ImGuiTheme.h"

#include <algorithm>

#include "ImGuizmo.h"

namespace rmt {

void ensureViewportGizmoStyle(UiRuntimeState& uiState) {
    if (uiState.gizmoAxisColorsSwapped) {
        return;
    }

    ImGuizmo::Style& gizmoStyle = ImGuizmo::GetStyle();
    std::swap(gizmoStyle.Colors[ImGuizmo::DIRECTION_Y], gizmoStyle.Colors[ImGuizmo::DIRECTION_Z]);
    std::swap(gizmoStyle.Colors[ImGuizmo::PLANE_Y], gizmoStyle.Colors[ImGuizmo::PLANE_Z]);
    uiState.gizmoAxisColorsSwapped = true;
}

} // namespace rmt
