#pragma once
#include "World/Component/UCollisionComponent.h"
#include "World/Component/UMeshComponent.h"

class UBoxColliderComponent final : public UCollisionComponent {
public:
    /// <summary>플레이 초기화에서 메시 기준 충돌 경계를 구성하도록 초기화 호출을 요청합니다.</summary>
    UBoxColliderComponent();
    ~UBoxColliderComponent() override = default;

public:
    JG_DECLARE_DERIVED_TYPEINFO(UBoxColliderComponent, UCollisionComponent)

    void SetMeshComponent(UMeshComponent* InMeshComponent);
    UMeshComponent* GetMeshComponent() const override;
    bool BuildBoundsFromMesh();

    bool RaycastBounds(const FRay& Ray, float& OutDistance) const override;
    void DrawEditorBounds(ILineDrawContext* LineContext, ELineDepthMode DepthMode) const override;
    FVector3 GetExtent() const;
    void SetExtent(const FVector3& InExtent);

    bool ResolveLoadedReferences() override;
    void InitializeComponent() override;

protected:
    void Serialize(FArchive& Archive) override;

private:
    TObjectRef<UMeshComponent> mMeshComponent{};
    FGuid mPendingMeshComponentGuid{};
    DirectX::BoundingOrientedBox mObb{DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f}, DirectX::XMFLOAT3{1.0f, 1.0f, 1.0f}, DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 1.0f}};
};
