#include "pch.h"
#include "Core/Base/ErrorHandler.h"
#include "Core/Base/FScopedFlag.h"
#include "Engine/FEngineLoop.h"
#include "Engine/IEngineLoopClient.h"
#include "Platform/IPlatformApplication.h"

int FEngineLoop::Run(IEngineLoopClient& Client, IPlatformApplication& Platform) {
    if (mClient != nullptr) {
        ErrorHandler::Report("FEngineLoop", "Engine loop is already active", ErrorHandler::EErrorLevel::Critical);
    }

    mClient = &Client;
    mFrameTimer.Reset();
    mRunning = true;

    try {
        while (Platform.PumpMessages()) {
            Tick();
        }
    } catch (...) {
        mRunning = false;
        mClient = nullptr;
        throw;
    }

    mRunning = false;
    mClient = nullptr;

    return Platform.GetExitCode();
}

void FEngineLoop::Tick() {
    if (!mRunning || mTicking) {
        return;
    }

    const FScopedFlag TickScope{mTicking};

    mFrameTimer.Tick();
    mClient->Tick(static_cast<float>(mFrameTimer.GetUpdateDeltaSeconds()));
}

bool FEngineLoop::IsRunning() const {
    return mRunning;
}
