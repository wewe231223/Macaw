#pragma once

#include <memory>
#include "Core/STL.h"
#include "CoreUObject/UObject.h"

class AActor;
class UWorld;

class ULevel final : public UObject {
public:
    explicit ULevel(UWorld& World);
    ~ULevel() override;

public:
    JG_DECLARE_NON_CREATABLE_DERIVED_TYPEINFO(ULevel, UObject)

    UWorld& GetWorld() const;
    const TArray<std::unique_ptr<AActor>>& GetActors() const;

private:
    bool CanChangeOuter(const UObject* NewOuter) const override;

    friend class UWorld;
    friend class FSceneSerializer;

private:
    UWorld& mWorld;
    TArray<std::unique_ptr<AActor>> mActors{};
};
