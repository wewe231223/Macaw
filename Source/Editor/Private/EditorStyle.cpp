/**
 * @file EditorStyle.cpp
 * @brief 에디터 공통 벡터 아이콘과 툴바 버튼 스타일을 구현합니다.
 * @ingroup Editor
 */

#include "EditorStyle.h"

#include <imgui_internal.h>
#include <array>

namespace Editor {

    /**
     * @fn void Editor::EditorStyle::DrawIcon(EIcon Icon, const ImVec2& Origin, float Size, ImU32 Color)
     * @brief 공통 아이콘의 정규화된 도형을 요청한 화면 크기와 색상으로 그립니다.
     */
    void Editor::EditorStyle::DrawIcon(EIcon Icon, const ImVec2& Origin, float Size, ImU32 Color) {
        ImDrawList* DrawList{ImGui::GetWindowDrawList()};
        const float Scale{Size / 20.0f};
        const float Thickness{1.6f * Scale};
        const auto Point{[&Origin, Scale](float X, float Y) {
            return ImVec2{Origin.x + X * Scale, Origin.y + Y * Scale};
        }};
        const auto Line{[DrawList, &Point, Color, Thickness](float X1, float Y1, float X2, float Y2) {
            DrawList->AddLine(Point(X1, Y1), Point(X2, Y2), Color, Thickness);
        }};

        switch (Icon) {
        case EIcon::Tools: {
            const std::array<ImVec2, 6> Cube{Point(2.0f, 5.0f), Point(8.0f, 2.0f), Point(14.0f, 5.0f), Point(14.0f, 12.0f), Point(8.0f, 15.0f), Point(2.0f, 12.0f)};
            DrawList->AddPolyline(Cube.data(), static_cast<int>(Cube.size()), Color, Thickness, ImDrawFlags_Closed);
            Line(2.0f, 5.0f, 8.0f, 8.0f);
            Line(8.0f, 8.0f, 14.0f, 5.0f);
            Line(8.0f, 8.0f, 8.0f, 15.0f);
            DrawList->AddTriangleFilled(Point(10.0f, 9.0f), Point(18.0f, 14.0f), Point(12.0f, 16.0f), Color);
            Line(14.0f, 14.0f, 16.0f, 18.0f);
            break;
        }

        case EIcon::Translate:
            Line(10.0f, 3.0f, 10.0f, 17.0f);
            Line(3.0f, 10.0f, 17.0f, 10.0f);
            DrawList->AddTriangleFilled(Point(10.0f, 1.0f), Point(7.0f, 5.0f), Point(13.0f, 5.0f), Color);
            DrawList->AddTriangleFilled(Point(10.0f, 19.0f), Point(7.0f, 15.0f), Point(13.0f, 15.0f), Color);
            DrawList->AddTriangleFilled(Point(1.0f, 10.0f), Point(5.0f, 7.0f), Point(5.0f, 13.0f), Color);
            DrawList->AddTriangleFilled(Point(19.0f, 10.0f), Point(15.0f, 7.0f), Point(15.0f, 13.0f), Color);
            break;
        case EIcon::Rotate:
            DrawList->PathArcTo(Point(10.0f, 10.0f), 7.0f * Scale, -2.25f, 2.55f, 24);
            DrawList->PathStroke(Color, Thickness);
            DrawList->AddTriangleFilled(Point(3.0f, 12.0f), Point(3.0f, 18.0f), Point(8.0f, 14.0f), Color);
            break;
        case EIcon::Scale:
            Line(3.0f, 3.0f, 17.0f, 3.0f);
            Line(17.0f, 3.0f, 17.0f, 17.0f);
            Line(17.0f, 17.0f, 11.0f, 17.0f);
            Line(3.0f, 3.0f, 3.0f, 9.0f);
            DrawList->AddRectFilled(Point(2.0f, 12.0f), Point(8.0f, 18.0f), Color, Scale);
            break;
        case EIcon::World:
            DrawList->AddCircle(Point(10.0f, 10.0f), 8.0f * Scale, Color, 24, Thickness);
            DrawList->AddEllipse(Point(10.0f, 10.0f), ImVec2{3.5f * Scale, 8.0f * Scale}, Color, 0.0f, 24, Thickness);
            Line(2.0f, 10.0f, 18.0f, 10.0f);
            break;
        case EIcon::Local: {
            const std::array<ImVec2, 6> Cube{Point(3.0f, 6.0f), Point(10.0f, 2.0f), Point(17.0f, 6.0f), Point(17.0f, 14.0f), Point(10.0f, 18.0f), Point(3.0f, 14.0f)};
            DrawList->AddPolyline(Cube.data(), static_cast<int>(Cube.size()), Color, Thickness, ImDrawFlags_Closed);
            Line(3.0f, 6.0f, 10.0f, 10.0f);
            Line(10.0f, 10.0f, 17.0f, 6.0f);
            Line(10.0f, 10.0f, 10.0f, 18.0f);
            break;
        }

        case EIcon::Grid:
            for (int Index{}; Index < 4; ++Index) {
                const float Offset{3.0f + static_cast<float>(Index) * 4.5f};
                Line(Offset, 3.0f, Offset, 16.5f);
                Line(3.0f, Offset, 16.5f, Offset);
            }

            break;
        case EIcon::Play:
            DrawList->AddTriangleFilled(Point(6.0f, 3.0f), Point(17.0f, 10.0f), Point(6.0f, 17.0f), Color);
            break;
        case EIcon::Pause:
            DrawList->AddRectFilled(Point(4.0f, 4.0f), Point(8.0f, 16.0f), Color, Scale);
            DrawList->AddRectFilled(Point(12.0f, 4.0f), Point(16.0f, 16.0f), Color, Scale);
            break;
        case EIcon::Step:
            DrawList->AddTriangleFilled(Point(3.0f, 4.0f), Point(13.0f, 10.0f), Point(3.0f, 16.0f), Color);
            DrawList->AddRectFilled(Point(15.0f, 4.0f), Point(18.0f, 16.0f), Color, Scale);
            break;
        case EIcon::Stop:
            DrawList->AddRectFilled(Point(4.0f, 4.0f), Point(16.0f, 16.0f), Color, Scale);
            break;
        case EIcon::QuadView:
            DrawList->AddRect(Point(2.0f, 2.0f), Point(18.0f, 18.0f), Color, Scale, ImDrawFlags_None, Thickness);
            Line(10.0f, 2.0f, 10.0f, 18.0f);
            Line(2.0f, 10.0f, 18.0f, 10.0f);
            break;
        case EIcon::Maximize:
            Line(3.0f, 8.0f, 3.0f, 3.0f);
            Line(3.0f, 3.0f, 8.0f, 3.0f);
            Line(12.0f, 3.0f, 17.0f, 3.0f);
            Line(17.0f, 3.0f, 17.0f, 8.0f);
            Line(17.0f, 12.0f, 17.0f, 17.0f);
            Line(17.0f, 17.0f, 12.0f, 17.0f);
            Line(8.0f, 17.0f, 3.0f, 17.0f);
            Line(3.0f, 17.0f, 3.0f, 12.0f);
            break;
        }
    }

