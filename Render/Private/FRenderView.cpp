#include "pch.h"
#include "Render/FRenderView.h"
#include "Render/FRenderScene.h"
#include "Render/FLODSelection.h"

#include <algorithm>
#include <cmath>

namespace {
    float CalculateScreenSize(const FPrimitiveSceneInfo& Object, const CameraProbe& Camera, float ProjectionScale, bool Perspective) {
        const DirectX::BoundingSphere& Bounds{Object.mWorldSphereBounds};

        if (Bounds.Radius <= 1e-4f || !std::isfinite(Bounds.Radius)) {
            return 0.0f;
        }

        float ScreenSize{Bounds.Radius * ProjectionScale};

        if (Perspective) {
            const FMatrix& View{Camera.mView};
            const float ViewDepth{Bounds.Center.x * View.M[0][2] + Bounds.Center.y * View.M[1][2] + Bounds.Center.z * View.M[2][2] + View.M[3][2]};

            ScreenSize /= (std::max)(std::abs(ViewDepth), 1e-4f);
        }

        return ScreenSize;
    }

    float CalculateSortDepth(const FPrimitiveSceneInfo& Object, const FMatrix& Transform, const FMatrix& View) {
        const DirectX::BoundingSphere& Bounds{Object.mWorldSphereBounds};
        const FVector3 Center{Bounds.Radius > 0.0f ? FVector3{Bounds.Center.x, Bounds.Center.y, Bounds.Center.z} : FVector3{Transform.M[3][0], Transform.M[3][1], Transform.M[3][2]}};
        const float Depth{Center.mX * View.M[0][2] + Center.mY * View.M[1][2] + Center.mZ * View.M[2][2] + View.M[3][2]};

        return std::isfinite(Depth) ? Depth : 0.0f;
    }
}

bool FRenderView::IsPassEnabled(ERenderPass Pass) const {
    const std::size_t Index{static_cast<std::size_t>(Pass)};

    if (Index >= mPasses.size() || !mPasses.test(Index)) {
        return false;
    }

    return true;
}

void FRenderView::SetPassEnabled(ERenderPass Pass, bool Enabled) {
    const std::size_t Index{static_cast<std::size_t>(Pass)};

    if (Index < mPasses.size()) {
        mPasses.set(Index, Enabled);
    }
}

void FRenderView::CollectMeshDrawCommands(const FRenderScene& Scene, TArray<FVisibleMeshDrawCommand>& OutCommands) const {
    OutCommands.clear();

    if (mTarget == nullptr || (!IsPassEnabled(ERenderPass::Opaque) && !IsPassEnabled(ERenderPass::Translucent)) || Scene.GetCachedMeshDrawCommands().empty()) {
        return;
    }

    const TArray<FPrimitiveSceneInfo>& Objects{Scene.GetPrimitives()};
    const bool Perspective{std::abs(mCamera.mProjection.M[2][3]) > 1e-6f};
    const float ProjectionScale{std::abs(mCamera.mProjection.M[1][1])};
    const float ViewportHeight{mTarget->GetViewport().Height};
    TArray<Uint32> ObjectIndices{};
    TArray<Uint32> BoundaryPositions{};

    Scene.QueryFrustum(mCamera.mViewProjection, ObjectIndices, Perspective ? &BoundaryPositions : nullptr);

    for (const Uint32 Position : BoundaryPositions) {
        if (!mCamera.mViewFrustum.Intersects(Objects[ObjectIndices[Position]].mWorldOBB)) {
            ObjectIndices[Position] = UINT32_MAX;
        }
    }

    OutCommands.reserve(ObjectIndices.size());

    for (const Uint32 ObjectIndex : ObjectIndices) {
        if (ObjectIndex == UINT32_MAX) {
            continue;
        }

        const FPrimitiveSceneInfo& Object{Objects[ObjectIndex]};

        if (!mSettings.mBRenderSky && Object.mSky) {
            continue;
        }

        FLODSelection LOD{};

        if (!Object.mSky && mUseLOD) {
            LOD = SelectMeshLOD(CalculateScreenSize(Object, mCamera, ProjectionScale, Perspective), ViewportHeight, Object.mAvailableLODMask, Object.mCullable);
        }

        if (LOD.mCulled) {
            continue;
        }

        const float Depth{CalculateSortDepth(Object, Scene.GetObjectTransforms()[ObjectIndex], mCamera.mView)};
        const auto AddLevel{[&](Uint32 Level, float Dither) {
            for (const Uint32 CommandIndex : Object.mCachedCommandsByLOD[Level]) {
                OutCommands.push_back(FVisibleMeshDrawCommand{ObjectIndex, CommandIndex, Dither, Depth});
            }
        }};

        AddLevel(LOD.mLevel, LOD.mDither);

        if (LOD.mNextLevel != UINT32_MAX) {
            AddLevel(LOD.mNextLevel, -LOD.mDither);
        }
    }
}
