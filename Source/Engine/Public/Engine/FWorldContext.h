#pragma once

#include "CoreUObject/FObjectRegistry.h"

class UWorld;

class FWorldContext {
public:
    FWorldContext();
    ~FWorldContext();

public:
    FObjectRegistry& GetRegistry();
    const FObjectRegistry& GetRegistry() const;

    UWorld* GetWorld() const;

private:
    FObjectRegistry mRegistry{};
    std::unique_ptr<UWorld> mWorld{};
};
