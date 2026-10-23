#pragma once

#include "World/Tick/FTickFunction.h"

class AActor;

class FActorTickFunction final : public FTickFunction {
public:
    explicit FActorTickFunction(AActor& Target);
    ~FActorTickFunction() override = default;

public:
    bool CanTick(ETickType TickType) const override;
    void ExecuteTick(float DeltaTime) override;

private:
    AActor* mTarget{};
};
