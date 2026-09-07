#pragma once

#include "UActorComponent.h"
#include "USceneComponent.h"

struct FRenderProbe;

class UPrimitiveComponent : public USceneComponent
{
public:
    UPrimitiveComponent() = default;
    ~UPrimitiveComponent() override = default;

    JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(UPrimitiveComponent, USceneComponent)

    bool IsVisible() const;
    void SetVisible(bool bInVisible);

    virtual void MakeRender(FRenderProbe& OutProbe) const {};

	JG_DECLARE_DERIVED_TYPEINFO(UPrimitiveComponent, USceneComponent);
protected:
    void Serialize(FArchive& Archive) override;

private:
    bool bVisible = true;
};