    /**
     * @fn bool Editor::EditorStyle::IconButton(const char* Label, EIcon Icon, bool Selected, bool Enabled, bool Dropdown, const char* Tooltip)
     * @brief 아이콘 버튼을 표시하고 선택 색상, 비활성 처리와 툴팁을 적용합니다.
     */
    bool Editor::EditorStyle::IconButton(const char* Label, EIcon Icon, bool Selected, bool Enabled, bool Dropdown, const char* Tooltip) {
        const float Scale{ImGui::GetStyle().FontScaleMain * ImGui::GetStyle().FontScaleDpi};

        ImGui::PushID(Label);
        ImGui::BeginDisabled(!Enabled);

        if (Selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
        }

        const float Height{ImGui::GetFrameHeight()};
        const float IconSize{ImGui::GetFontSize() + 2.0f * Scale};
        const bool Pressed{ImGui::Button("##Button", ImVec2{Height + (Dropdown ? 12.0f * Scale : 0.0f), Height})};
        const ImVec2 Minimum{ImGui::GetItemRectMin()};
        const ImVec2 Origin{Minimum.x + (Height - IconSize) * 0.5f, Minimum.y + (Height - IconSize) * 0.5f};
        const ImVec4 Foreground{Icon == EIcon::Play ? ImVec4{0.30f, 0.85f, 0.40f, 1.0f} : Icon == EIcon::Stop ? ImVec4{0.95f, 0.30f, 0.28f, 1.0f} : ImGui::GetStyleColorVec4(ImGuiCol_Text)};
        const ImU32 Color{ImGui::GetColorU32(Foreground)};
        Editor::EditorStyle::DrawIcon(Icon, Origin, IconSize, Color);

        if (Dropdown) {
            ImDrawList* DrawList{ImGui::GetWindowDrawList()};
            const float X{Minimum.x + Height + 3.0f * Scale};
            const float Y{Minimum.y + Height * 0.5f};
            DrawList->AddTriangleFilled(ImVec2{X - 3.0f * Scale, Y - 1.5f * Scale}, ImVec2{X + 3.0f * Scale, Y - 1.5f * Scale}, ImVec2{X, Y + 1.5f * Scale}, Color);
        }

        if (Selected) {
            ImGui::PopStyleColor(2);
        }

        ImGui::EndDisabled();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", Tooltip != nullptr ? Tooltip : Label);
        }

