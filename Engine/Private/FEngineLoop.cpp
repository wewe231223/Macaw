#include "pch.h"
#include "Core/Base/ErrorHandler.h"
#include "Engine/FEngineLoop.h"
#include "Engine/IEngineLoopClient.h"
#include "Platform/IPlatformApplication.h"

int FEngineLoop::Run(IEngineLoopClient& Client, IPlatformApplication& Platform) {
    if (mClient != nullptr) {
        ErrorHandler::Report("FEngineLoop", "Engine loop is already active", ErrorHandler::EErrorLevel::Critical);
    }

    mClient = &Client;

    int ExitCode{1};

    if (Client.Initialize()) {
        mFrameTimer.Reset();
        mRunning = true;

        while (Platform.PumpMessages()) {
            Tick();
        }

        ExitCode = Platform.GetExitCode();
    }

    mRunning = false;
    mClient = nullptr;
    Client.Shutdown();

    return ExitCode;
}

void FEngineLoop::Tick() {
    if (!mRunning || mTicking) {
        return;
    }

    mTicking = true;
    mFrameTimer.Tick();
    mClient->Tick(static_cast<float>(mFrameTimer.GetUpdateDeltaSeconds()));
    mTicking = false;
}

bool FEngineLoop::IsRunning() const {
    return mRunning;
}
