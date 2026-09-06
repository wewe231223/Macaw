#pragma once

#include "UActorComponent.h"
#include "USceneComponent.h"

class UPrimitiveComponent : public USceneComponent
{
public:
    UPrimitiveComponent() = default;
    ~UPrimitiveComponent() override = default;

    bool IsVisible() const;
    void SetVisible(bool bInVisible);

private:
    bool bVisible = true;
};