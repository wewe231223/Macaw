#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "Core/Base/FTransform.h"
#include "CoreUObject/TObjectRef.h"
#include "Core/Archive/FArchive.h"
#include "World/Component/UActorComponent.h"

class FProperty;

enum class EAttachmentTransformRule {
    KeepRelativeTransform,
    KeepWorldTransform
};

class USceneComponent : public UActorComponent {
public:
    ReflectTypes(USceneComponent, UActorComponent);

public:
    USceneComponent();
    ~USceneComponent() override = default;

public:
    void DestroyComponent(bool BPromoteChildren = false) override;

    ReflectClass(USceneComponent, EClassFlags::None);

    const FTransform& GetRelativeTransform() const;
    void SetRelativeTransform(const FTransform& Transform);

    bool IsAbsoluteLocation() const;
    bool IsAbsoluteRotation() const;
    bool IsAbsoluteScale() const;
    void SetAbsoluteLocation(bool AbsoluteLocation);
    void SetAbsoluteRotation(bool AbsoluteRotation);
    void SetAbsoluteScale(bool AbsoluteScale);
    void SetAbsolute(bool AbsoluteLocation, bool AbsoluteRotation, bool AbsoluteScale);
    bool CalculateRelativeTransform(const FTransform& WorldTransform, const USceneComponent* Parent, FTransform& OutRelative) const;

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
    friend class FWorldSerializer;

    static void SerializeTransformProperty(void* Container, FArchive& Archive, const FProperty& Property);
    static void SerializeParentProperty(void* Container, FArchive& Archive, const FProperty& Property);
    void UpdateComponentTransform();
    void UpdateWorldTransform();
    void NotifyTransformUpdate();

private:
    // 부모와 자식 연결을 정리하는 동안 파괴 처리의 중복 진입을 막습니다.
    bool mDestroyingHierarchy{};
    ReflectAccessor(mTransform, GetRelativeTransform, SetRelativeTransform, "Transform", EPropertyFlags::Edit | EPropertyFlags::Serialize, &SerializeTransformProperty, ReflectEditor("Transform", "Relative Transform"));
    FTransform mTransform{};
    FTransform mWorldTransform{};
    Uint64 mTransformRevision{1};
    ReflectAccessor(mAbsoluteLocation, IsAbsoluteLocation, SetAbsoluteLocation, "bAbsoluteLocation", EPropertyFlags::Edit, ReflectEditor("Transform", "Absolute Location"));
    bool mAbsoluteLocation{};
    ReflectAccessor(mAbsoluteRotation, IsAbsoluteRotation, SetAbsoluteRotation, "bAbsoluteRotation", EPropertyFlags::Edit, ReflectEditor("Transform", "Absolute Rotation"));
    bool mAbsoluteRotation{};
    ReflectAccessor(mAbsoluteScale, IsAbsoluteScale, SetAbsoluteScale, "bAbsoluteScale", EPropertyFlags::Edit, ReflectEditor("Transform", "Absolute Scale"));
    bool mAbsoluteScale{};
    FGuid mPendingParentGuid{};
    ReflectAccessor(mParent, GetParent, SetEditorParent, "Parent", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Reference | EPropertyFlags::OwnerOnly, &SerializeParentProperty, ReflectEditor("Attachment", "Parent"), ReflectCondition(CanEditParent), ReflectFilter(CanSelectParent));
    TObjectRef<USceneComponent> mParent{};
    ReflectReadOnlyProperty(mChildren, "Children", EPropertyFlags::ReadOnly | EPropertyFlags::Reference);
    std::vector<TObjectRef<USceneComponent>> mChildren{};

    ReflectMembers();
};
