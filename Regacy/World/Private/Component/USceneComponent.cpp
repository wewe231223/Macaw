#include "pch.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include "CoreUObject/Reflection/UClass.h"
#include "World/Component/USceneComponent.h"
#include "World/AActor.h"
#include "World/UWorld.h"

const FTransform& USceneComponent::GetRelativeTransform() const {
    return mTransform;
}

void USceneComponent::SetRelativeTransform(const FTransform& Transform) {
    if (mTransform == Transform) {
        return;
    }

    mTransform = Transform;
    UpdateComponentTransform();
}

bool USceneComponent::IsAbsoluteLocation() const {
    return mAbsoluteLocation;
}

bool USceneComponent::IsAbsoluteRotation() const {
    return mAbsoluteRotation;
}

bool USceneComponent::IsAbsoluteScale() const {
    return mAbsoluteScale;
}

void USceneComponent::SetAbsoluteLocation(bool AbsoluteLocation) {
    SetAbsolute(AbsoluteLocation, mAbsoluteRotation, mAbsoluteScale);
}

void USceneComponent::SetAbsoluteRotation(bool AbsoluteRotation) {
    SetAbsolute(mAbsoluteLocation, AbsoluteRotation, mAbsoluteScale);
}

void USceneComponent::SetAbsoluteScale(bool AbsoluteScale) {
    SetAbsolute(mAbsoluteLocation, mAbsoluteRotation, AbsoluteScale);
}

void USceneComponent::SetAbsolute(bool AbsoluteLocation, bool AbsoluteRotation, bool AbsoluteScale) {
    if (mAbsoluteLocation == AbsoluteLocation && mAbsoluteRotation == AbsoluteRotation && mAbsoluteScale == AbsoluteScale) {
        return;
    }

    mAbsoluteLocation = AbsoluteLocation;
    mAbsoluteRotation = AbsoluteRotation;
    mAbsoluteScale = AbsoluteScale;
    UpdateComponentTransform();
}

bool USceneComponent::CalculateRelativeTransform(const FTransform& WorldTransform, const USceneComponent* Parent, FTransform& OutRelative) const {
    FTransform RelativeTransform{WorldTransform};

    if (Parent != nullptr) {
        FTransform ParentTransform{Parent->GetComponentTransform()};

        if (mAbsoluteLocation && mAbsoluteScale) {
            ParentTransform.SetScale(FVector3{1.0f, 1.0f, 1.0f});
        }

        if (!WorldTransform.MakeRelativeTo(ParentTransform, RelativeTransform)) {
            return false;
        }

        if (mAbsoluteLocation) {
            RelativeTransform.SetPosition(WorldTransform.GetPosition());
        }

        if (mAbsoluteRotation) {
            RelativeTransform.SetRotation(WorldTransform.GetRotationQuaternion());
        }

        if (mAbsoluteScale) {
            RelativeTransform.SetScale(WorldTransform.GetScale());
        }
    }

    OutRelative = RelativeTransform;

    return true;
}

void USceneComponent::SetRelativeLocation(const FVector3& Location) {
    FTransform Transform{mTransform};

    Transform.SetPosition(Location);
    SetRelativeTransform(Transform);
}

void USceneComponent::SetRelativeLocationAndRotation(const FVector3& Location, const FRotator& Rotation) {
    FTransform Transform{mTransform};

    Transform.SetPosition(Location);
    Transform.SetRotation(Rotation);
    SetRelativeTransform(Transform);
}

FVector3 USceneComponent::GetRelativeLocation() const {
    return mTransform.GetPosition();
}

void USceneComponent::SetRelativeRotation(const FRotator& Rotation) {
    FTransform Transform{mTransform};

    Transform.SetRotation(Rotation);
    SetRelativeTransform(Transform);
}

FRotator USceneComponent::GetRelativeRotation() const {
    return mTransform.GetRotation();
}

void USceneComponent::SetRelativeScale3D(const FVector3& Scale) {
    FTransform Transform{mTransform};

    Transform.SetScale(Scale);
    SetRelativeTransform(Transform);
}

FVector3 USceneComponent::GetRelativeScale3D() const {
    return mTransform.GetScale();
}

void USceneComponent::Serialize(FArchive& Archive) {
    if (Archive.IsLoading()) {
        DetachFromComponent();
    }

    Super::Serialize(Archive);
}

