#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include <memory>
#include "Core/STL.h"
#include "CoreUObject/UObject.h"
#include "World/AActor.h"

class UWorld;

class ULevel final : public UObject {
public:
    ReflectTypes(ULevel, UObject);

public:
    explicit ULevel(UWorld& World);
    ~ULevel() override;

public:
    ReflectClass(ULevel, EClassFlags::NoFactory);

    UWorld& GetWorld() const;
    const TArray<std::unique_ptr<AActor>>& GetActors() const;

private:
    bool CanChangeOuter(const UObject* NewOuter) const override;

    friend class UWorld;
    friend class FSceneSerializer;

private:
    UWorld& mWorld;
    ReflectReadOnlyProperty(mActors, "Actors", EPropertyFlags::ReadOnly | EPropertyFlags::Owned);
    TArray<std::unique_ptr<AActor>> mActors{};

    ReflectMembers();
};
