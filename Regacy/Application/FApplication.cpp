#include "pch.h"
#include "Application/FApplication.h"
#include "Resource.h"
#include "Core/Console/Console.h"

FApplication::FApplication() = default;

FApplication::~FApplication() {
    Shutdown();
}

int FApplication::Run(HINSTANCE Instance, int ShowCommand) {
    mInstance = Instance;
    mShowCommand = ShowCommand;

    int ExitCode{1};

    try {
        if (Initialize()) {
            ExitCode = mEngineLoop.Run(*this, mPlatform);
        }
    } catch (...) {
        Shutdown();
        throw;
    }

    Shutdown();

    return ExitCode;
}

bool FApplication::Initialize() {
    const HACCEL AcceleratorTable{LoadAccelerators(mInstance, MAKEINTRESOURCE(IDC_MACAW))};

    if (!mPlatform.Initialize(mInstance, mShowCommand, *this, LoadIcon(mInstance, MAKEINTRESOURCE(IDI_MACAW)), LoadIcon(mInstance, MAKEINTRESOURCE(IDI_SMALL)), AcceleratorTable, static_cast<int>(mLoadingWindowWidth), static_cast<int>(mLoadingWindowHeight))) {
        return false;
    }

#ifdef OBJ_VIEWER
    constexpr EEditorMode Mode{EEditorMode::Viewer};
#else
    constexpr EEditorMode Mode{EEditorMode::Editor};
#endif

    if (!mEngine.InitializeEditor(mPlatform.GetWindowHandle(), mLoadingWindowWidth, mLoadingWindowHeight, mPlatform, Mode)) {
        return false;
    }

    RestoreGameWindow();
    Console::AddLog(Console::STDOutHandle, ELogLevel::Log, ELogCategory::Etc, "Macaw Engine Initialized.");

    return true;
}

void FApplication::Tick(float DeltaTime) {
    mEngine.TickFrame(DeltaTime, [this]() {
        DrawTitleBar();
    });

    EnableExternalDropsForImGuiViewports();
}

void FApplication::Shutdown() {
    mEngine.Shutdown();
    mPlatform.Shutdown();
}