void USceneComponent::DestroyComponent(bool BPromoteChildren) {
    if (DeferComponentDestruction(BPromoteChildren) || mDestroyingHierarchy) {
        return;
    }

    mDestroyingHierarchy = true;

    AActor* Actor{GetOwner()};
    const UWorld::FActorDispatchScope Dispatch{Actor != nullptr ? Actor->GetWorld() : nullptr};
    USceneComponent* ParentComponent{GetParent()};
    USceneComponent* PromotedComponent{};
    const bool WasRoot{Actor != nullptr && Actor->GetRootComponent() == this};
    std::vector<USceneComponent*> ChildrenToDetach{};

    ChildrenToDetach.reserve(mChildren.size());

    for (const TObjectRef<USceneComponent>& ChildRef : mChildren) {
        if (USceneComponent* Child{ChildRef.Get()}) {
            ChildrenToDetach.push_back(Child);

            if (BPromoteChildren && WasRoot && PromotedComponent == nullptr && Child->GetOwner() == Actor) {
                PromotedComponent = Child;
            }
        }
    }

    if (PromotedComponent != nullptr) {
        if (!PromotedComponent->AttachToComponent(ParentComponent, EAttachmentTransformRule::KeepWorldTransform)) {
            PromotedComponent->DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform);
        }

        Actor->SetRootComponent(PromotedComponent);
    }

    for (USceneComponent* Child : ChildrenToDetach) {
        if (Child != PromotedComponent && !Child->AttachToComponent(PromotedComponent != nullptr ? PromotedComponent : ParentComponent, EAttachmentTransformRule::KeepWorldTransform)) {
            Child->DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform);
        }
    }

    DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform);
    Super::DestroyComponent(BPromoteChildren);
}

bool USceneComponent::AttachToComponent(USceneComponent* ParentComponent, EAttachmentTransformRule Rule) {
    if (IsBeingDestroyed() || (ParentComponent != nullptr && (ParentComponent->IsBeingDestroyed() || (ParentComponent->GetOwner() != nullptr && ParentComponent->GetOwner()->IsBeingDestroyed()))) || ParentComponent == this || mParent.Get() == ParentComponent) {
        return false;
    }

    UWorld* World{GetTypedOuter<UWorld>()};
    const UWorld* ParentWorld{ParentComponent != nullptr ? ParentComponent->GetTypedOuter<UWorld>() : nullptr};

    if (World != nullptr && ParentWorld != nullptr && World != ParentWorld) {
        return false;
    }

    for (USceneComponent* Ancestor{ParentComponent}; Ancestor != nullptr; Ancestor = Ancestor->GetParent()) {
        if (Ancestor == this) {
            return false;
        }
    }

    FTransform RelativeTransform{mTransform};

    if (Rule == EAttachmentTransformRule::KeepWorldTransform) {
        const FTransform WorldTransform{GetComponentTransform()};

        if (!CalculateRelativeTransform(WorldTransform, ParentComponent, RelativeTransform)) {
            return false;
        }
    }

    if (USceneComponent* PreviousParent{mParent.Get()}) {
        std::erase_if(PreviousParent->mChildren, [this](const TObjectRef<USceneComponent>& ChildRef) {
            return ChildRef.Get() == this;
        });
    }

    mParent.Set(ParentComponent);

    if (ParentComponent != nullptr) {
        ParentComponent->mChildren.emplace_back(this);
    }

    if (Rule == EAttachmentTransformRule::KeepWorldTransform) {
        mTransform = RelativeTransform;
    }

    UpdateComponentTransform();

    if (World != nullptr) {
        World->MarkStructureDirty();
    }

    return true;
}

bool USceneComponent::DetachFromComponent(EAttachmentTransformRule Rule) {
    return AttachToComponent(nullptr, Rule);
}

bool USceneComponent::SetWorldTransform(const FTransform& WorldTransform) {
    FTransform RelativeTransform{};

    if (!CalculateRelativeTransform(WorldTransform, GetParent(), RelativeTransform)) {
        return false;
    }

    SetRelativeTransform(RelativeTransform);

    return true;
}

bool USceneComponent::SetWorldTransform(const FMatrix& WorldTransform) {
    FVector3 Scale{};
    FQuat Rotation{};
    FVector3 Translation{};

    if (!WorldTransform.Decompose(Scale, Rotation, Translation)) {
        return false;
    }

    return SetWorldTransform(FTransform{Translation, Rotation, Scale});
}

bool USceneComponent::SetWorldLocation(const FVector3& Location) {
    FTransform DesiredWorldTransform{GetComponentTransform()};

    DesiredWorldTransform.SetPosition(Location);

    return SetWorldTransform(DesiredWorldTransform);
}

bool USceneComponent::SetWorldLocationAndRotation(const FVector3& Location, const FRotator& Rotation) {
    FTransform DesiredWorldTransform{GetComponentTransform()};

    DesiredWorldTransform.SetPosition(Location);
    DesiredWorldTransform.SetRotation(Rotation);

    return SetWorldTransform(DesiredWorldTransform);
}

bool USceneComponent::SetWorldRotation(const FRotator& Rotation) {
    FTransform DesiredWorldTransform{GetComponentTransform()};

    DesiredWorldTransform.SetRotation(Rotation);

    return SetWorldTransform(DesiredWorldTransform);
}

bool USceneComponent::SetWorldScale3D(const FVector3& Scale) {
    FTransform DesiredWorldTransform{GetComponentTransform()};

    DesiredWorldTransform.SetScale(Scale);

    return SetWorldTransform(DesiredWorldTransform);
}

USceneComponent* USceneComponent::GetParent() const {
    return mParent.Get();
}

const std::vector<TObjectRef<USceneComponent>>& USceneComponent::GetChildren() const {
    return mChildren;
}

const FTransform& USceneComponent::GetComponentTransform() const {
    return mWorldTransform;
}

