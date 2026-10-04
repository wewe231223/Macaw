#pragma once

#include "Core/Time/FFrameTimer.h"

class IEngineLoopClient;
class IPlatformApplication;

class FEngineLoop final {
public:
    int Run(IEngineLoopClient& Client, IPlatformApplication& Platform);
    void Tick();
    bool IsRunning() const;

private:
    FFrameTimer mFrameTimer{};
    IEngineLoopClient* mClient{};
    bool mRunning{};
    bool mTicking{};
};
