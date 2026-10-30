/**
 * @file FRenderScene.h
 * @brief 전역 에셋을 조회하며 씬별 프리미티브와 조명 및 명령 캐시를 유지하는 렌더 씬을 정의합니다.
 */

#pragma once
#include "RenderCore/FRenderData.h"
#include "RenderCore/FSceneUpdateBatch.h"
#include "Render/FBVHTree.h"
#include "Render/FPrimitiveSceneInfo.h"
#include "Render/FMeshDrawCommandCache.h"

#include <deque>
#include <map>
#include <unordered_map>

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
    /**
     * @brief 이 씬의 변경 사항을 적용하고 전역 에셋으로 정적 메시 명령을 갱신합니다.
     * @param Updates 소비할 씬 변경 목록입니다.
     * @param Materials 장치별 머티리얼 인덱스 저장소입니다.
     * @param Resources 선택적인 GPU 에셋 캐시입니다. nullptr이면 CPU 명령만 구성합니다.
     */
    void Synchronize(FSceneUpdateBatch& Updates, const FMaterialBuffer& Materials, FRenderAssetResources* Resources = nullptr);

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

    /**
     * @brief 전역 에셋의 현재 상태로 이 씬의 정적 메시 캐시를 갱신합니다.
     * @param Materials 머티리얼 인덱스 저장소입니다.
     * @param Resources 선택적인 GPU 에셋 캐시입니다.
     */
    void RefreshStaticMeshes(const FMaterialBuffer& Materials, FRenderAssetResources* Resources);
    void UpdateBounds();

private:
    Uint64 mSceneId{};
    Uint64 mRevision{1};
    Uint64 mJournalFloor{};

    bool mObjectsChanged{};

    TArray<FPrimitiveSceneInfo> mObjects{};
    TArray<FMatrix> mObjectTransforms{};
    TArray<Uint32> mFreeObjects{};

    std::map<Uint64, std::unique_ptr<FLightSceneProxy>> mLights{};
    TArray<FLightShaderParameters> mLightParameters{};

    std::unordered_map<Uint64, Uint32> mObjectLookup{};

    FMeshDrawCommandCache mCommandCache{};

    FBVHTree mBoundsTree{};
    TArray<Uint32> mUnboundedObjects{};

    TArray<Uint32> mChangedObjects{};
    std::deque<FObjectChange> mChanges{};
};
