#include "Editor/FImGuiUIScale.h"
#include "Platform/Window.h"

#include <imgui_impl_win32.h>
#include <algorithm>
#include <cmath>
#include <string>

void FImGuiUIScale::Initialize(float FontSize) {
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

void FImGuiUIScale::Update(const FWindowsApplication& Platform) {
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

void FImGuiUIScale::DrawMenu() {
    if (!ImGui::BeginMenu("UI Scale")) {
        return;
    }
    if (ImGui::MenuItem("Automatic", nullptr, mUserScale == 0.0f)) {
        mUserScale = 0.0f;
    }
    ImGui::Separator();
    for (int Percent : {100, 125, 150, 175, 200}) {
        const float Scale{static_cast<float>(Percent) / 100.0f};
        const std::string Label{std::to_string(Percent) + "%"};
        if (ImGui::MenuItem(Label.c_str(), nullptr, mUserScale == Scale)) {
            mUserScale = Scale;
        }
    }
    ImGui::EndMenu();
}

float FImGuiUIScale::GetScale() const {
    return mAppliedScale;
}
