#pragma once

#include "World/Tick/FTickFunction.h"

class UActorComponent;

class FActorComponentTickFunction final : public FTickFunction {
public:
    explicit FActorComponentTickFunction(UActorComponent& Target);
    ~FActorComponentTickFunction() override = default;

public:
    bool CanTick(ETickType TickType) const override;
    void ExecuteTick(float DeltaTime) override;

private:
    UActorComponent* mTarget{};
};
