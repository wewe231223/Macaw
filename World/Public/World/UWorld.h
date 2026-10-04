#pragma once

#include <memory>
#include <optional>
#include "World/AActor.h"
#include "World/EWorldType.h"
#include "World/ULevel.h"
#include "CoreUObject/TSubsystemCollection.h"
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
private:
    /// <summary>생명주기와 알림 콜백 중 객체가 해제되지 않도록 호출 깊이를 추적합니다.</summary>
    class FActorDispatchScope final {
    public:
        /// <summary>World가 있으면 보호할 콜백 범위의 호출 깊이를 증가시킵니다.</summary>
        explicit FActorDispatchScope(UWorld* World);
        /// <summary>콜백 범위를 벗어날 때 호출 깊이를 복원합니다.</summary>
        ~FActorDispatchScope();
        /// <summary>호출 깊이 복원이 중복되지 않도록 복사를 금지합니다.</summary>
        FActorDispatchScope(const FActorDispatchScope&) = delete;
        /// <summary>보호 중인 World가 바뀌지 않도록 복사 대입을 금지합니다.</summary>
        FActorDispatchScope& operator=(const FActorDispatchScope&) = delete;

    private:
        // 콜백 호출 깊이를 추적할 World이며 없으면 보호 동작을 생략합니다.
        UWorld* mWorld{};
    };

public:
    UWorld();
    ~UWorld() override;

public:
    void Initialize(EWorldType WorldType = EWorldType::Editor);
    void CleanupWorld();
    bool IsInitialized() const;
    EWorldType GetWorldType() const;
    ULevel& GetPersistentLevel();
    const ULevel& GetPersistentLevel() const;

    void BeginPlay();
    /// <summary>World의 플레이를 종료하며 콜백이나 Tick 처리 중에는 종료 요청을 보류합니다.</summary>
    void EndPlay(EEndPlayReason Reason = EEndPlayReason::RemovedFromWorld);
    bool HasBegunPlay() const;

    TSubsystemCollection<UWorldSubsystem, UWorld>& GetSubsystems();

    AActor* AddActor(std::unique_ptr<AActor> InActor);

    /// <summary>지정한 Actor 타입을 생성하고 구성과 컴포넌트 등록까지 마칩니다.</summary>
    template <typename T>
        requires std::is_base_of_v<AActor, T>
    T* SpawnActor(const FTransform& Transform = {});

    /// <summary>Actor를 생성하고 구성을 미루며 이후 FinishSpawningActor 호출이 필요합니다.</summary>
    template <typename T>
        requires std::is_base_of_v<AActor, T>
    T* SpawnActorDeferred(const FTransform& Transform = {});

    /// <summary>런타임 타입 정보로 Actor를 생성하고 생성 완료 단계까지 처리합니다.</summary>
    AActor* SpawnActor(const FTypeInfo& Type, const FTransform& Transform = {});
    /// <summary>런타임 타입 정보로 Actor를 생성하고 구성 완료 단계는 보류합니다.</summary>
    AActor* SpawnActorDeferred(const FTypeInfo& Type, const FTransform& Transform = {});
    /// <summary>구성 콜백과 컴포넌트 등록을 수행하고 플레이 중이면 BeginPlay까지 진행합니다.</summary>
    bool FinishSpawningActor(AActor* Actor, const FTransform& Transform);

    /// <summary>메시와 이름표 컴포넌트를 설정한 Actor를 생성합니다.</summary>
    AActor* SpawnStaticMeshActor(const FAssetHandle& MeshHandle, const FAssetHandle& PipelineHandle, const FAssetHandle& MaterialHandle, const FVector3& Position);
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

    JG_DECLARE_DERIVED_TYPEINFO(UWorld, UObject);

    void SetAssetRegistry(const IAssetRegistry* InAssetRegistry, IAssetRegistryMutator* InAssetRegistryMutator = nullptr);

    const IAssetRegistry* GetAssetRegistry() const;
    IAssetRegistryMutator* GetAssetRegistryMutator() const;

    FName MakeUniqueObjectName(std::string_view SourceName);
    AActor* FindActorByName(FName InName) const;

    void MarkStructureDirty();
    Uint64 GetStructureRevision() const;

