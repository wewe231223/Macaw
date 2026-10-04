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
    JG_DECLARE_DERIVED_TYPEINFO(ULevel, UObject)

    UWorld& GetWorld() const;
    const TArray<std::unique_ptr<AActor>>& GetActors() const;

private:
    friend class UWorld;

private:
    UWorld& mWorld;
    TArray<std::unique_ptr<AActor>> mActors{};
};