Uint64 USceneComponent::GetTransformRevision() const {
    return mTransformRevision;
}

FMatrix USceneComponent::GetComponentToWorld() const {
    return GetComponentTransform().ToMatrixWithScale();
}

FVector3 USceneComponent::GetComponentLocation() const {
    return GetComponentTransform().GetPosition();
}

FRotator USceneComponent::GetComponentRotation() const {
    return GetComponentTransform().GetRotation();
}

FVector3 USceneComponent::GetComponentScale() const {
    return GetComponentTransform().GetScale();
}

bool USceneComponent::ResolveLoadedReferences() {
    if (!UActorComponent::ResolveLoadedReferences()) {
        return false;
    }

    if (!mPendingParentGuid.IsValid()) {
        return true;
    }

    UObject* ResolvedObject{UObjectSystem::Resolve(UObjectSystem::FindHandleByGuid(mPendingParentGuid))};

    if (ResolvedObject == nullptr || !ResolvedObject->GetClass()->IsChildOf(USceneComponent::StaticClass())) {
        return false;
    }

    USceneComponent* ParentComponent{static_cast<USceneComponent*>(ResolvedObject)};

    if (!AttachToComponent(ParentComponent)) {
        return false;
    }

    mPendingParentGuid = {};

    return true;
}

void USceneComponent::OnTransformUpdate() {
    OnRenderStateChanged();
}

void USceneComponent::UpdateComponentTransform() {
    const UWorld::FActorDispatchScope Dispatch{GetTypedOuter<UWorld>()};

    UpdateWorldTransform();
    NotifyTransformUpdate();
}

void USceneComponent::UpdateWorldTransform() {
    const USceneComponent* Parent{GetParent()};

    mWorldTransform = Parent != nullptr ? mTransform.Compose(Parent->mWorldTransform) : mTransform;

    if (mAbsoluteLocation) {
        mWorldTransform.SetPosition(mTransform.GetPosition());
    }

    if (mAbsoluteRotation) {
        mWorldTransform.SetRotation(mTransform.GetRotationQuaternion());
    }

    if (mAbsoluteScale) {
        mWorldTransform.SetScale(mTransform.GetScale());
    }

    ++mTransformRevision;

    for (const TObjectRef<USceneComponent>& ChildRef : mChildren) {
        if (USceneComponent* Child{ChildRef.Get()}) {
            Child->UpdateWorldTransform();
        }
    }
}

void USceneComponent::NotifyTransformUpdate() {
    OnTransformUpdate();

    for (const TObjectRef<USceneComponent>& ChildRef : mChildren) {
        if (USceneComponent* Child{ChildRef.Get()}) {
            Child->NotifyTransformUpdate();
        }
    }
}

USceneComponent::USceneComponent() {
    InitializeClass(StaticClass());
}

bool USceneComponent::SetEditorParent(USceneComponent* Parent) {
    return CanEditParent() && CanSelectParent(Parent) && AttachToComponent(Parent, EAttachmentTransformRule::KeepWorldTransform);
}

bool USceneComponent::CanEditParent() const {
    return !IsBeingDestroyed() && GetOwner() != nullptr && GetOwner()->GetRootComponent() != this;
}

bool USceneComponent::CanSelectParent(USceneComponent* Parent) const {
    if (Parent != nullptr && (Parent->IsBeingDestroyed() || Parent->GetOwner() != GetOwner())) {
        return false;
    }

    for (const USceneComponent* Ancestor{Parent}; Ancestor != nullptr; Ancestor = Ancestor->GetParent()) {
        if (Ancestor == this) {
            return false;
        }
    }

    return true;
}

void USceneComponent::SerializeParentProperty(void* Container, FArchive& Archive, const FProperty& Property) {
    USceneComponent& Object{*static_cast<USceneComponent*>(Container)};

    SerializeObjectReference(Archive, Property.GetSerializationName(), Object.GetParent(), Object.mPendingParentGuid);

    if (Archive.IsLoading()) {
        Object.mParent = {};
    }
}

void USceneComponent::SerializeTransformProperty(void* Container, FArchive& Archive, const FProperty& Property) {
    USceneComponent& Object{*static_cast<USceneComponent*>(Container)};
    FTransform Transform{Object.mTransform};
    bool AbsoluteLocation{Object.mAbsoluteLocation};
    bool AbsoluteRotation{Object.mAbsoluteRotation};
    bool AbsoluteScale{Object.mAbsoluteScale};

    Archive.BeginObjectScope(Property.GetSerializationName());
    Transform.Serialize(Archive);
    Archive.Serialize("bAbsoluteLocation", AbsoluteLocation);
    Archive.Serialize("bAbsoluteRotation", AbsoluteRotation);
    Archive.Serialize("bAbsoluteScale", AbsoluteScale);
    Archive.EndObjectScope();

    if (Archive.IsLoading() && !Archive.HasError()) {
        Object.mTransform = Transform;
        Object.mAbsoluteLocation = AbsoluteLocation;
        Object.mAbsoluteRotation = AbsoluteRotation;
        Object.mAbsoluteScale = AbsoluteScale;
        Object.UpdateComponentTransform();
    }
}
