/**
 * @file FImGuiUIScale.cpp
 * @brief 창의 DPI와 사용자 설정에 따라 ImGui 스타일과 글꼴 배율을 적용합니다.
 * @ingroup Editor
 */

#include "Editor/FImGuiUIScale.h"
#include "Platform/Window.h"

#include <imgui_impl_win32.h>
#include <algorithm>
#include <cmath>
#include <string>

namespace Editor {

    void Editor::FImGuiUIScale::Initialize(float FontSize) {
        ImGuiIO& Input{ImGui::GetIO()};
        Input.ConfigDpiScaleFonts = false;
        Input.ConfigDpiScaleViewports = true;

        ImGuiStyle& Style{ImGui::GetStyle()};
        Style.FontSizeBase = FontSize;
        Style.FontScaleMain = 1.0f;
        Style.FontScaleDpi = 1.0f;
        mBaseStyle = Style;
        mAppliedScale = 0.0f;
    }

    void Editor::FImGuiUIScale::Update(const Platform::FWindowsApplication& Platform) {
        if (Platform.IsMinimized()) {
            return;
        }

        const float DpiScale{std::max(1.0f, ImGui_ImplWin32_GetDpiScaleForHwnd(Platform.GetWindowHandle()))};
        const float WindowScale{std::clamp(std::min(static_cast<float>(Platform.GetWidth()) / 1440.0f, static_cast<float>(Platform.GetHeight()) / 900.0f), 1.0f, 2.0f)};
        const float Scale{std::round((mUserScale == 0.0f ? std::max(DpiScale, WindowScale) : DpiScale * mUserScale) * 16.0f) / 16.0f};

        if (Scale == mAppliedScale) {
            return;
        }

        ImGuiStyle& Style{ImGui::GetStyle()};
        Style = mBaseStyle;
        Style.ScaleAllSizes(Scale);
        Style.FontScaleMain = Scale;
        mAppliedScale = Scale;
    }

    bool Editor::FImGuiUIScale::DrawMenu() {
        if (!ImGui::BeginMenu("UI Scale")) {
            return false;
        }

        bool Changed{};

        if (ImGui::MenuItem("Automatic", nullptr, mUserScale == 0.0f)) {
            Changed = SetUserScale(0.0f);
        }

        ImGui::Separator();

        for (int Percent : {75, 100, 125, 150, 175, 200, 250, 300}) {
            const float Scale{static_cast<float>(Percent) / 100.0f};
            const std::string Label{std::to_string(Percent) + "%"};

            if (ImGui::MenuItem(Label.c_str(), nullptr, mUserScale == Scale)) {
                Changed = SetUserScale(Scale);
            }
        }

        ImGui::EndMenu();
        return Changed;
    }

    void Editor::FImGuiUIScale::SetBaseStyle(const ImGuiStyle& Style, float FontSize) {
        if (!std::isfinite(FontSize) || FontSize < 10.0f || FontSize > 32.0f) {
            return;
        }

        mBaseStyle = Style;
        mBaseStyle.FontSizeBase = FontSize;
        mBaseStyle.FontScaleMain = 1.0f;
        mBaseStyle.FontScaleDpi = 1.0f;
        mAppliedScale = 0.0f;
    }

    bool Editor::FImGuiUIScale::SetUserScale(float Scale) {
        if (!std::isfinite(Scale) || (Scale != 0.0f && (Scale < 0.75f || Scale > 3.0f))) {
            return false;
        }

        const bool Changed{mUserScale != Scale};
        mUserScale = Scale;
        return Changed;
    }

    float Editor::FImGuiUIScale::GetUserScale() const {
        return mUserScale;
    }

    float Editor::FImGuiUIScale::GetScale() const {
        return mAppliedScale;
    }

}
