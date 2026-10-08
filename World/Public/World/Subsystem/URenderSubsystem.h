#pragma once
#include "World/Subsystem/UWorldSubsystem.h"
#include "RenderCore/FRenderProbe.h"
#include "RenderCore/FSceneUpdateBatch.h"
#include "World/Component/UStaticMeshComponent.h"

#include <unordered_map>
#include <unordered_set>

class URenderSubsystem : public UWorldSubsystem {
public:
    URenderSubsystem();
    ~URenderSubsystem() override = default;

public:
    JG_DECLARE_DERIVED_TYPEINFO(URenderSubsystem, UWorldSubsystem)

    void RegisterComponent(UStaticMeshComponent* Component);
    void UnregisterComponent(UStaticMeshComponent* Component);
    void MarkComponentDirty(UActorComponent* Component);
    void RecreateRenderStates();

    void AddPrimitive(std::unique_ptr<FPrimitiveSceneProxy> Proxy);
    void UpdatePrimitiveTransform(FObjectHandle ComponentHandle, const FPrimitiveTransform& Transform);
    void RemovePrimitive(FObjectHandle ComponentHandle);

    void BuildSceneUpdates(FSceneUpdateBatch& Updates);
    void BuildRenderProbes(FSceneRenderData& Scene);

    bool ContainsComponent(const UStaticMeshComponent* Component) const;
    const TArray<UStaticMeshComponent*>& GetRegisteredComponents() const;

    Uint64 GetSceneId() const;

private:
    static Uint64 GetComponentKey(FObjectHandle Handle);
    FPrimitiveSceneUpdate& FindOrAddPrimitiveUpdate(FObjectHandle Handle);
    void FlushDeferredRenderUpdates();
    void FinishPrimitiveUpdates(FSceneRenderData& Scene);

    void OnDeinitialize() override;

private:
    TArray<UStaticMeshComponent*> mComponents{};
    std::unordered_map<Uint64, std::size_t> mComponentIndices{};

    TArray<TObjectRef<UActorComponent>> mDirtyComponents{};
    std::unordered_set<Uint64> mDirtyComponentKeys{};

    TArray<FPrimitiveSceneUpdate> mPrimitiveUpdates{};
    std::unordered_map<Uint64, std::size_t> mPrimitiveUpdateIndices{};

    Uint64 mSceneId{};
    Uint64 mRevision{};
};
