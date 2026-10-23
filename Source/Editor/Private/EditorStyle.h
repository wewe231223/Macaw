#pragma once

#include <imgui.h>

namespace EditorStyle {
    enum class EIcon {
        Tools,
        Translate,
        Rotate,
        Scale,
        World,
        Local,
        Grid
    };

    void PushViewportToolbar();
    void PopViewportToolbar();
    void DrawIcon(EIcon Icon, const ImVec2& Origin, float Size, ImU32 Color);
    bool IconButton(const char* Label, EIcon Icon, bool Selected = false, bool Enabled = true, bool Dropdown = false, const char* Tooltip = nullptr);
    bool IconMenuItem(const char* Label, EIcon Icon, bool Selected, bool Enabled = true);
}
