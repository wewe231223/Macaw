#pragma once

#include "Core/FrameTimer.h"
#include "Editor/EditorEngine.h"
#include "Renderer/Renderer.h"

class FApplication {
public:
    FApplication();
    ~FApplication();

public:
    int Run();

private:
    void Shutdown();

private:
    FWindowsApplication mPlatform{};
    FRenderer mRenderer{};
    UEditorEngine mEngine{};
    FFrameTimer mTimer{};
};
