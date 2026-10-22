#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/FObjectRegistry.h"

class UWorld;

class FWorldContext {
public:
    ReflectTypes(FWorldContext);

public:
    FWorldContext();
    ~FWorldContext();

public:
    ReflectStruct(FWorldContext);

    FObjectRegistry& GetRegistry();
    const FObjectRegistry& GetRegistry() const;

    UWorld* GetWorld() const;

private:
    ReflectMembers();

private:
    FObjectRegistry mRegistry{};
    ReflectProperty(mRegistry, "Registry", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    std::unique_ptr<UWorld> mWorld{};
    ReflectProperty(mWorld, "World", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
