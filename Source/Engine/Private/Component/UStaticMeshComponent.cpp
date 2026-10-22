#include "Engine/Component/UStaticMeshComponent.h"
#include "Core/Reflection/EPropertyFlags.h"
#include "Engine/EPrimitiveType.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"

UStaticMeshComponent::UStaticMeshComponent() = default;

UStaticMeshComponent::~UStaticMeshComponent() = default;

ImplementReflection(UStaticMeshComponent)

EPrimitiveType UStaticMeshComponent::GetPrimitiveType() const {
    return mPrimitive;
}

bool UStaticMeshComponent::SetPrimitiveType(EPrimitiveType Type) {
    if (Type >= EPrimitiveType::Count) {
        return false;
    }

    if (mPrimitive != Type) {
        mPrimitive = Type;
        MarkRenderStateDirty();
    }

    return true;
}
