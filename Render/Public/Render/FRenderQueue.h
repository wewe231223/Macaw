#pragma once
#include "Render/FRenderView.h"
#include "Render/FRenderScene.h"
#include "Render/FMeshDrawData.h"

class FRenderQueue {
private:
    struct FVisibleObject {
        Uint32 mObjectIndex{};
        Uint32 mLODLevel{};
        float mLODDither{};
    };

    struct FSceneCacheKey {
        const FRenderScene* mScene{nullptr};
        Uint64 mSceneId{};
        Uint64 mObjectRevision{};
        Uint64 mTemplateRevision{};
        CameraProbe mCamera{};
        float mViewportHeight{};
        bool mUseLOD{};
        bool mRenderSky{};
        bool mOpaque{};
        bool mTranslucent{};
    };

public:
    void Build(const FRenderScene& Scene, const FRenderView& View);

    const TArray<FMeshDrawBatch>& GetItems(ERenderPass Pass) const;
    const TArray<FMeshDrawRecord>& GetDrawRecords() const;

private:
    bool IsSceneCacheCurrent(const FRenderScene& Scene, const FRenderView& View) const;
    void CommitSceneCache(const FRenderScene& Scene, const FRenderView& View);

    void BuildSceneItems(const FRenderScene& Scene, const FRenderView& View);

    float CalculateScreenSize(const FRenderSceneObject& Object, const CameraProbe& Camera, float ProjectionScale, bool Perspective) const;

private:
    FSceneCacheKey mSceneCacheKey{};

    TArray<FMeshDrawBatch> mSceneItems{};
    TArray<FMeshDrawBatch> mOpaqueItems{};
    TArray<FMeshDrawBatch> mTranslucentItems{};
    TArray<FMeshDrawBatch> mEmptyItems{};

    TArray<FMeshDrawRecord> mDrawRecords{};

    TArray<Uint32> mVisibleObjectIndices{};
    TArray<Uint32> mBoundaryObjectPositions{};
    TArray<FVisibleObject> mVisibleObjects{};

    TArray<Uint32> mBucketCounts{};
    TArray<Uint32> mBucketWritePositions{};
};
