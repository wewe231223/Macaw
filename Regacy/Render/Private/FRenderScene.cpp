#include "pch.h"
#include "Asset/FAssetRegistry.h"
#include "Render/FRenderScene.h"
#include "Render/FStaticMeshBatchCollector.h"
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

void FRenderScene::Synchronize(FSceneUpdateBatch& Updates, const FMaterialBuffer& Materials, FRenderAssetResources* Resources) {
    mChangedObjects.clear();
    mObjectsChanged = false;

    for (FPrimitiveSceneUpdate& Update : Updates.mPrimitiveUpdates) {
        ApplyPrimitiveUpdate(Update);
    }

    for (FLightSceneUpdate& Update : Updates.mLightUpdates) {
        ApplyLightUpdate(Update);
    }

    if (!Updates.mLightUpdates.empty()) {
        mLightParameters.clear();

        for (const auto& Entry : mLights) {
            mLightParameters.push_back(Entry.second->GetShaderParameters());
        }
    }

    Updates.mPrimitiveUpdates.clear();
    Updates.mLightUpdates.clear();
    CommitObjectChanges();

    UpdateBounds();

    RefreshStaticMeshes(Materials, Resources);
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

const TArray<FLightShaderParameters>& FRenderScene::GetLights() const {
    return mLightParameters;
}

void FRenderScene::QueryFrustum(const FMatrix& ViewProjection, TArray<Uint32>& OutIndices, TArray<Uint32>* OutBoundaryPositions) const {
    OutIndices.reserve(mBoundsTree.GetObjectCount() + mUnboundedObjects.size());
    mBoundsTree.FrustumCull(ViewProjection, OutIndices, OutBoundaryPositions);

    OutIndices.insert(OutIndices.end(), mUnboundedObjects.begin(), mUnboundedObjects.end());
}

void FRenderScene::ApplyPrimitiveUpdate(FPrimitiveSceneUpdate& Update) {
    if (!Update.mComponentHandle.IsValid()) {
        return;
    }

    const Uint64 Key{MakeObjectKey(Update.mComponentHandle)};
    const auto Position{mObjectLookup.find(Key)};

    if (Update.mType == ESceneUpdateType::Remove) {
        if (Position != mObjectLookup.end()) {
            RemoveObject(Position->second);
            mObjectLookup.erase(Position);
        }
    } else if (Update.mType == ESceneUpdateType::Transform || Update.mType == ESceneUpdateType::DynamicData) {
        if (Position != mObjectLookup.end()) {
            FPrimitiveSceneProxy& Proxy{*mObjects[Position->second].mProxy};

            if (Update.mType == ESceneUpdateType::Transform) {
                Proxy.SetTransform(Update.mTransform);
                UpdateObjectTransform(Position->second, Update.mTransform);
            }

            if (Update.mBillboardUV.has_value()) {
                Proxy.SetBillboardUV(Update.mBillboardUV->mUvMin, Update.mBillboardUV->mUvMax);
            }

            if (Update.mUVTransformsChanged) {
                Proxy.SetUVTransforms(Update.mUVTransforms);
            }
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

void FRenderScene::RefreshStaticMeshes(const FMaterialBuffer& Materials, FRenderAssetResources* Resources) {
    const IAssetRegistry* Registry{FAssetRegistry::TryGet()};

    mCommandCache.BeginUpdate();

    for (Uint32 ObjectIndex{}; ObjectIndex < mObjects.size(); ++ObjectIndex) {
        FPrimitiveSceneInfo& Primitive{mObjects[ObjectIndex]};

        if (!Primitive.mActive) {
            continue;
        }

        const FMeshSceneData* MeshData{Primitive.mProxy->GetMeshData()};

        if (MeshData == nullptr) {
            Primitive.mSky = false;
            Primitive.mAvailableLODMask = 0;

            for (TArray<Uint32>& Commands : Primitive.mCachedCommandsByLOD) {
                Commands.clear();
            }

            continue;
        }

        const UMesh* Mesh{Registry != nullptr ? Registry->ResolveAsset<UMesh>(MeshData->mMeshHandle) : nullptr};
        const Uint64 MeshRevision{Mesh != nullptr ? Mesh->GetRenderRevision() : 0};

        Primitive.mSky = false;
        Primitive.mAvailableLODMask = 0;

        for (TArray<Uint32>& Commands : Primitive.mCachedCommandsByLOD) {
            Commands.clear();
        }

        if (Registry == nullptr) {
            Primitive.mStaticMeshes.Invalidate();
            continue;
        }

        const FRenderAssetStamp Stamp{Mesh != nullptr ? Mesh->GetHandle() : FObjectHandle{}, MeshRevision};
        const TArray<FMeshBatch>* StaticMeshes{Primitive.mStaticMeshes.GetOrUpdate(Stamp, [&](TArray<FMeshBatch>& Meshes) {
            Meshes.clear();

            FStaticMeshBatchCollector Collector{Meshes};

            Primitive.mProxy->DrawStaticElements(Collector);

            return true;
        })};

        if (StaticMeshes == nullptr) {
            continue;
        }

        for (const FMeshBatch& StaticMesh : *StaticMeshes) {
            if (StaticMesh.mLODLevel >= GLODCount) {
                continue;
            }

            TArray<Uint32>& Commands{Primitive.mCachedCommandsByLOD[StaticMesh.mLODLevel]};

            mCommandCache.AddMeshBatch(StaticMesh, Materials, Resources, Commands);

            if (!Commands.empty()) {
                Primitive.mAvailableLODMask |= 1u << StaticMesh.mLODLevel;
            }
        }

        const TArray<Uint32>& BaseCommands{Primitive.mCachedCommandsByLOD[0]};

        Primitive.mSky = !BaseCommands.empty() && std::ranges::all_of(BaseCommands, [this](Uint32 Index) {
            return mCommandCache.GetCommands()[Index]->mState.mSky;
        });
    }

    mCommandCache.EndUpdate();
}

void FRenderScene::UpdateBounds() {
    for (const Uint32 ObjectIndex : mChangedObjects) {
        const FPrimitiveSceneInfo& Object{mObjects[ObjectIndex]};
        const auto Position{std::lower_bound(mUnboundedObjects.begin(), mUnboundedObjects.end(), ObjectIndex)};

        if (Object.mActive && !Object.mCullable) {
            mBoundsTree.Remove(ObjectIndex);

            if (Position == mUnboundedObjects.end() || *Position != ObjectIndex) {
                mUnboundedObjects.insert(Position, ObjectIndex);
            }
        } else {
            if (Position != mUnboundedObjects.end() && *Position == ObjectIndex) {
                mUnboundedObjects.erase(Position);
            }

            if (Object.mActive) {
                mBoundsTree.Register(ObjectIndex, Object.mWorldAABB);
            } else {
                mBoundsTree.Remove(ObjectIndex);
            }
        }
    }

    mBoundsTree.CommitUpdates();
}

void FRenderScene::CollectDynamicMeshElements(FDynamicPrimitiveDrawInterface& DrawInterface) const {
    for (const FPrimitiveSceneInfo& Primitive : mObjects) {
        if (Primitive.mActive) {
            Primitive.mProxy->GetDynamicMeshElements(DrawInterface);
        }
    }
}

const FLightSceneProxy* FRenderScene::FindLight(FObjectHandle ComponentHandle) const {
    const auto Position{mLights.find(MakeObjectKey(ComponentHandle))};

    return Position != mLights.end() ? Position->second.get() : nullptr;
}

void FRenderScene::ApplyLightUpdate(FLightSceneUpdate& Update) {
    if (!Update.mComponentHandle.IsValid()) {
        return;
    }

    const Uint64 Key{MakeObjectKey(Update.mComponentHandle)};
    const auto Position{mLights.find(Key)};

    if (Update.mType == ESceneUpdateType::Remove) {
        mLights.erase(Key);
    } else if (Update.mType == ESceneUpdateType::Create && Update.mProxy != nullptr && Update.mProxy->GetComponentHandle() == Update.mComponentHandle) {
        mLights[Key] = std::move(Update.mProxy);
    } else if (Update.mType == ESceneUpdateType::Transform && Position != mLights.end()) {
        Position->second->SetTransform(Update.mWorld);
    }
}
