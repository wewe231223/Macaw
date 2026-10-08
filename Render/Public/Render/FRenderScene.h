#pragma once
#include "RenderCore/FRenderData.h"
#include "RenderCore/FSceneUpdateBatch.h"
#include "Render/FBVHTree.h"
#include "Render/FPrimitiveSceneInfo.h"
#include "Render/FMeshDrawCommandCache.h"

#include <deque>
#include <map>
#include <unordered_map>

class IAssetRegistry;
class FRenderAssetResources;

enum class ERenderUpdateMode : Uint8 {
    Partial,
    Full
};

class FRenderScene {
private:
    struct FObjectChange {
        Uint64 mRevision{};
        Uint32 mObjectIndex{};
    };

public:
    explicit FRenderScene(Uint64 SceneId);

public:
    void Synchronize(const IAssetRegistry* Registry, FSceneUpdateBatch& Updates, const FMaterialBuffer& Materials, FRenderAssetResources* Resources = nullptr);

    Uint64 GetId() const;
    Uint64 GetRevision() const;
    const TArray<FMatrix>& GetObjectTransforms() const;

    ERenderUpdateMode CollectChangedObjects(Uint64 SinceRevision, TArray<Uint32>& OutIndices) const;

    const TArray<FPrimitiveSceneInfo>& GetPrimitives() const;
    const FPrimitiveSceneProxy* FindPrimitive(FObjectHandle ComponentHandle) const;
    const TArray<std::shared_ptr<const FMeshDrawCommand>>& GetCachedMeshDrawCommands() const;

    const TArray<FLightShaderParameters>& GetLights() const;
    const FLightSceneProxy* FindLight(FObjectHandle ComponentHandle) const;
    void CollectDynamicMeshElements(FDynamicPrimitiveDrawInterface& DrawInterface) const;

    void QueryFrustum(const FMatrix& ViewProjection, TArray<Uint32>& OutIndices, TArray<Uint32>* OutBoundaryPositions = nullptr) const;

private:
    void ApplyPrimitiveUpdate(FPrimitiveSceneUpdate& Update);
    void ApplyLightUpdate(FLightSceneUpdate& Update);

    Uint32 AddObject(const FPrimitiveSceneProxy& Proxy);
    void UpdateObject(Uint32 ObjectIndex, const FPrimitiveSceneProxy& Proxy);
    void UpdateObjectTransform(Uint32 ObjectIndex, const FPrimitiveTransform& Transform);
    void RemoveObject(Uint32 ObjectIndex);

    void RecordObjectChange(Uint32 ObjectIndex);
    void CommitObjectChanges();

    void RefreshStaticMeshes(const IAssetRegistry* Registry, const FMaterialBuffer& Materials, FRenderAssetResources* Resources);
    void UpdateBounds();
    void RebuildBounds();

private:
    Uint64 mSceneId{};
    Uint64 mRevision{1};
    Uint64 mJournalFloor{};

    bool mTopologyDirty{};
    bool mObjectsChanged{};

    TArray<FPrimitiveSceneInfo> mObjects{};
    TArray<FMatrix> mObjectTransforms{};
    TArray<Uint32> mFreeObjects{};

    std::map<Uint64, std::unique_ptr<FLightSceneProxy>> mLights{};
    TArray<FLightShaderParameters> mLightParameters{};

    std::unordered_map<Uint64, Uint32> mObjectLookup{};

    FMeshDrawCommandCache mCommandCache{};

    FBVHTree mBoundsTree{};
    TArray<DirectX::BoundingBox> mBuildBounds{};
    TArray<Uint32> mBoundsObjects{};
    TArray<Uint32> mUnboundedObjects{};

    TArray<Uint32> mChangedObjects{};
    std::deque<FObjectChange> mChanges{};
};
