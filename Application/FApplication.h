#pragma once

#include <atomic>
#include <filesystem>
#include <vector>
#include <shellapi.h>
#include "Application/IApplication.h"
#include "Application/FApplicationContext.h"
#include "Engine/FEngineLoop.h"
#include "Engine/IEngineLoopClient.h"
#include "Platform/Windows/FWindowsApplication.h"
#include "Platform/Windows/IWindowsMessageHandler.h"

class FLoadingProgress;

class FApplication : public IApplication, private IEngineLoopClient, private IWindowsMessageHandler {
private:
    struct FWindowState {
        // 메뉴 클릭 영역과 창 드래그 영역을 구분하는 메뉴 오른쪽 끝 X 좌표입니다.
        int mMenuHitRight{900};
        // 사용자가 창을 이동하거나 크기를 조절하는 중인지 나타냅니다.
        bool mInMoveLoop{};
        // 초기화 완료 후부터 종료 시작 전까지 일반 프레임 처리를 허용합니다.
        bool mFrameEnabled{};
        // ImGui 초기화 여부를 기록하여 초기화 전 정리와 중복 정리를 방지합니다.
        bool mImGuiInitialized{};
        bool mImGuiPlatformInitialized{};
        bool mImGuiRendererInitialized{};
    };

    struct FPendingExternalFileDrop {
        std::filesystem::path mFilePath{};
        POINT mScreenPosition{};
    };

public:
    FApplication();
    ~FApplication() override;

public:
    int Run(HINSTANCE Instance, int ShowCommand) final;

private:
    virtual void InitializeMode(FApplicationContext& Context, HWND WindowHandle) = 0;
    virtual void ProcessInput(FApplicationContext& Context, float DeltaTime);
    virtual void RenderMode(FApplicationContext& Context, float DeltaTime) = 0;

    bool InitializeApplication(FLoadingProgress& Progress, HWND WindowHandle);
    bool Initialize() override;
    void Tick(float DeltaTime) override;
    void SaveState();
    void Shutdown() override;

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
    FApplicationContext mContext{};
    FWindowState mWindowState{};
    std::atomic<bool> mAcceptGameInput{};
    FEngineLoop mEngineLoop{};
    FWindowsApplication mPlatform{};
    HINSTANCE mInstance{};
    int mShowCommand{};
    bool mInitialized{};
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mEditorLogo{};
    std::vector<FPendingExternalFileDrop> mPendingExternalFileDrops{};
};
