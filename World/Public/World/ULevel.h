#pragma once

#include <memory>
#include "Core/STL.h"
#include "CoreUObject/UObject.h"
#include "World/ULevel.generated.h"

class AActor;
class UWorld;

UCLASS(NoFactory)
class ULevel final : public UObject {
public:
    GENERATED_TYPES(ULevel)

public:
    explicit ULevel(UWorld& World);
    ~ULevel() override;

public:
    GENERATED_BODY(ULevel)

    UWorld& GetWorld() const;
    const TArray<std::unique_ptr<AActor>>& GetActors() const;

private:
    bool CanChangeOuter(const UObject* NewOuter) const override;

    friend class UWorld;
    friend class FSceneSerializer;

private:
    UWorld& mWorld;
    UPROPERTY(Name = "Actors", Owned, ReadOnly)
    TArray<std::unique_ptr<AActor>> mActors{};
};
