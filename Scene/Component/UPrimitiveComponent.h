#pragma once

#include "UActorComponent.h"
#include "USceneComponent.h"

struct FRenderProbe;

class UPrimitiveComponent : public USceneComponent
{
public:
    UPrimitiveComponent() = default;
    ~UPrimitiveComponent() override = default;

    bool IsVisible() const;
    void SetVisible(bool bInVisible);

    virtual void MakeRender(FRenderProbe& OutProbe) const = 0;

private:
    bool bVisible = true;
};
