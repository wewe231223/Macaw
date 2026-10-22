#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/UObject.h"

class UWorld;
class AActor;

class ULevel final : public UObject {
public:
    ReflectTypes(ULevel, UObject);

public:
    ULevel();
    ~ULevel() override;

public:
    ReflectClass(ULevel);

    void SetWorld(UWorld* World);

    bool AddActor(std::unique_ptr<AActor> Actor);
    bool DestroyActor(FObjectHandle Handle);

    const TArray<std::unique_ptr<AActor>>& GetActors() const;

private:
    ReflectMembers();

private:
    UWorld* mWorld{};
    ReflectProperty(mWorld, "World", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    TArray<std::unique_ptr<AActor>> mActors{};
    ReflectProperty(mActors, "Actors", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
