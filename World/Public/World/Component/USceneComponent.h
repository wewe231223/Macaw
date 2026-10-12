#pragma once
#include "Core/Base/FTransform.h"
#include "CoreUObject/TObjectRef.h"
#include "Core/Archive/FArchive.h"
#include "World/Component/UActorComponent.h"
#include "World/Component/USceneComponent.generated.h"

enum class EAttachmentTransformRule {
    KeepRelativeTransform,
    KeepWorldTransform
};

UCLASS()
class USceneComponent : public UActorComponent {
public:
    GENERATED_TYPES(USceneComponent)

public:
    USceneComponent();
    ~USceneComponent() override = default;

public:
    virtual void OnUnregister() override;
    void DestroyComponent(bool BPromoteChildren = false) override;
    void RemoveChild(USceneComponent* InChild);

    GENERATED_BODY(USceneComponent)

    const FTransform& GetRelativeTransform() const;
    void SetRelativeTransform(const FTransform& Transform);

    void SetRelativeLocation(const FVector3& Location);
    void SetRelativeLocationAndRotation(const FVector3& Location, const FRotator& Rotation);
    FVector3 GetRelativeLocation() const;

    void SetRelativeRotation(const FRotator& Rotation);
    FRotator GetRelativeRotation() const;

    void SetRelativeScale3D(const FVector3& Scale);
    FVector3 GetRelativeScale3D() const;

    bool AttachToComponent(USceneComponent* Parent, EAttachmentTransformRule Rule = EAttachmentTransformRule::KeepRelativeTransform);
    bool DetachFromComponent(EAttachmentTransformRule Rule = EAttachmentTransformRule::KeepRelativeTransform);

    bool SetWorldTransform(const FTransform& WorldTransform);
    bool SetWorldTransform(const FMatrix& WorldTransform);
    bool SetWorldLocation(const FVector3& Location);
    bool SetWorldLocationAndRotation(const FVector3& Location, const FRotator& Rotation);
    bool SetWorldRotation(const FRotator& Rotation);
    bool SetWorldScale3D(const FVector3& Scale);

    const FTransform& GetComponentTransform() const;
    Uint64 GetTransformRevision() const;
    FMatrix GetComponentToWorld() const;
    FVector3 GetComponentLocation() const;
    FRotator GetComponentRotation() const;
    FVector3 GetComponentScale() const;

    USceneComponent* GetParent() const;
    bool SetEditorParent(USceneComponent* Parent);
    bool CanEditParent() const;
    bool CanSelectParent(USceneComponent* Parent) const;
    const std::vector<TObjectRef<USceneComponent>>& GetChildren() const;

    // PrimitiveComponent 에서 호출될 world bound 업데이트 위한 함수
    virtual void OnTransformUpdate();

    void Serialize(FArchive& Archive) override;
    bool ResolveLoadedReferences() override;

private:
    void MarkTransformDirty();
    void NotifyTransformUpdate();

private:
    // 부모와 자식 연결을 정리하는 동안 파괴 처리의 중복 진입을 막습니다.
    bool mDestroyingHierarchy{};
    UPROPERTY(Edit, Serialize, Name = "Transform", Category = "Transform", DisplayName = "Relative Transform", Getter = GetRelativeTransform, Setter = SetRelativeTransform)
    FTransform mTransform{};
    mutable FTransform mWorldTransform{};
    mutable FObjectHandle mCachedParentHandle{};
    mutable Uint64 mCachedParentRevision{};
    mutable Uint64 mTransformRevision{};
    mutable bool mWorldTransformDirty{true};
    FGuid mPendingParentGuid{};

    UPROPERTY(Edit, Serialize, Name = "Parent", Reference, OwnerOnly, Category = "Attachment", Getter = GetParent, Setter = SetEditorParent, EditCondition = CanEditParent, ObjectFilter = CanSelectParent, PendingGuid = mPendingParentGuid)
    TObjectRef<USceneComponent> mParent{};
    UPROPERTY(Name = "Children", Reference, ReadOnly)
    std::vector<TObjectRef<USceneComponent>> mChildren{};
};
