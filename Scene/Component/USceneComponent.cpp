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

void USceneComponent::OnDestroy()
{
    for (USceneComponent* Child : Children)
    {
        if (Child)
        {
            Child->Parent = nullptr;
            Child->OnDestroy();
        }
    }
    Children.clear();

    if (Parent)
    {
        Parent->RemoveChild(this);
        Parent = nullptr;
    }

    UActorComponent::OnDestroy();
}

void USceneComponent::RemoveChild(USceneComponent* InChild)
{
    if (!InChild) return;

    auto It = std::find(Children.begin(), Children.end(), InChild);
    if (It != Children.end())
    {
        Children.erase(It);
        InChild->Parent = nullptr; 
    }
}

void USceneComponent::AttachTo(USceneComponent* InParent)
{
    Parent = InParent;

    if (Parent != nullptr)
    {
        Parent->Children.push_back(this);
    }
}

USceneComponent* USceneComponent::GetParent() const
{
    return Parent;
}

const std::vector<USceneComponent*>& USceneComponent::GetChildren() const
{
    return Children;
}

FMatrix USceneComponent::GetWorldMatrix() const
{
    FMatrix LocalMatrix = Transform.GetWorldMatrix();

    if (Parent == nullptr)
    {
        return LocalMatrix;
    }

    return LocalMatrix * Parent->GetWorldMatrix();
}


void USceneComponent::Serialize(FArchive& Archive)
{
	UActorComponent::Serialize(Archive);
	Archive.SerializeStruct("Transform", Transform);
}
