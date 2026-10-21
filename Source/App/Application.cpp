#include "Application.h"
#include "CoreUObject/UObject.h"
#include "Core/Base/ErrorHandler.h"
#include "Core/Memory.h"

FApplication::FApplication() = default;

FApplication::~FApplication() {
    Shutdown();
}

int FApplication::Run() {
    if (!mPlatform.Create(L"Macaw", 1440, 900)) {
        ErrorHandler::Fatal("Application", "Window creation failed");
    }

    if (!mRenderer.Initialize(mPlatform.GetWindowHandle(), mPlatform.GetWidth(), mPlatform.GetHeight())) {
        ErrorHandler::Fatal("Application", "Renderer initialization failed");
    }

    if (!mEngine.InitializeEditor(mRenderer, mPlatform)) {
        ErrorHandler::Fatal("Application", "Editor initialization failed");
    }

    while (mPlatform.PumpMessages() && !mEngine.WantsExit()) {
        if (mPlatform.IsMinimized()) {
            MsgWaitForMultipleObjects(0, nullptr, false, 16, QS_ALLINPUT);
            continue;
        }

        mEngine.TickFrame(mTimer.Tick());
        mRenderer.Present();
    }

    mEngine.Shutdown();
    ErrorHandler::Report(FMemory::GetStats().mHeapBytes != 0, "Application", "UObject allocations remain after editor shutdown", ErrorHandler::EErrorLevel::Critical);
    Shutdown();

    return 0;
}

void FApplication::Shutdown() {
    mEngine.Shutdown();
    mRenderer.Shutdown();
    mPlatform.Shutdown();
}
