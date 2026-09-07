#include "PCH.h"

#include "USceneComponent.h"

FTransform& USceneComponent::GetTransform()
{
    return Transform;
}

const FTransform& USceneComponent::GetTransform() const
{
    return Transform;
}

void USceneComponent::Serialize(FArchive& Archive)
{
    UActorComponent::Serialize(Archive);

    Archive.SerializeStruct("Transform", Transform);
}