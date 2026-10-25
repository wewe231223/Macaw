#pragma once

#include "Engine/AActor.h"

class UBillboardTextComponent;

class ABillboardTextActor final : public AActor {
public:
    ReflectTypes(ABillboardTextActor, AActor);

public:
    ABillboardTextActor();
    ~ABillboardTextActor() override;

public:
    ReflectClass(ABillboardTextActor);

    bool InitializeDefaultComponents() override;
    UBillboardTextComponent* GetTextComponent() const;

private:
    ReflectMembers();
};
