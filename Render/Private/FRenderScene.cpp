#include "pch.h"
#include "Render/FRenderScene.h"
#include "RenderCore/FStaticMeshSceneProxy.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Asset/UMaterial.h"
#include "Asset/UMesh.h"
#include "Asset/Pipeline/UPipeline.h"

#include <algorithm>
#include <cstring>
#include <tuple>

namespace {
    constexpr std::size_t MaximumJournalChanges{262144};

    Uint64 MakeObjectKey(FObjectHandle Handle) {
        return (static_cast<Uint64>(Handle.mGeneration) << 32) | Handle.mIndex;
    }

    bool HasUsableBounds(const DirectX::BoundingBox& Bounds) {
        return std::isfinite(Bounds.Center.x) && std::isfinite(Bounds.Center.y) && std::isfinite(Bounds.Center.z) && std::isfinite(Bounds.Extents.x) && std::isfinite(Bounds.Extents.y) && std::isfinite(Bounds.Extents.z) && Bounds.Extents.x >= 0.0f && Bounds.Extents.y >= 0.0f && Bounds.Extents.z >= 0.0f && (Bounds.Extents.x > 0.0f || Bounds.Extents.y > 0.0f || Bounds.Extents.z > 0.0f);
    }
}

bool operator<(const FRenderTemplateGroupKey& Left, const FRenderTemplateGroupKey& Right) {
    return std::tie(Left.mPipelineHandle.mId, Left.mPipelineHandle.mGeneration, Left.mMaterialHandle.mId, Left.mMaterialHandle.mGeneration, Left.mMeshHandle.mId, Left.mMeshHandle.mGeneration) < std::tie(Right.mPipelineHandle.mId, Right.mPipelineHandle.mGeneration, Right.mMaterialHandle.mId, Right.mMaterialHandle.mGeneration, Right.mMeshHandle.mId, Right.mMeshHandle.mGeneration);
}

FRenderScene::FRenderScene(Uint64 SceneId)
	: mSceneId{SceneId} {
}

void FRenderScene::Synchronize(const IAssetRegistry* Registry, FSceneRenderData& Scene, const FMaterialBuffer& Materials) {
    FSceneUpdateBatch Updates{};

    std::swap(Updates.mRenderData, Scene);
    Synchronize(Registry, Updates, Materials);
    std::swap(Updates.mRenderData, Scene);
}

void FRenderScene::Synchronize(const IAssetRegistry* Registry, FSceneUpdateBatch& Updates, const FMaterialBuffer& Materials) {
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

    RefreshTemplates(Registry, Materials);
}

Uint64 FRenderScene::GetId() const {
    return mSceneId;
}

Uint64 FRenderScene::GetRevision() const {
    return mRevision;
}

Uint64 FRenderScene::GetTemplateRevision() const {
    return mTemplateRevision;
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

const TArray<FRenderSceneObject>& FRenderScene::GetObjects() const {
    return mObjects;
}

const FPrimitiveSceneProxy* FRenderScene::FindPrimitive(FObjectHandle ComponentHandle) const {
    const auto Position{mObjectLookup.find(MakeObjectKey(ComponentHandle))};

    return Position != mObjectLookup.end() ? mSceneProxies[Position->second].get() : nullptr;
}

const TArray<FRenderBatchTemplate>& FRenderScene::GetTemplates() const {
    return mTemplates;
}

const TArray<FRenderTemplateGroup>& FRenderScene::GetTemplateGroups() const {
    return mTemplateGroups;
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

void FRenderScene::CollectVisibleObjects(const FFrustum& Frustum, TArray<Uint32>& OutIndices) const {
    OutIndices.reserve(mBoundsObjects.size() + mUnboundedObjects.size());
    mBoundsTree.FrustumCull(Frustum, OutIndices);

    std::erase_if(OutIndices, [this, &Frustum](Uint32 ObjectIndex) {
        return !Frustum.Intersects(mObjects[ObjectIndex].mWorldOBB);
    });

    OutIndices.insert(OutIndices.end(), mUnboundedObjects.begin(), mUnboundedObjects.end());
}

void FRenderScene::CollectVisibleObjects(const FMatrix& ViewProjection, TArray<Uint32>& OutIndices) const {
    OutIndices.reserve(mBoundsObjects.size() + mUnboundedObjects.size());
    mBoundsTree.FrustumCull(ViewProjection, OutIndices);

    OutIndices.insert(OutIndices.end(), mUnboundedObjects.begin(), mUnboundedObjects.end());
}

void FRenderScene::CollectVisibleObjects(const CameraProbe& Camera, TArray<Uint32>& OutIndices, TArray<Uint32>& OutBoundaryPositions) const {
    const bool Perspective{std::abs(Camera.mProjection.M[2][3]) > 1e-6f};

    OutIndices.reserve(mBoundsObjects.size() + mUnboundedObjects.size());
    OutBoundaryPositions.clear();
    mBoundsTree.FrustumCull(Camera.mViewProjection, OutIndices, Perspective ? &OutBoundaryPositions : nullptr);

    for (const Uint32 Position : OutBoundaryPositions) {
        if (!Camera.mViewFrustum.Intersects(mObjects[OutIndices[Position]].mWorldOBB)) {
            OutIndices[Position] = UINT32_MAX;
        }
    }

    if (!OutBoundaryPositions.empty()) {
        std::erase(OutIndices, UINT32_MAX);
    }

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
            mSceneProxies[Position->second]->SetTransform(Update.mTransform);
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
            mSceneProxies[ObjectIndex] = std::move(Update.mProxy);
        }
    }
}

