#include "pch.h"
#include "Render/FRenderScene.h"
#include "RenderCore/FStaticMeshSceneProxy.h"
#include "Render/FStaticMeshBatchCollector.h"
#include "Render/FMeshPassProcessor.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Asset/UMesh.h"

#include <algorithm>
#include <cstring>

namespace {
    constexpr std::size_t MaximumJournalChanges{262144};

    Uint64 MakeObjectKey(FObjectHandle Handle) {
        return (static_cast<Uint64>(Handle.mGeneration) << 32) | Handle.mIndex;
    }

    bool HasUsableBounds(const DirectX::BoundingBox& Bounds) {
        return std::isfinite(Bounds.Center.x) && std::isfinite(Bounds.Center.y) && std::isfinite(Bounds.Center.z) && std::isfinite(Bounds.Extents.x) && std::isfinite(Bounds.Extents.y) && std::isfinite(Bounds.Extents.z) && Bounds.Extents.x >= 0.0f && Bounds.Extents.y >= 0.0f && Bounds.Extents.z >= 0.0f && (Bounds.Extents.x > 0.0f || Bounds.Extents.y > 0.0f || Bounds.Extents.z > 0.0f);
    }
}

FRenderScene::FRenderScene(Uint64 SceneId)
	: mSceneId(SceneId) {
}

void FRenderScene::Synchronize(const IAssetRegistry* Registry, FSceneRenderData& Scene, const FMaterialBuffer& Materials, FRenderAssetResources* Resources) {
    FSceneUpdateBatch Updates{};

    std::swap(Updates.mRenderData, Scene);
    Synchronize(Registry, Updates, Materials, Resources);
    std::swap(Updates.mRenderData, Scene);
}

void FRenderScene::Synchronize(const IAssetRegistry* Registry, FSceneUpdateBatch& Updates, const FMaterialBuffer& Materials, FRenderAssetResources* Resources) {
    FSceneRenderData& Scene{Updates.mRenderData};

    mLightProbes.swap(Scene.mLightProbes);
    mTextProbes.swap(Scene.mTextProbes);
    mBillboardProbes.swap(Scene.mBillboardProbes);

    Scene.mLightProbes.clear();
    Scene.mTextProbes.clear();
    Scene.mBillboardProbes.clear();

    mChangedObjects.clear();
    mObjectsChanged = false;

    if (!mSourceRevision.IsCurrent(Scene.mRevision)) {
        ApplyObjectUpdates(Scene);
        mSourceRevision.Commit(Scene.mRevision);
    }

    for (FPrimitiveSceneUpdate& Update : Updates.mPrimitiveUpdates) {
        ApplyPrimitiveUpdate(Update);
    }

    Updates.mPrimitiveUpdates.clear();
    CommitObjectChanges();

    UpdateBounds();

    RefreshStaticMeshes(Registry, Materials, Resources);
}

Uint64 FRenderScene::GetId() const {
    return mSceneId;
}

Uint64 FRenderScene::GetRevision() const {
    return mRevision;
}

const TArray<FMatrix>& FRenderScene::GetObjectTransforms() const {
    return mObjectTransforms;
}

ERenderUpdateMode FRenderScene::CollectChangedObjects(Uint64 SinceRevision, TArray<Uint32>& OutIndices) const {
    OutIndices.clear();

    if (SinceRevision == mRevision) {
        return ERenderUpdateMode::Partial;
    }

    if (SinceRevision == 0 || SinceRevision > mRevision || SinceRevision < mJournalFloor) {
        return ERenderUpdateMode::Full;
    }

    for (auto Position{mChanges.rbegin()}; Position != mChanges.rend() && Position->mRevision > SinceRevision; ++Position) {
        OutIndices.push_back(Position->mObjectIndex);
    }

    std::sort(OutIndices.begin(), OutIndices.end());
    OutIndices.erase(std::unique(OutIndices.begin(), OutIndices.end()), OutIndices.end());

    return ERenderUpdateMode::Partial;
}

const TArray<FPrimitiveSceneInfo>& FRenderScene::GetPrimitives() const {
    return mObjects;
}

