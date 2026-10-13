#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Component/UCollisionComponent.h"
#include "World/Component/UMeshComponent.h"

class FProperty;

struct FBoxColliderBounds {
public:
    ReflectTypes(FBoxColliderBounds);

public:
    ReflectStruct(FBoxColliderBounds);

    DirectX::BoundingOrientedBox ToNative() const;
    void SetNative(const DirectX::BoundingOrientedBox& Bounds);

public:
    ReflectReadOnlyProperty(mCenter, "OBB_Center", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mCenter));
    FVector3 mCenter{};
    ReflectProperty(mExtent, "OBB_Extent", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mExtent), ReflectEditor("", "Extent"), ReflectRange(0.001f, std::numeric_limits<float>::max()), ReflectSpeed(0.05f));
    FVector3 mExtent{1.0f, 1.0f, 1.0f};
    ReflectReadOnlyProperty(mOrientation, "OBB_Orientation", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mOrientation));
    FQuat mOrientation{0.0f, 0.0f, 0.0f, 1.0f};

    ReflectMembers();
};

class UBoxColliderComponent final : public UCollisionComponent {
public:
    ReflectTypes(UBoxColliderComponent, UCollisionComponent);

public:
    /// <summary>플레이 초기화에서 메시 기준 충돌 경계를 구성하도록 초기화 호출을 요청합니다.</summary>
    UBoxColliderComponent();
    ~UBoxColliderComponent() override = default;

public:
    ReflectClass(UBoxColliderComponent, EClassFlags::None);

    void SetMeshComponent(UMeshComponent* InMeshComponent);
    UMeshComponent* GetMeshComponent() const override;
    bool BuildBoundsFromMesh();

    bool RaycastBounds(const FRay& Ray, float& OutDistance) const override;
    void DrawEditorBounds(ILineDrawContext* LineContext, ELineDepthMode DepthMode) const override;
    FVector3 GetExtent() const;
    void SetExtent(const FVector3& InExtent);
    const FBoxColliderBounds& GetBounds() const;
    void SetBounds(const FBoxColliderBounds& Bounds);

    bool ResolveLoadedReferences() override;
    void InitializeComponent() override;

public:
    void Serialize(FArchive& Archive) override;

private:
    static void SerializeMeshComponentProperty(void* Container, FArchive& Archive, const FProperty& Property);

private:
    ReflectAccessor(mMeshComponent, GetMeshComponent, SetMeshComponent, "GuidMeshComponent", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Reference | EPropertyFlags::OwnerOnly, &SerializeMeshComponentProperty, ReflectEditor("Collision", "Source Mesh Component"));
    TObjectRef<UMeshComponent> mMeshComponent{};
    FGuid mPendingMeshComponentGuid{};
    ReflectAccessor(mBounds, GetBounds, SetBounds, "Bounds", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Inline, ReflectSerializer(mBounds), ReflectEditor("Collision", "Extent"));
    FBoxColliderBounds mBounds{};

    ReflectMembers();
};