Uint32 FRenderScene::FindOrAddTemplateGroup(const FMeshSceneData& Mesh) {
    const FRenderTemplateGroupKey Key{Mesh.mPipelineHandle, Mesh.mMaterialHandle, Mesh.mMeshHandle};
    const auto Position{mTemplateGroupIndicesByKey.find(Key)};

    if (Position != mTemplateGroupIndicesByKey.end()) {
        return Position->second;
    }

    const Uint32 GroupIndex{static_cast<Uint32>(mTemplateGroups.size())};
    FRenderTemplateGroup Group{};

    Group.mKey = Key;

    mTemplateGroups.push_back(Group);
    mTemplateGroupIndicesByKey.emplace(Key, GroupIndex);
    mTemplatesDirty = true;

    return GroupIndex;
}

Uint32 FRenderScene::AddObject(const FPrimitiveSceneProxy& Proxy) {
    Uint32 ObjectIndex{};

    if (mFreeObjects.empty()) {
        if (mObjects.size() >= 0x80000000ull) {
            return UINT32_MAX;
        }

        ObjectIndex = static_cast<Uint32>(mObjects.size());
        mObjects.emplace_back();
        mSceneProxies.emplace_back();
        mObjectTransforms.emplace_back();
    } else {
        ObjectIndex = mFreeObjects.back();
        mFreeObjects.pop_back();
    }

    FRenderSceneObject& Object{mObjects[ObjectIndex]};

    Object = {};
    Object.mComponentHandle = Proxy.GetComponentHandle();
    Object.mActive = true;
    mTopologyDirty = true;

    UpdateObject(ObjectIndex, Proxy);

    return ObjectIndex;
}

void FRenderScene::UpdateObject(Uint32 ObjectIndex, const FPrimitiveSceneProxy& Proxy) {
    FRenderSceneObject& Object{mObjects[ObjectIndex]};
    const FMeshSceneData& Mesh{Proxy.GetMeshData()};
    Uint32 GroupIndex{Object.mTemplateGroupIndex};

    if (GroupIndex == UINT32_MAX || mTemplateGroups[GroupIndex].mKey.mPipelineHandle != Mesh.mPipelineHandle || mTemplateGroups[GroupIndex].mKey.mMaterialHandle != Mesh.mMaterialHandle || mTemplateGroups[GroupIndex].mKey.mMeshHandle != Mesh.mMeshHandle) {
        GroupIndex = FindOrAddTemplateGroup(Mesh);
    }

    if (Object.mTemplateGroupIndex != GroupIndex) {
        if (Object.mTemplateGroupIndex != UINT32_MAX) {
            FRenderTemplateGroup& PreviousGroup{mTemplateGroups[Object.mTemplateGroupIndex]};

            --PreviousGroup.mReferenceCount;
            mTemplatesDirty = mTemplatesDirty || PreviousGroup.mReferenceCount == 0;
        }

        mTemplatesDirty = mTemplatesDirty || mTemplateGroups[GroupIndex].mReferenceCount == 0;
        ++mTemplateGroups[GroupIndex].mReferenceCount;
        Object.mTemplateGroupIndex = GroupIndex;
        RecordObjectChange(ObjectIndex);
    }

    if (Object.mOwnerHandle != Proxy.GetOwnerHandle()) {
        Object.mOwnerHandle = Proxy.GetOwnerHandle();
        RecordObjectChange(ObjectIndex);
    }

    UpdateObjectTransform(ObjectIndex, Proxy.GetTransform());
}