const FPrimitiveSceneProxy* FRenderScene::FindPrimitive(FObjectHandle ComponentHandle) const {
    const auto Position{mObjectLookup.find(MakeObjectKey(ComponentHandle))};

    return Position != mObjectLookup.end() ? mObjects[Position->second].mProxy.get() : nullptr;
}

const TArray<std::shared_ptr<const FMeshDrawCommand>>& FRenderScene::GetCachedMeshDrawCommands() const {
    return mCommandCache.GetCommands();
}

const TArray<FLightProbe>& FRenderScene::GetLightProbes() const {
    return mLightProbes;
}

const TArray<FTextProbe>& FRenderScene::GetTextProbes() const {
    return mTextProbes;
}

const TArray<FBillboardProbe>& FRenderScene::GetBillboardProbes() const {
    return mBillboardProbes;
}

void FRenderScene::QueryFrustum(const FMatrix& ViewProjection, TArray<Uint32>& OutIndices, TArray<Uint32>* OutBoundaryPositions) const {
    OutIndices.reserve(mBoundsObjects.size() + mUnboundedObjects.size());
    mBoundsTree.FrustumCull(ViewProjection, OutIndices, OutBoundaryPositions);

    OutIndices.insert(OutIndices.end(), mUnboundedObjects.begin(), mUnboundedObjects.end());
}

void FRenderScene::ApplyObjectUpdates(const FSceneRenderData& Scene) {
    for (const FRenderObjectUpdate& ObjectUpdate : Scene.mObjectUpdates) {
        FPrimitiveSceneUpdate Update{};

        Update.mComponentHandle = ObjectUpdate.mComponentHandle;

        if (!ObjectUpdate.mRemoved) {
            const FActorProbe& Probe{ObjectUpdate.mProbe};

            Update.mType = EPrimitiveSceneUpdate::Create;
            Update.mProxy = std::make_unique<FStaticMeshSceneProxy>(ObjectUpdate.mComponentHandle, Probe.mOwnerHandle, FPrimitiveTransform{Probe.mWorld, Probe.mWorldSphereBounds, Probe.mWorldOBB, Probe.mWorldAABB}, FMeshSceneData{Probe.mMeshHandle, Probe.mMaterialHandle, Probe.mPipelineHandle});
        }

        ApplyPrimitiveUpdate(Update);
    }
}

void FRenderScene::ApplyPrimitiveUpdate(FPrimitiveSceneUpdate& Update) {
    if (!Update.mComponentHandle.IsValid()) {
        return;
    }

    const Uint64 Key{MakeObjectKey(Update.mComponentHandle)};
    const auto Position{mObjectLookup.find(Key)};

    if (Update.mType == EPrimitiveSceneUpdate::Remove) {
        if (Position != mObjectLookup.end()) {
            RemoveObject(Position->second);
            mObjectLookup.erase(Position);
        }
    } else if (Update.mType == EPrimitiveSceneUpdate::Transform) {
        if (Position != mObjectLookup.end()) {
            mObjects[Position->second].mProxy->SetTransform(Update.mTransform);
            UpdateObjectTransform(Position->second, Update.mTransform);
        }
    } else if (Update.mProxy != nullptr && Update.mProxy->GetComponentHandle() == Update.mComponentHandle) {
        Uint32 ObjectIndex{UINT32_MAX};

        if (Position != mObjectLookup.end()) {
            ObjectIndex = Position->second;
            UpdateObject(ObjectIndex, *Update.mProxy);
        } else {
            ObjectIndex = AddObject(*Update.mProxy);
        }

        if (ObjectIndex != UINT32_MAX) {
            mObjectLookup.emplace(Key, ObjectIndex);
            mObjects[ObjectIndex].mProxy = std::move(Update.mProxy);
        }
    }
}

