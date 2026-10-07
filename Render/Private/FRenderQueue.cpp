#include "pch.h"
#include "Render/FRenderQueue.h"
#include "Render/FLODSelection.h"

#include <algorithm>
#include <cstring>

namespace {
    bool IsSameMatrix(const FMatrix& Left, const FMatrix& Right) {
        return std::memcmp(Left.M, Right.M, sizeof(Left.M)) == 0;
    }

    bool IsSameFrustum(const FFrustum& Left, const FFrustum& Right) {
        return Left.Origin.x == Right.Origin.x && Left.Origin.y == Right.Origin.y && Left.Origin.z == Right.Origin.z && Left.Orientation.x == Right.Orientation.x && Left.Orientation.y == Right.Orientation.y && Left.Orientation.z == Right.Orientation.z && Left.Orientation.w == Right.Orientation.w && Left.RightSlope == Right.RightSlope && Left.LeftSlope == Right.LeftSlope && Left.TopSlope == Right.TopSlope && Left.BottomSlope == Right.BottomSlope && Left.Near == Right.Near && Left.Far == Right.Far;
    }
}

void FRenderQueue::Build(const FRenderScene& Scene, const FRenderView& View) {
    if (IsSceneCacheCurrent(Scene, View)) {
        return;
    }

    mSceneItems.clear();
    mOpaqueItems.clear();
    mTranslucentItems.clear();

    if (View.IsPassEnabled(ERenderPass::Opaque) || View.IsPassEnabled(ERenderPass::Translucent)) {
        BuildSceneItems(Scene, View);
    } else {
        mDrawRecords.clear();
    }

    CommitSceneCache(Scene, View);
}

const TArray<FMeshDrawBatch>& FRenderQueue::GetItems(ERenderPass Pass) const {
    switch (Pass) {
        case ERenderPass::Opaque:
            return mOpaqueItems;

        case ERenderPass::Translucent:
            return mTranslucentItems;

        default:
            return mEmptyItems;
    }
}

const TArray<FMeshDrawRecord>& FRenderQueue::GetDrawRecords() const {
    return mDrawRecords;
}

bool FRenderQueue::IsSceneCacheCurrent(const FRenderScene& Scene, const FRenderView& View) const {
    const float ViewportHeight{View.mTarget != nullptr ? View.mTarget->GetViewport().Height : 0.0f};

    if (mSceneCacheKey.mViewportHeight != ViewportHeight) {
        return false;
    }

    return mSceneCacheKey.mScene == &Scene && mSceneCacheKey.mSceneId == Scene.GetId() && mSceneCacheKey.mObjectRevision == Scene.GetRevision() && mSceneCacheKey.mTemplateRevision == Scene.GetTemplateRevision() && IsSameMatrix(mSceneCacheKey.mCamera.mView, View.mCamera.mView) && IsSameMatrix(mSceneCacheKey.mCamera.mProjection, View.mCamera.mProjection) && IsSameMatrix(mSceneCacheKey.mCamera.mViewProjection, View.mCamera.mViewProjection) && IsSameFrustum(mSceneCacheKey.mCamera.mViewFrustum, View.mCamera.mViewFrustum) && mSceneCacheKey.mUseLOD == View.mUseLOD && mSceneCacheKey.mRenderSky == View.mSettings.mBRenderSky && mSceneCacheKey.mOpaque == View.IsPassEnabled(ERenderPass::Opaque) && mSceneCacheKey.mTranslucent == View.IsPassEnabled(ERenderPass::Translucent);
}

void FRenderQueue::CommitSceneCache(const FRenderScene& Scene, const FRenderView& View) {
    const float ViewportHeight{View.mTarget != nullptr ? View.mTarget->GetViewport().Height : 0.0f};

    mSceneCacheKey = FSceneCacheKey{&Scene, Scene.GetId(), Scene.GetRevision(), Scene.GetTemplateRevision(), View.mCamera, ViewportHeight, View.mUseLOD, View.mSettings.mBRenderSky, View.IsPassEnabled(ERenderPass::Opaque), View.IsPassEnabled(ERenderPass::Translucent)};
}

