#include "Engine/Component/USceneComponent.h"
#include "Core/Reflection/EPropertyFlags.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FTransform.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"

USceneComponent::USceneComponent() = default;

USceneComponent::~USceneComponent() = default;

ImplementReflection(USceneComponent, EClassFlags::NoFactory)

const FTransform& USceneComponent::GetTransform() const {
    return mTransform;
}

bool USceneComponent::SetTransform(const FTransform& Transform) {
    if (!Transform.IsValid()) {
        return false;
    }

    mTransform = Transform;

    return true;
}

FMatrix USceneComponent::GetWorldMatrix() const {
    return mTransform.ToMatrix();
}