Uint32 FRenderScene::AddObject(const FPrimitiveSceneProxy& Proxy) {
    Uint32 ObjectIndex{};

    if (mFreeObjects.empty()) {
        if (mObjects.size() >= 0x80000000ull) {
            return UINT32_MAX;
        }

        ObjectIndex = static_cast<Uint32>(mObjects.size());
        mObjects.emplace_back();
        mObjectTransforms.emplace_back();
    } else {
        ObjectIndex = mFreeObjects.back();
        mFreeObjects.pop_back();
    }

    FPrimitiveSceneInfo& Object{mObjects[ObjectIndex]};

    Object = {};
    Object.mComponentHandle = Proxy.GetComponentHandle();
    Object.mActive = true;
    mTopologyDirty = true;

    UpdateObject(ObjectIndex, Proxy);

    return ObjectIndex;
}

void FRenderScene::UpdateObject(Uint32 ObjectIndex, const FPrimitiveSceneProxy& Proxy) {
    FPrimitiveSceneInfo& Object{mObjects[ObjectIndex]};
    Object.mStaticMeshes.Invalidate();
    RecordObjectChange(ObjectIndex);

    if (Object.mOwnerHandle != Proxy.GetOwnerHandle()) {
        Object.mOwnerHandle = Proxy.GetOwnerHandle();
        RecordObjectChange(ObjectIndex);
    }

    UpdateObjectTransform(ObjectIndex, Proxy.GetTransform());
}

void FRenderScene::UpdateObjectTransform(Uint32 ObjectIndex, const FPrimitiveTransform& Transform) {
    FPrimitiveSceneInfo& Object{mObjects[ObjectIndex]};
    const bool Cullable{HasUsableBounds(Transform.mWorldAABB)};
    const bool TransformChanged{std::memcmp(&mObjectTransforms[ObjectIndex], &Transform.mWorld, sizeof(FMatrix)) != 0};
    const bool BoundsChanged{std::memcmp(&Object.mWorldSphereBounds, &Transform.mWorldSphereBounds, sizeof(DirectX::BoundingSphere)) != 0 || std::memcmp(&Object.mWorldOBB, &Transform.mWorldOBB, sizeof(DirectX::BoundingOrientedBox)) != 0 || std::memcmp(&Object.mWorldAABB, &Transform.mWorldAABB, sizeof(DirectX::BoundingBox)) != 0};

    if (!TransformChanged && !BoundsChanged && Object.mCullable == Cullable) {
        return;
    }

    if (Object.mCullable != Cullable) {
        mTopologyDirty = true;
    }

    Object.mWorldSphereBounds = Transform.mWorldSphereBounds;
    Object.mWorldOBB = Transform.mWorldOBB;
    Object.mWorldAABB = Transform.mWorldAABB;
    Object.mCullable = Cullable;
    mObjectTransforms[ObjectIndex] = Transform.mWorld;

    RecordObjectChange(ObjectIndex);
}

void FRenderScene::RemoveObject(Uint32 ObjectIndex) {
    FPrimitiveSceneInfo& Object{mObjects[ObjectIndex]};

    if (!Object.mActive) {
        return;
    }

    Object.mActive = false;
    mObjects[ObjectIndex].mProxy.reset();

    Object.mStaticMeshes = {};
    Object.mAvailableLODMask = 0;

    for (TArray<Uint32>& Commands : Object.mCachedCommandsByLOD) {
        Commands.clear();
    }

    mFreeObjects.push_back(ObjectIndex);
    mTopologyDirty = true;

    RecordObjectChange(ObjectIndex);
}

void FRenderScene::RecordObjectChange(Uint32 ObjectIndex) {
    mObjectsChanged = true;
    mChangedObjects.push_back(ObjectIndex);
}

void FRenderScene::CommitObjectChanges() {
    if (mObjectsChanged) {
        ++mRevision;
        std::sort(mChangedObjects.begin(), mChangedObjects.end());
        mChangedObjects.erase(std::unique(mChangedObjects.begin(), mChangedObjects.end()), mChangedObjects.end());

        for (const Uint32 ObjectIndex : mChangedObjects) {
            mChanges.push_back(FObjectChange{mRevision, ObjectIndex});
        }

        while (mChanges.size() > MaximumJournalChanges) {
            mJournalFloor = (std::max)(mJournalFloor, mChanges.front().mRevision);
            mChanges.pop_front();
        }
    }
}