        ImGui::PopID();

        return Pressed;
    }

    /**
     * @fn void Editor::EditorStyle::PushViewportToolbar()
     * @brief 뷰포트 툴바에서 사용할 스타일을 ImGui 스택에 추가합니다.
     */
    void Editor::EditorStyle::PushViewportToolbar() {
        const float Scale{ImGui::GetStyle().FontScaleMain * ImGui::GetStyle().FontScaleDpi};
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{5.0f * Scale, 5.0f * Scale});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{2.0f * Scale, 4.0f * Scale});
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f * Scale);
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
    }

    /**
     * @fn void Editor::EditorStyle::PopViewportToolbar()
     * @brief 뷰포트 툴바의 스타일 스택 변경을 되돌립니다.
     */
    void Editor::EditorStyle::PopViewportToolbar() {
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(3);
    }

    /**
     * @fn bool Editor::EditorStyle::IconMenuItem(const char* Label, EIcon Icon, bool Selected, bool Enabled)
     * @brief 메뉴의 기본 아이콘 열에 벡터 아이콘을 배치하여 항목 이름과 체크 표시를 정렬합니다.
     */
    bool Editor::EditorStyle::IconMenuItem(const char* Label, EIcon Icon, bool Selected, bool Enabled) {
        const ImVec2 Position{ImGui::GetCursorScreenPos()};
        const float Size{ImGui::GetFontSize()};
        ImGui::GetCurrentWindow()->DC.MenuColumns.DeclColumns(Size, 0.0f, 0.0f, 0.0f);
        ImGui::BeginDisabled(!Enabled);

        const bool Pressed{ImGui::MenuItem(Label, nullptr, Selected)};
        const ImVec2 Minimum{ImGui::GetItemRectMin()};
        const ImVec2 Maximum{ImGui::GetItemRectMax()};
        Editor::EditorStyle::DrawIcon(Icon, ImVec2{Position.x, (Minimum.y + Maximum.y - Size) * 0.5f}, Size, ImGui::GetColorU32(ImGuiCol_Text));
        ImGui::EndDisabled();

        return Pressed;
    }

}
