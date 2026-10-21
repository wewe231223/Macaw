#pragma once

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
    TArray<std::unique_ptr<AActor>> mActors{};
};