void FRenderScene::RefreshStaticMeshes(const IAssetRegistry* Registry, const FMaterialBuffer& Materials, FRenderAssetResources* Resources) {
    const FAssetHandle SkyPipeline{Registry != nullptr ? Registry->FindAsset(FAssetPath{"/Game/Pipeline/SkyDome.json"}) : FAssetHandle{}};

    mCommandCache.BeginUpdate();

    for (Uint32 ObjectIndex{}; ObjectIndex < mObjects.size(); ++ObjectIndex) {
        FPrimitiveSceneInfo& Primitive{mObjects[ObjectIndex]};

        if (!Primitive.mActive) {
            continue;
        }

        const FMeshSceneData& MeshData{Primitive.mProxy->GetMeshData()};
        const UMesh* Mesh{Registry != nullptr ? Registry->ResolveAsset<UMesh>(MeshData.mMeshHandle) : nullptr};
        const Uint64 MeshRevision{Mesh != nullptr ? Mesh->GetRenderRevision() : 0};
        Primitive.mSky = static_cast<bool>(SkyPipeline) && MeshData.mPipelineHandle == SkyPipeline;
        Primitive.mAvailableLODMask = 0;

        for (TArray<Uint32>& Commands : Primitive.mCachedCommandsByLOD) {
            Commands.clear();
        }

        if (Registry == nullptr) {
            Primitive.mStaticMeshes.Invalidate();
            continue;
        }

        const FRenderAssetStamp Stamp{Mesh != nullptr ? Mesh->GetHandle() : FObjectHandle{}, MeshRevision};
        const TArray<FStaticMeshBatch>* StaticMeshes{Primitive.mStaticMeshes.GetOrUpdate(Stamp, [&](TArray<FStaticMeshBatch>& Meshes) {
            Meshes.clear();

            FStaticMeshBatchCollector Collector{*Registry, ObjectIndex, Meshes};

            Primitive.mProxy->DrawStaticElements(Collector);

            return true;
        })};

        if (StaticMeshes == nullptr) {
            continue;
        }

        FMeshPassProcessor Opaque{ERenderPass::Opaque, *Registry, Materials, Resources, mCommandCache};
        FMeshPassProcessor Translucent{ERenderPass::Translucent, *Registry, Materials, Resources, mCommandCache};

        for (const FStaticMeshBatch& StaticMesh : *StaticMeshes) {
            if (StaticMesh.mLODLevel >= GLODCount) {
                continue;
            }

            TArray<Uint32>& Commands{Primitive.mCachedCommandsByLOD[StaticMesh.mLODLevel]};

            Opaque.AddMeshBatch(StaticMesh, Commands);
            Translucent.AddMeshBatch(StaticMesh, Commands);

            if (!Commands.empty()) {
                Primitive.mAvailableLODMask |= 1u << StaticMesh.mLODLevel;
            }
        }
    }

    mCommandCache.EndUpdate();
}

void FRenderScene::UpdateBounds() {
    if (mTopologyDirty) {
        RebuildBounds();
    } else {
        for (const Uint32 ObjectIndex : mChangedObjects) {
            mBoundsTree.UpdateBounds(ObjectIndex, mObjects[ObjectIndex].mWorldAABB);
        }

        mBoundsTree.Refit(mChangedObjects);
    }
}

void FRenderScene::RebuildBounds() {
    mBuildBounds.resize(mObjects.size());
    mBoundsObjects.clear();
    mUnboundedObjects.clear();

    for (Uint32 Index{}; Index < mObjects.size(); ++Index) {
        if (!mObjects[Index].mActive) {
            continue;
        }

        if (mObjects[Index].mCullable) {
            mBuildBounds[Index] = mObjects[Index].mWorldAABB;
            mBoundsObjects.push_back(Index);
        } else {
            mUnboundedObjects.push_back(Index);
        }
    }

    mBoundsTree.Build(mBuildBounds, mBoundsObjects);
    mTopologyDirty = false;
}
