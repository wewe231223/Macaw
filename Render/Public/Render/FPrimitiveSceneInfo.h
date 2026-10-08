#pragma once
#include "RenderCore/FPrimitiveSceneProxy.h"
#include "Render/FStaticMeshBatch.h"
#include "Render/FMeshDrawCommand.h"
#include "Core/Base/TCachedValue.h"
#include "Render/FRenderAssetStamp.h"
#include "Asset/FLODSettings.h"

struct FPrimitiveSceneInfo {
    FObjectHandle mComponentHandle{};
    FObjectHandle mOwnerHandle{};
    DirectX::BoundingSphere mWorldSphereBounds{};
    DirectX::BoundingOrientedBox mWorldOBB{};
    DirectX::BoundingBox mWorldAABB{};
    bool mActive{};
    bool mCullable{};
    bool mSky{};
    std::unique_ptr<FPrimitiveSceneProxy> mProxy{};
    TCachedValue<TArray<FStaticMeshBatch>, FRenderAssetStamp> mStaticMeshes{};
    std::array<TArray<Uint32>, GLODCount> mCachedCommandsByLOD{};
    Uint32 mAvailableLODMask{};
};
