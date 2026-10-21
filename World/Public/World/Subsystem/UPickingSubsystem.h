#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Subsystem/UWorldSubsystem.h"
#include "CoreUObject/TObjectRef.h"
#include "Core/Spatial/FDynamicBVH8.h"
#include "World/Component/UPrimitiveComponent.h"
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include "World/Component/FMeshPickingProxy.h"

enum class EPickingTarget {
    Primitives,
    MeshSurfaces
};

class FWorldRaycastAccelerationStructure {
private:
    struct FNode {
        DirectX::BoundingBox BoundingBox{};
        Uint32 mLeft{InvalidIndex};
        Uint32 mRight{InvalidIndex};
        TObjectRef<UPrimitiveComponent> mComponent{};
    };

    struct FBuildItem {
        DirectX::BoundingBox BoundingBox{};
        TObjectRef<UPrimitiveComponent> mComponent{};
    };

    struct MinMaxBox {
    public:
        void Expand(const DirectX::BoundingBox& Box);
        float SurfaceArea() const;
        static MinMaxBox Merge(const MinMaxBox& Box1, const MinMaxBox& Box2);

    public:
        float mMinX{std::numeric_limits<float>::max()};
        float mMinY{std::numeric_limits<float>::max()};
        float mMinZ{std::numeric_limits<float>::max()};
        float mMaxX{std::numeric_limits<float>::lowest()};
        float mMaxY{std::numeric_limits<float>::lowest()};
        float mMaxZ{std::numeric_limits<float>::lowest()};
    };

    enum class EProxyKind : Uint8 {
        Box,
        Mesh,
        Billboard
    };

    struct alignas(64) FProxy {
        FMeshPickingProxy mMesh{};
        DirectX::BoundingBox mBox{};
        TObjectRef<UPrimitiveComponent> mComponent{};
        FVector2 mBillboardSize{};
        EProxyKind mKind{EProxyKind::Box};
        bool mEnabled{}, mHasCustomBox{};
    };

    static_assert(sizeof(FProxy) == 128);

public:
    bool BuildStructure(const TArray<TObjectRef<UPrimitiveComponent>>& Components);
    bool Raycast(const FRay& Ray, const FWorldRaycastQuery& Query, FWorldRaycastHit& OutHit, EPickingTarget Target = EPickingTarget::Primitives, const FMatrix* CameraWorld = nullptr) const;
    void WarmupRaycast() const;
    void Clear();

    bool IsBuilt() const;

    void InsertComponent(UPrimitiveComponent* Component);
    void UpdateComponent(UPrimitiveComponent* Component);
    void RemoveComponent(UPrimitiveComponent* Component);

private:
    Uint32 MakeChild(TArray<FNode>& BuildNodes, TArray<FBuildItem>& Items, Uint32 First, Uint32 Count, const MinMaxBox& Bounds);
    static Uint64 ComponentKey(FObjectHandle Handle);

    FProxy MakeProxy(UPrimitiveComponent* Component, DirectX::BoundingBox& Bounds);
    bool RaycastProxy(const FProxy& Proxy, const FRay& Ray, float MaxDistance, FWorldRaycastHit& OutHit, const FMatrix* CameraWorld, EPickingTarget Target = EPickingTarget::Primitives) const;

private:
    static constexpr Uint32 Slice{32};
    static constexpr Uint32 InvalidIndex{std::numeric_limits<Uint32>::max()};
    std::unordered_map<Uint64, DirectX::BoundingOrientedBox> mLocalBoxes{};
    TArray<FProxy> mLeaves{};
    TArray<Uint32> mFreeLeaves{};
    std::unordered_map<Uint64, Uint32> mLeafIndices{};
    FDynamicBVH8 mTree{};
    bool mBuilt{false};
};

/// <summary>Provides editor picking over registered primitive component volumes.</summary>
class UPickingSubsystem : public UWorldSubsystem {
public:
    ReflectTypes(UPickingSubsystem, UWorldSubsystem);

public:
    UPickingSubsystem();
    ~UPickingSubsystem() override = default;

public:
    ReflectClass(UPickingSubsystem);

    void RegisterComponent(UPrimitiveComponent* Component);
    void UnregisterComponent(UPrimitiveComponent* Component);
    bool RebuildAccelerationStructure();
    void UpdateComponent(UPrimitiveComponent* Component);
    void SynchronizeProxies() const;
    bool Raycast(const FRay& Ray, const FWorldRaycastQuery& Query, FWorldRaycastHit& OutHit, EPickingTarget Target = EPickingTarget::Primitives, const FMatrix* CameraWorld = nullptr) const;

    bool ContainsComponent(const UPrimitiveComponent* Component) const;
    const TArray<TObjectRef<UPrimitiveComponent>>& GetRegisteredComponents() const;

protected:
    void OnDeinitialize() override;

private:
    TArray<TObjectRef<UPrimitiveComponent>> mComponents{};
    std::unordered_set<Uint64> mRegisteredComponentKeys{};
    mutable FWorldRaycastAccelerationStructure mRaycastAccelerationStructure{};
    mutable TArray<TObjectRef<UPrimitiveComponent>> mDirtyComponents;
    mutable std::unordered_set<Uint64> mDirtyComponentKeys;
};
