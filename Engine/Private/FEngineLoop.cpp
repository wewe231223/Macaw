#include "pch.h"
#include "Engine/FEngineLoop.h"
#include "Engine/IEngineLoopClient.h"
#include "Platform/IPlatformApplication.h"
#include <stdexcept>

int FEngineLoop::Run(IEngineLoopClient& Client, IPlatformApplication& Platform) {
    if (mClient != nullptr) {
        throw std::logic_error{"Engine loop is already active"};
    }
    mClient = &Client;
    int ExitCode{1};
    try {
        if (Client.Initialize()) {
            mFrameTimer.Reset();
            mRunning = true;
            while (Platform.PumpMessages()) {
                Tick();
            }
            ExitCode = Platform.GetExitCode();
        }
    } catch (...) {
        mRunning = false;
        mClient = nullptr;
        Client.Shutdown();
        throw;
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
    try {
        mClient->Tick(static_cast<float>(mFrameTimer.GetUpdateDeltaSeconds()));
    } catch (...) {
        mTicking = false;
        throw;
    }
    mTicking = false;
}

bool FEngineLoop::IsRunning() const {
    return mRunning;
}
