#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/FrameTimer.h"
#include "Editor/EditorEngine.h"
#include "Renderer/Renderer.h"

class FApplication {
public:
    ReflectTypes(FApplication);

public:
    FApplication();
    ~FApplication();

public:
    ReflectStruct(FApplication);

    int Run();

private:
    void Shutdown();

private:
    ReflectMembers();

private:
    FWindowsApplication mPlatform{};
    ReflectProperty(mPlatform, "Platform", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FRenderer mRenderer{};
    ReflectProperty(mRenderer, "Renderer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    UEditorEngine mEngine{};
    ReflectProperty(mEngine, "Engine", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FFrameTimer mTimer{};
    ReflectProperty(mTimer, "Timer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
