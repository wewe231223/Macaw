#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Component/UActorComponent.h"
#include "World/Component/USceneComponent.h"
#include "RenderCore/FRenderData.h"
#include "RenderCore/FPrimitiveSceneProxy.h"

#include <memory>

class UPrimitiveComponent : public USceneComponent {
public:
    ReflectTypes(UPrimitiveComponent, USceneComponent);

public:
    UPrimitiveComponent();
    ~UPrimitiveComponent() override = default;

public:
    ReflectClass(UPrimitiveComponent, EClassFlags::Abstract);

    bool IsVisible() const;
    void SetVisible(bool BInVisible);

    void OnRenderStateChanged() override;
    void OnRegister() override;
    void OnUnregister() override;

    virtual std::unique_ptr<FPrimitiveSceneProxy> CreateSceneProxy() const;
    void CreateRenderState() override;
    void DestroyRenderState() override;
    void SendRenderTransform() override;
    void SendRenderDynamicData() override;
    bool SetUVTransform(FName MaterialSlot, const FUVTransform& Transform, FObjectHandle SourceHandle = {});
    void ClearUVTransform(FName MaterialSlot, FObjectHandle SourceHandle = {});
    const TArray<FPrimitiveUVTransform>& GetUVTransforms() const;
    FPrimitiveTransform GetRenderTransform() const;

    void UpdateBounds();

    void SetPickingBox(const DirectX::BoundingOrientedBox& Box);

    const DirectX::BoundingOrientedBox& GetPickingBox() const;

    void BuildBoundsFromOBB();

    const DirectX::BoundingSphere& GetBoundingSphere() const;

    const DirectX::BoundingBox& GetWorldAABB() const;
    const DirectX::BoundingOrientedBox& GetWorldOBB() const;
    const DirectX::BoundingSphere& GetWorldSphere() const;

    virtual void OnTransformUpdate() override;

    void Serialize(FArchive& Archive) override;

private:
    TArray<FPrimitiveUVTransform> mUVTransforms{};
    ReflectAccessor(mBVisible, IsVisible, SetVisible, "bVisible", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mBVisible), ReflectEditor("Rendering", "Visible"));
    bool mBVisible{true};

    DirectX::BoundingBox mLocalAABB{};
    DirectX::BoundingOrientedBox mPickingBox{DirectX::XMFLOAT3{0.f, 0.f, 0.f}, DirectX::XMFLOAT3{0.f, 0.f, 0.f}, DirectX::XMFLOAT4{0.f, 0.f, 0.f, 1.f}};
    DirectX::BoundingSphere mLocalSphere{};

    DirectX::BoundingBox mWorldAABB{};
    DirectX::BoundingOrientedBox mWorldOBB{};
    DirectX::BoundingSphere mWorldSphere{};

    Uint64 mWorldBoundsTransformRevision{};

    ReflectMembers();
};
