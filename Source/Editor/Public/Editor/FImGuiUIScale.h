#pragma once

#include "Core/ReflectionMacros.h"

#include <imgui.h>

class FWindowsApplication;

class FImGuiUIScale {
public:
    ReflectTypes(FImGuiUIScale);

public:
    ReflectStruct(FImGuiUIScale);

    void Initialize(float FontSize);
    void Update(const FWindowsApplication& Platform);
    void DrawMenu();

    float GetScale() const;

private:
    ReflectMembers();

private:
    ImGuiStyle mBaseStyle{};
    ReflectProperty(mBaseStyle, "BaseStyle", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mAppliedScale{};
    ReflectProperty(mAppliedScale, "AppliedScale", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mUserScale{};
    ReflectProperty(mUserScale, "UserScale", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