private:
    friend class FSceneSerializer;
    friend class AActor;
    friend class UActorComponent;
    friend class USceneComponent;

    void NotifyWorldChanged(EWorldChange Change, AActor* Actor = nullptr);
    void ClearActors();
    /// <summary>종료 이유에 따라 Actor를 파괴 상태로 전환하고 메모리 해제를 예약합니다.</summary>
    bool DestroyActorInternal(AActor* Actor, EEndPlayReason Reason);
    void RegisterTickActor(AActor* Actor);
    void UnregisterTickActor(AActor* Actor);
    /// <summary>컴포넌트를 World의 독립적인 Tick 목록에 추가합니다.</summary>
    void RegisterTickComponent(UActorComponent* Component);
    /// <summary>컴포넌트를 Tick 목록에서 제외하고 순회 중이면 빈 항목으로 남깁니다.</summary>
    void UnregisterTickComponent(UActorComponent* Component);
    /// <summary>Tick 순회를 종료하고 Actor와 컴포넌트 목록의 빈 항목을 정리합니다.</summary>
    void FinishTicks();
    /// <summary>Actor 소유권을 Level로 옮기고 객체 등록과 World 연결을 수행합니다.</summary>
    AActor* AddActorInternal(std::unique_ptr<AActor> InActor);
    /// <summary>복원한 Actor의 PostLoad와 컴포넌트 등록 및 필요한 플레이 시작을 처리합니다.</summary>
    void InitializeLoadedActor(AActor& Actor);
    void InitializeSubsystems();
    void DeinitializeSubsystems();
    void RefreshActorTicks();

private:
    Uint64 mStructureRevision{};
    std::vector<IWorldObserver*> mObservers{};

    FWorldTime mTime{};

    std::unique_ptr<ULevel> mPersistentLevel{};
    EWorldType mWorldType{EWorldType::Editor};
    bool mInitialized{};
    bool mHasBegunPlay{};
    bool mEndingPlay{};
    bool mBeginningPlay{};
    bool mEndPlayRequested{};
    // 안전한 시점까지 보류한 플레이 종료 요청의 이유입니다.
    EEndPlayReason mEndPlayReason{EEndPlayReason::RemovedFromWorld};
    std::size_t mActorDispatchDepth{};
    bool mCleaningUp{};
    bool mLoadingScene{};
    bool mFlushingActors{};
    TArray<AActor*> mPendingDestroyActors{};
    TArray<AActor*> mTickActors{};
    // Actor의 Tick 목록과 별도로 실행할 컴포넌트입니다.
    TArray<UActorComponent*> mTickComponents{};
    bool mBTickingActors{};
    bool mTickActorsNeedCompaction{};
    // Tick 도중 제거된 컴포넌트의 빈 항목을 정리해야 하는지 나타냅니다.
    bool mTickComponentsNeedCompaction{};

    const IAssetRegistry* mAssetRegistry{nullptr};
    IAssetRegistryMutator* mAssetRegistryMutator{nullptr};

    TSubsystemCollection<UWorldSubsystem, UWorld> mSubsystems{};
};

template <typename T>
    requires std::is_base_of_v<AActor, T>
T* UWorld::SpawnActor(const FTransform& Transform) {
    static_assert(std::is_same_v<typename T::TypeInfoOwner, T>);

    return static_cast<T*>(SpawnActor(*T::StaticTypeInfo(), Transform));
}

template <typename T>
    requires std::is_base_of_v<AActor, T>
T* UWorld::SpawnActorDeferred(const FTransform& Transform) {
    static_assert(std::is_same_v<typename T::TypeInfoOwner, T>);

    return static_cast<T*>(SpawnActorDeferred(*T::StaticTypeInfo(), Transform));
}
