#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include "World/AActor.h"
#include "World/Component/UCameraComponent.h"
#include "World/Component/UStaticMeshComponent.h"
#include "World/Component/UCollisionComponent.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Asset/IAssetRegistryMutator.h"
#include "Asset/UMesh.h"
#include "CoreUObject/TObjectRef.h"
#include "Core/Common.h"
#include "CoreUObject/UObject.h"
#include "CoreUObject/UObjectSystem.h"
#include "RenderCore/FRenderProbe.h"
#include "World/FWorldTime.h"
#include "World/IWorldObserver.h"
#include "World/Subsystem/UCameraSubsystem.h"
#include "World/Subsystem/UCollisionSubsystem.h"
#include "World/Subsystem/UPickingSubsystem.h"
#include "World/Subsystem/URenderSubsystem.h"
#include "World/Subsystem/UBillboardSubsystem.h"
#include "World/Subsystem/UTextSubsystem.h"
#include "World/Subsystem/ULightSubsystem.h"

class UWorld : public UObject {
public:
    UWorld();
    ~UWorld() override;

public:
    AActor* AddActor(std::unique_ptr<AActor> InActor);

    template <typename T> requires std::is_base_of_v<AActor, T> T* AdoptActor();

    AActor* SpawnActor(const FAssetHandle& MeshHandle, const FAssetHandle& PipelineHandle, const FAssetHandle& MaterialHandle, const FVector3& Position);
    bool DestroyActor(AActor* Actor);
    void FlushPendingDestroyActors();

    void AttachActor(AActor* Child, AActor* Parent);
    void DetachActor(AActor* Actor);
    bool RenameActor(AActor* Actor, const FName& NewName);

    const TArray<std::unique_ptr<AActor>>& GetActors() const;

    void BuildSceneRenderData(FSceneRenderData& Scene);

    void AddObserver(IWorldObserver& Observer);
    void RemoveObserver(IWorldObserver& Observer);

    void Tick(float DeltaTime);

    FWorldTime& GetTime();
    const FWorldTime& GetTime() const;

    URenderSubsystem& GetRenderSubsystem();
    const URenderSubsystem& GetRenderSubsystem() const;

    UCollisionSubsystem& GetCollisionSubsystem();
    const UCollisionSubsystem& GetCollisionSubsystem() const;

    UPickingSubsystem& GetPickingSubsystem();
    const UPickingSubsystem& GetPickingSubsystem() const;

    UCameraSubsystem& GetCameraSubsystem();
    const UCameraSubsystem& GetCameraSubsystem() const;

    UBillboardSubsystem& GetBillboardSubsystem();
    const UBillboardSubsystem& GetBillboardSubsystem() const;

    UTextSubsystem& GetTextSubsystem();
    const UTextSubsystem& GetTextSubsystem() const;

    ULightSubsystem& GetLightSubsystem();
    const ULightSubsystem& GetLightSubsystem() const;

    bool SaveScene(const FString& SceneName, const IAssetRegistry* AssetRegistry);
    bool LoadScene(const std::filesystem::path& ScenePath);

    JG_DECLARE_DERIVED_TYPEINFO(UWorld, UObject);

    void SetAssetRegistry(const IAssetRegistry* InAssetRegistry, IAssetRegistryMutator* InAssetRegistryMutator = nullptr);

    const IAssetRegistry* GetAssetRegistry() const;
    IAssetRegistryMutator* GetAssetRegistryMutator() const;

    FName MakeUniqueObjectName(std::string_view SourceName);
    AActor* FindActorByName(FName InName) const;

    void MarkStructureDirty();
    Uint64 GetStructureRevision() const;

private:
    friend class FTemporarySceneLoader;
    friend class AActor;

    void NotifyWorldChanged(EWorldChange Change, AActor* Actor = nullptr);
    void ClearActors();
    void RegisterTickActor(AActor* Actor);
    void UnregisterTickActor(AActor* Actor);
    void FinishActorTicks(bool WasTicking);
    void InitializeSubsystems();
    void DeinitializeSubsystems();

private:
    Uint64 mStructureRevision{};
    std::vector<IWorldObserver*> mObservers{};

    FWorldTime mTime{};

    TArray<std::unique_ptr<AActor>> mActors{};
    TArray<AActor*> mPendingDestroyActors{};
    TArray<AActor*> mTickActors{};
    bool mBTickingActors{};
    bool mTickActorsNeedCompaction{};

    TArray<UStaticMeshComponent*> mRenderableComponents{};
    TArray<TObjectRef<UCollisionComponent>> mCollisionComponents{};

    const IAssetRegistry* mAssetRegistry{nullptr};
    IAssetRegistryMutator* mAssetRegistryMutator{nullptr};

    std::unique_ptr<URenderSubsystem> mRenderSubsystem{};
    std::unique_ptr<UCollisionSubsystem> mCollisionSubsystem{};
    std::unique_ptr<UPickingSubsystem> mPickingSubsystem{};
    std::unique_ptr<UCameraSubsystem> mCameraSubsystem{};

    std::unique_ptr<UBillboardSubsystem> mBillboardSubsystem{};
    std::unique_ptr<UTextSubsystem> mTextSubsystem{};
    std::unique_ptr<ULightSubsystem> mLightSubsystem{};

};

template <typename T> requires std::is_base_of_v<AActor, T> T* UWorld::AdoptActor() {
    std::unique_ptr<T> NewActor{std::make_unique<T>()};

    T* ActorPtr{NewActor.get()};

    if (AddActor(std::move(NewActor)) == nullptr) {
        return nullptr;
    }

    ActorPtr->SetName(MakeUniqueObjectName(ActorPtr->GetTypeInfo()->mTypeName));

    return ActorPtr;
}
