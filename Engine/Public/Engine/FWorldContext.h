#pragma once

#include <memory>
#include "World/EWorldType.h"

class UWorld;

class FWorldContext final {
public:
    explicit FWorldContext(EWorldType WorldType);
    ~FWorldContext();
    FWorldContext(const FWorldContext&) = delete;
    FWorldContext& operator=(const FWorldContext&) = delete;
    FWorldContext(FWorldContext&&) = delete;
    FWorldContext& operator=(FWorldContext&&) = delete;

public:
    UWorld& GetWorld();
    const UWorld& GetWorld() const;
    EWorldType GetWorldType() const;

private:
    std::unique_ptr<UWorld> mWorld{};
};