void FRenderScene::UpdateObjectTransform(Uint32 ObjectIndex, const FPrimitiveTransform& Transform) {
    FRenderSceneObject& Object{mObjects[ObjectIndex]};
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
    FRenderSceneObject& Object{mObjects[ObjectIndex]};

    if (!Object.mActive) {
        return;
    }

    Object.mActive = false;
    mSceneProxies[ObjectIndex].reset();

    if (Object.mTemplateGroupIndex != UINT32_MAX) {
        FRenderTemplateGroup& Group{mTemplateGroups[Object.mTemplateGroupIndex]};

        --Group.mReferenceCount;
        mTemplatesDirty = mTemplatesDirty || Group.mReferenceCount == 0;
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

void FRenderScene::RefreshTemplates(const IAssetRegistry* Registry, const FMaterialBuffer& Materials) {
    if (mMaterialBufferRevision != Materials.GetRevision()) {
        mMaterialBufferRevision = Materials.GetRevision();
        mTemplatesDirty = true;
    }

    const FAssetHandle SkyPipeline{Registry != nullptr ? Registry->FindAsset(FAssetPath{"/Game/Pipeline/SkyDome.json"}) : FAssetHandle{}};

    for (FRenderTemplateGroup& Group : mTemplateGroups) {
        const bool Sky{Group.mKey.mPipelineHandle == SkyPipeline};

        if (Group.mSky != Sky) {
            Group.mSky = Sky;
            mTemplatesDirty = true;
        }

        if (Group.mReferenceCount == 0) {
            continue;
        }

        const UMesh* Mesh{Registry != nullptr ? Registry->ResolveAsset<UMesh>(Group.mKey.mMeshHandle) : nullptr};
        const UMaterial* Material{Registry != nullptr ? Registry->ResolveAsset<UMaterial>(Group.mKey.mMaterialHandle) : nullptr};
        const UPipeline* Pipeline{Registry != nullptr ? Registry->ResolveAsset<UPipeline>(Group.mKey.mPipelineHandle) : nullptr};
        const Uint64 MeshRevision{Mesh != nullptr ? Mesh->GetRenderRevision() : 0};
        const Uint64 MaterialRevision{Material != nullptr ? Material->GetRenderRevision() : 0};

        const bool MeshChanged{Group.mMesh.Update(Mesh, MeshRevision)};
        const bool MaterialChanged{Group.mMaterial.Update(Material, MaterialRevision)};

        if (MeshChanged || MaterialChanged || Group.mPipeline != Pipeline) {
            Group.mPipeline = Pipeline;
            mTemplatesDirty = true;
        }
    }

    if (!mTemplatesDirty) {
        return;
    }

    mTemplates.clear();

    for (FRenderTemplateGroup& Group : mTemplateGroups) {
        Group.mTemplateRangesByLOD.fill(FRenderTemplateRange{});
        Group.mAvailableLODMask = 0;

        const UMesh* Mesh{Group.mMesh.GetValue()};
        const UMaterial* Material{Group.mMaterial.GetValue()};

        if (Group.mReferenceCount == 0 || Mesh == nullptr || Material == nullptr || Group.mPipeline == nullptr) {
            continue;
        }

        for (Uint32 Level{}; Level < GLODCount; ++Level) {
            if (Level != 0 && !Mesh->HasLOD(static_cast<int>(Level))) {
                continue;
            }

            const Uint32 FirstTemplateIndex{static_cast<Uint32>(mTemplates.size())};

            AppendMeshDrawTemplates(*Mesh, *Material, Materials, Group.mKey.mPipelineHandle, Group.mKey.mMeshHandle, Level, mTemplates);

            FRenderTemplateRange& TemplateRange{Group.mTemplateRangesByLOD[Level]};

            TemplateRange = FRenderTemplateRange{FirstTemplateIndex, static_cast<Uint32>(mTemplates.size()) - FirstTemplateIndex};

            if (TemplateRange.mTemplateCount != 0) {
                Group.mAvailableLODMask |= 1u << Level;
            }
        }
    }

    ++mTemplateRevision;
    mTemplatesDirty = false;
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
