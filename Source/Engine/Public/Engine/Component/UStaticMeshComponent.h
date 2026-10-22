#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/EPrimitiveType.h"

class UStaticMeshComponent final : public UPrimitiveComponent {
public:
    ReflectTypes(UStaticMeshComponent, UPrimitiveComponent);

public:
    UStaticMeshComponent();
    ~UStaticMeshComponent() override;

public:
    ReflectClass(UStaticMeshComponent);

    EPrimitiveType GetPrimitiveType() const override;
    bool SetPrimitiveType(EPrimitiveType Type);

private:
    ReflectMembers();

private:
    EPrimitiveType mPrimitive{EPrimitiveType::Cube};
    ReflectAccessor(mPrimitive, GetPrimitiveType, SetPrimitiveType, "Primitive", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
