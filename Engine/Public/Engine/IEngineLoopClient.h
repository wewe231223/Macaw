#pragma once

class IEngineLoopClient {
public:
    virtual ~IEngineLoopClient() = default;

public:
    virtual bool Initialize() = 0;
    virtual void Tick(float DeltaTime) = 0;
    virtual void Shutdown() = 0;
};
