#pragma once

#include <filesystem>
#include <shellapi.h>
#include "Application/IApplication.h"
#include "Editor/UEditorEngine.h"
#include "Engine/FEngineLoop.h"
#include "Engine/IEngineLoopClient.h"
#include "Platform/Windows/FWindowsApplication.h"
#include "Platform/Windows/IWindowsMessageHandler.h"

class FApplication final : public IApplication, private IEngineLoopClient, private IWindowsMessageHandler {
private:
    struct FWindowState {
        // 메뉴 클릭 영역과 창 드래그 영역을 구분하는 메뉴 오른쪽 끝 X 좌표입니다.
        int mMenuHitRight{900};
        // 사용자가 창을 이동하거나 크기를 조절하는 중인지 나타냅니다.
        bool mInMoveLoop{};
    };

public:
    FApplication();
    ~FApplication() override;

public:
    int Run(HINSTANCE Instance, int ShowCommand) final;

private:
    bool Initialize();
    void Tick(float DeltaTime) override;
    void Shutdown();

    void RestoreGameWindow();
    void DrawCaptionButton(const char* Identifier, int Index, UINT Command);
    void DrawTitleBar();

    void QueueExternalFileDrops(HWND WindowHandle, HDROP DropHandle);
    void EnableExternalDropsForImGuiViewports();
    LRESULT ProcessWindowMessage(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam) override;

    static LRESULT CALLBACK ExternalDropWindowProcedure(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam);

private:
    static constexpr UINT mLoadingWindowWidth{960};
    static constexpr UINT mLoadingWindowHeight{540};
    FWindowsApplication mPlatform{};
    UEditorEngine mEngine{};
    FWindowState mWindowState{};
    FEngineLoop mEngineLoop{};
    HINSTANCE mInstance{};
    int mShowCommand{};
};
