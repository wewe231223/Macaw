#pragma once

class IEngineLoopClient {
public:
    virtual ~IEngineLoopClient() = default;

public:
    virtual void Tick(float DeltaTime) = 0;
};