void FRenderQueue::BuildSceneItems(const FRenderScene& Scene, const FRenderView& View) {
    const TArray<FRenderSceneObject>& Objects{Scene.GetObjects()};
    const TArray<FRenderTemplateGroup>& Groups{Scene.GetTemplateGroups()};
    const TArray<FRenderBatchTemplate>& Templates{Scene.GetTemplates()};

    if (Templates.empty()) {
        mDrawRecords.clear();
        return;
    }

    const bool Perspective{std::abs(View.mCamera.mProjection.M[2][3]) > 1e-6f};
    const float ProjectionScale{std::abs(View.mCamera.mProjection.M[1][1])};
    const float ViewportHeight{View.mTarget != nullptr ? View.mTarget->GetViewport().Height : 0.0f};

    Scene.CollectVisibleObjects(View.mCamera, mVisibleObjectIndices, mBoundaryObjectPositions);

    mVisibleObjects.clear();
    mVisibleObjects.reserve(mVisibleObjectIndices.size());
    mBucketCounts.assign(Templates.size() * 2, 0);
    mBucketWritePositions.resize(mBucketCounts.size());

    constexpr Uint32 SelectedFlag{static_cast<Uint32>(ERenderObjectFlags::Selected)};

    for (const Uint32 ObjectIndex : mVisibleObjectIndices) {
        const FRenderSceneObject& Object{Objects[ObjectIndex]};
        const FRenderTemplateGroup& Group{Groups[Object.mTemplateGroupIndex]};

        if (!View.mSettings.mBRenderSky && Group.mSky) {
            continue;
        }

        const Uint32 Flags{Object.mFlags};
        const Uint32 BucketFlag{(Flags & SelectedFlag) != 0 ? 1u : 0u};
        FLODSelection LOD{};

        if (!Group.mSky && View.mUseLOD) {
            LOD = SelectMeshLOD(CalculateScreenSize(Object, View.mCamera, ProjectionScale, Perspective), ViewportHeight, Group.mAvailableLODMask, Object.mCullable && BucketFlag == 0);
        }

        if (LOD.mCulled) {
            continue;
        }

        auto AddLevel{[&](Uint32 Level, float Dither) {
            const FRenderTemplateRange& Range{Group.mTemplateRangesByLOD[Level]};

            if (Range.mTemplateCount == 0) {
                return;
            }

            mVisibleObjects.push_back(FVisibleObject{ObjectIndex, Level, Flags, Dither});

            for (Uint32 Index{}; Index < Range.mTemplateCount; ++Index) {
                ++mBucketCounts[(Range.mFirstTemplateIndex + Index) * 2 + BucketFlag];
            }
        }};

        AddLevel(LOD.mLevel, LOD.mDither);

        if (LOD.mNextLevel != UINT32_MAX) {
            AddLevel(LOD.mNextLevel, -LOD.mDither);
        }
    }

    std::size_t TotalRecords{};

    for (std::size_t Bucket{}; Bucket < mBucketCounts.size(); ++Bucket) {
        const Uint32 Count{mBucketCounts[Bucket]};

        if (Count == 0) {
            continue;
        }

        if (TotalRecords > UINT32_MAX - static_cast<std::size_t>(Count)) {
            mSceneItems.clear();
            mDrawRecords.clear();
            return;
        }

        const FRenderBatchTemplate& Template{Templates[Bucket / 2]};
        const Uint32 FirstRecord{static_cast<Uint32>(TotalRecords)};
        const Uint32 Flags{(Bucket & 1u) != 0 ? SelectedFlag : 0};

        mSceneItems.push_back(FMeshDrawBatch{Template.mState, FirstRecord, Count, Flags});
        mBucketWritePositions[Bucket] = FirstRecord;
        TotalRecords += Count;
    }

    mDrawRecords.resize(TotalRecords);

    for (const FVisibleObject& VisibleObject : mVisibleObjects) {
        const FRenderTemplateGroup& Group{Groups[Objects[VisibleObject.mObjectIndex].mTemplateGroupIndex]};
        const FRenderTemplateRange& TemplateRange{Group.mTemplateRangesByLOD[VisibleObject.mLODLevel]};
        const Uint32 BucketFlag{(VisibleObject.mFlags & SelectedFlag) != 0 ? 1u : 0u};

        for (Uint32 Index{}; Index < TemplateRange.mTemplateCount; ++Index) {
            const Uint32 TemplateIndex{TemplateRange.mFirstTemplateIndex + Index};
            const Uint32 Destination{mBucketWritePositions[TemplateIndex * 2 + BucketFlag]++};

            mDrawRecords[Destination] = FMeshDrawRecord{VisibleObject.mObjectIndex, Templates[TemplateIndex].mMaterialIndex, VisibleObject.mFlags, VisibleObject.mLODDither};
        }
    }

    for (const FMeshDrawBatch& Item : mSceneItems) {
        if (Item.mState.mBlendMode != EMaterialBlendMode::Translucent) {
            if (View.IsPassEnabled(ERenderPass::Opaque)) {
                mOpaqueItems.push_back(Item);
            }

            continue;
        }

        if (!View.IsPassEnabled(ERenderPass::Translucent)) {
            continue;
        }

        for (Uint32 Index{}; Index < Item.mRecordCount; ++Index) {
            FMeshDrawBatch SortedItem{Item};

            SortedItem.mFirstRecord += Index;
            SortedItem.mRecordCount = 1;

            const Uint32 ObjectIndex{mDrawRecords[SortedItem.mFirstRecord].mObjectIndex};
            const DirectX::BoundingSphere& Bounds{Objects[ObjectIndex].mWorldSphereBounds};
            const FMatrix& Transform{Scene.GetObjectTransforms()[ObjectIndex]};
            const FVector3 Center{Bounds.Radius > 0.0f ? FVector3{Bounds.Center.x, Bounds.Center.y, Bounds.Center.z} : FVector3{Transform.M[3][0], Transform.M[3][1], Transform.M[3][2]}};
            const FMatrix& CameraView{View.mCamera.mView};
            const float Depth{Center.mX * CameraView.M[0][2] + Center.mY * CameraView.M[1][2] + Center.mZ * CameraView.M[2][2] + CameraView.M[3][2]};

            SortedItem.mSortDepth = std::isfinite(Depth) ? Depth : 0.0f;
            mTranslucentItems.push_back(SortedItem);
        }
    }

    std::stable_sort(mTranslucentItems.begin(), mTranslucentItems.end(), [](const FMeshDrawBatch& Left, const FMeshDrawBatch& Right) {
        return Left.mSortDepth > Right.mSortDepth;
    });
}

float FRenderQueue::CalculateScreenSize(const FRenderSceneObject& Object, const CameraProbe& Camera, float ProjectionScale, bool Perspective) const {
    const DirectX::BoundingSphere& Bounds{Object.mWorldSphereBounds};

    if (Bounds.Radius <= 1e-4f || !std::isfinite(Bounds.Radius)) {
        return 0.0f;
    }

    float ScreenSize{Bounds.Radius * ProjectionScale};

    if (Perspective) {
        // LOD 선택에 쓰는 카메라 깊이만 계산한다.
        const FMatrix& View{Camera.mView};
        const float ViewDepth{Bounds.Center.x * View.M[0][2] + Bounds.Center.y * View.M[1][2] + Bounds.Center.z * View.M[2][2] + View.M[3][2]};

        ScreenSize /= (std::max)(std::abs(ViewDepth), 1e-4f);
    }

    return ScreenSize;
}
