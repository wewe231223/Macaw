#pragma once
#include "World/Subsystem/UWorldSubsystem.h"
#include "RenderCore/FSceneUpdateBatch.h"
#include "World/Component/UActorComponent.h"
#include "CoreUObject/TObjectRef.h"

#include <unordered_map>
#include <unordered_set>
#include "World/Subsystem/URenderSubsystem.generated.h"

UCLASS()
class URenderSubsystem : public UWorldSubsystem {
public:
    GENERATED_TYPES(URenderSubsystem)

public:
    URenderSubsystem();
    ~URenderSubsystem() override = default;

public:
    GENERATED_BODY(URenderSubsystem)

    void RegisterComponent(UActorComponent* Component);
    void UnregisterComponent(UActorComponent* Component);
    void MarkComponentDirty(UActorComponent* Component);
    void RecreateRenderStates();

    void AddPrimitive(std::unique_ptr<FPrimitiveSceneProxy> Proxy);
    void UpdatePrimitiveTransform(FObjectHandle ComponentHandle, const FPrimitiveTransform& Transform);
    void UpdateBillboardUV(FObjectHandle ComponentHandle, const FVector2& UVMin, const FVector2& UVMax);
    void UpdatePrimitiveUV(FObjectHandle ComponentHandle, std::span<const FPrimitiveUVTransform> Transforms);
    void RemovePrimitive(FObjectHandle ComponentHandle);

    void AddLight(std::unique_ptr<FLightSceneProxy> Proxy);
    void UpdateLightTransform(FObjectHandle ComponentHandle, const FMatrix& World);
    void RemoveLight(FObjectHandle ComponentHandle);

    void BuildSceneUpdates(FSceneUpdateBatch& Updates);

    bool ContainsComponent(const UActorComponent* Component) const;
    const TArray<UActorComponent*>& GetRegisteredComponents() const;

private:
    static Uint64 GetComponentKey(FObjectHandle Handle);
    FPrimitiveSceneUpdate& FindOrAddPrimitiveUpdate(FObjectHandle Handle);
    FLightSceneUpdate& FindOrAddLightUpdate(FObjectHandle Handle);
    void FlushDeferredRenderUpdates();

    void OnDeinitialize() override;

private:
    TArray<UActorComponent*> mComponents{};
    std::unordered_map<Uint64, std::size_t> mComponentIndices{};

    TArray<TObjectRef<UActorComponent>> mDirtyComponents{};
    std::unordered_set<Uint64> mDirtyComponentKeys{};

    TArray<FPrimitiveSceneUpdate> mPrimitiveUpdates{};
    std::unordered_map<Uint64, std::size_t> mPrimitiveUpdateIndices{};

    TArray<FLightSceneUpdate> mLightUpdates{};
    std::unordered_map<Uint64, std::size_t> mLightUpdateIndices{};
};
