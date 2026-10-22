#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/Component/UActorComponent.h"
#include "Core/Math/FTransform.h"

class USceneComponent : public UActorComponent {
public:
    ReflectTypes(USceneComponent, UActorComponent);

public:
    USceneComponent();
    ~USceneComponent() override;

public:
    ReflectClass(USceneComponent);

    const FTransform& GetTransform() const;
    virtual bool SetTransform(const FTransform& Transform);

    FMatrix GetWorldMatrix() const;

private:
    ReflectMembers();

private:
    FTransform mTransform{};
    ReflectAccessor(mTransform, GetTransform, SetTransform, "Transform", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
