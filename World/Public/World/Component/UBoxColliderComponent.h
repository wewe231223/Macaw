#pragma once
#include "World/Component/UCollisionComponent.h"
#include "World/Component/UMeshComponent.h"
#include "World/Component/UBoxColliderComponent.generated.h"

USTRUCT()
struct FBoxColliderBounds {
public:
    GENERATED_TYPES(FBoxColliderBounds)

public:
    GENERATED_BODY(FBoxColliderBounds)

    DirectX::BoundingOrientedBox ToNative() const;
    void SetNative(const DirectX::BoundingOrientedBox& Bounds);

public:
    UPROPERTY(Serialize, Name = "OBB_Center", ReadOnly)
    FVector3 mCenter{};
    UPROPERTY(Edit, Serialize, Name = "OBB_Extent", DisplayName = "Extent", Min = 0.001f, Speed = 0.05f)
    FVector3 mExtent{1.0f, 1.0f, 1.0f};
    UPROPERTY(Serialize, Name = "OBB_Orientation", ReadOnly)
    FQuat mOrientation{0.0f, 0.0f, 0.0f, 1.0f};
};

UCLASS()
class UBoxColliderComponent final : public UCollisionComponent {
public:
    GENERATED_TYPES(UBoxColliderComponent)

public:
    /// <summary>플레이 초기화에서 메시 기준 충돌 경계를 구성하도록 초기화 호출을 요청합니다.</summary>
    UBoxColliderComponent();
    ~UBoxColliderComponent() override = default;

public:
    GENERATED_BODY(UBoxColliderComponent)

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
    UPROPERTY(Edit, Serialize, Name = "GuidMeshComponent", DisplayName = "Source Mesh Component", Category = "Collision", Reference, OwnerOnly, Getter = GetMeshComponent, Setter = SetMeshComponent, PendingGuid = mPendingMeshComponentGuid)
    TObjectRef<UMeshComponent> mMeshComponent{};
    FGuid mPendingMeshComponentGuid{};
    UPROPERTY(Edit, Serialize, Inline, Name = "Bounds", DisplayName = "Extent", Category = "Collision", Getter = GetBounds, Setter = SetBounds)
    FBoxColliderBounds mBounds{};
};
