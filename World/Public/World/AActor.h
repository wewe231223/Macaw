#pragma once
#include "Core/Common.h"
#include <limits>
#include "CoreUObject/UObject.h"
#include "World/Component/UActorComponent.h"
#include "World/Component/USceneComponent.h"

class UWorld;
class ULevel;

/// <summary>
/// World에 소속되며 UActorComponent를 RAII 방식으로 소유하는 게임플레이 객체입니다.
/// Actor의 월드 Transform은 RootComponent의 ComponentToWorld Transform으로 정의됩니다.
/// </summary>
class AActor : public UObject {
public:
    /// <summary>새 Actor를 생성합니다. World 소속과 Component 등록은 아직 수행하지 않습니다.</summary>
    AActor() = default;
    /// <summary>World 및 소유 Component와의 수명 관계를 정리합니다.</summary>
    ~AActor() override;

    AActor(const AActor&) = delete;
    AActor& operator=(const AActor&) = delete;

    AActor(AActor&&) = delete;
    AActor& operator=(AActor&&) = delete;

public:
    JG_DECLARE_DERIVED_TYPEINFO(AActor, UObject)

    /// <summary>Actor가 소유하는 새 Component를 생성하고 추가합니다.</summary>
    /// <typeparam name="T">생성할 UActorComponent 파생 타입입니다.</typeparam>
    /// <returns>Actor가 소유하는 새 Component입니다.</returns>
    template <typename T>
        requires std::is_base_of_v<UActorComponent, T>
    T* AddComponent();

    UActorComponent* AddComponent(const FTypeInfo& Type);

    /// <summary>지정한 타입과 호환되는 첫 번째 소유 Component를 찾습니다.</summary>
    /// <typeparam name="T">찾을 UActorComponent 파생 타입입니다.</typeparam>
    /// <returns>찾은 Component 또는 없으면 nullptr입니다.</returns>
    template <typename T>
        requires std::is_base_of_v<UActorComponent, T>
    T* GetComponent();

    /// <summary>Actor가 RAII 방식으로 소유하는 모든 Component를 반환합니다.</summary>
    const std::vector<std::unique_ptr<UActorComponent>>& GetComponents() const;

    /// <summary>Actor의 RootComponent를 반환합니다.</summary>
    USceneComponent* GetRootComponent();
    /// <summary>Actor의 RootComponent를 읽기 전용으로 반환합니다.</summary>
    const USceneComponent* GetRootComponent() const;

    /// <summary>현재 소속된 World를 반환합니다.</summary>
    UWorld* GetWorld() const;
    ULevel* GetLevel() const;
    /// <summary>소속 World에 Actor의 지연 파괴를 요청합니다.</summary>
    /// <returns>파괴 요청이 수락되었으면 true입니다.</returns>
    bool Destroy();
    /// <summary>Actor가 소유한 SceneComponent를 RootComponent로 지정합니다.</summary>
    /// <param name="InRootComponent">새 RootComponent 또는 nullptr입니다.</param>
    /// <returns>Component가 Actor 소유이거나 nullptr이면 true입니다.</returns>
    bool SetRootComponent(USceneComponent* InRootComponent);

    /// <summary>RootComponent 기준의 Actor 월드 Transform을 반환합니다.</summary>
    FTransform GetActorTransform() const;
    /// <summary>RootComponent를 통해 Actor 월드 Transform을 설정합니다.</summary>
    /// <param name="Transform">설정할 월드 Transform입니다.</param>
    /// <returns>RootComponent가 있어 Transform을 설정했으면 true입니다.</returns>
    bool SetActorTransform(const FTransform& Transform);
    /// <summary>Actor의 월드 위치를 반환합니다.</summary>
    FVector3 GetActorLocation() const;
    /// <summary>Actor의 월드 위치를 설정합니다.</summary>
    /// <param name="Location">설정할 월드 위치입니다.</param>
    /// <returns>RootComponent가 있어 위치를 설정했으면 true입니다.</returns>
    bool SetActorLocation(const FVector3& Location);
    /// <summary>Actor의 월드 위치와 회전을 한 번에 설정합니다.</summary>
    /// <param name="Location">설정할 월드 위치입니다.</param>
    /// <param name="Rotation">설정할 월드 회전입니다.</param>
    /// <returns>RootComponent가 있어 Transform을 설정했으면 true입니다.</returns>
    bool SetActorLocationAndRotation(const FVector3& Location, const FRotator& Rotation);
    /// <summary>Actor의 월드 회전을 반환합니다.</summary>
    FRotator GetActorRotation() const;
    /// <summary>Actor의 월드 회전을 설정합니다.</summary>
    /// <param name="Rotation">설정할 월드 회전입니다.</param>
    /// <returns>RootComponent가 있어 회전을 설정했으면 true입니다.</returns>
    bool SetActorRotation(const FRotator& Rotation);
    /// <summary>Actor의 월드 스케일을 반환합니다.</summary>
    FVector3 GetActorScale3D() const;
    /// <summary>Actor의 월드 스케일을 설정합니다.</summary>
    /// <param name="Scale">설정할 월드 스케일입니다.</param>
    /// <returns>RootComponent가 있어 스케일을 설정했으면 true입니다.</returns>
    bool SetActorScale3D(const FVector3& Scale);

    /// <summary>RootComponent 기준의 Actor 상대 Transform을 반환합니다.</summary>
    FTransform GetActorRelativeTransform() const;
    /// <summary>RootComponent 기준의 Actor 상대 Transform을 설정합니다.</summary>
    /// <param name="Transform">설정할 부모 기준 Transform입니다.</param>
    /// <returns>RootComponent가 있어 Transform을 설정했으면 true입니다.</returns>
    bool SetActorRelativeTransform(const FTransform& Transform);
    /// <summary>Actor의 부모 기준 상대 위치를 반환합니다.</summary>
    FVector3 GetActorRelativeLocation() const;
    /// <summary>Actor의 부모 기준 상대 위치를 설정합니다.</summary>
    /// <param name="Location">설정할 부모 기준 위치입니다.</param>
    /// <returns>RootComponent가 있어 위치를 설정했으면 true입니다.</returns>
    bool SetActorRelativeLocation(const FVector3& Location);
    /// <summary>Actor의 부모 기준 상대 위치와 회전을 한 번에 설정합니다.</summary>
    /// <param name="Location">설정할 부모 기준 위치입니다.</param>
    /// <param name="Rotation">설정할 부모 기준 회전입니다.</param>
    /// <returns>RootComponent가 있어 Transform을 설정했으면 true입니다.</returns>
    bool SetActorRelativeLocationAndRotation(const FVector3& Location, const FRotator& Rotation);
    /// <summary>Actor의 부모 기준 상대 회전을 반환합니다.</summary>
    FRotator GetActorRelativeRotation() const;
    /// <summary>Actor의 부모 기준 상대 회전을 설정합니다.</summary>
    /// <param name="Rotation">설정할 부모 기준 회전입니다.</param>
    /// <returns>RootComponent가 있어 회전을 설정했으면 true입니다.</returns>
    bool SetActorRelativeRotation(const FRotator& Rotation);
    /// <summary>Actor의 부모 기준 상대 스케일을 반환합니다.</summary>
    FVector3 GetActorRelativeScale3D() const;
    /// <summary>Actor의 부모 기준 상대 스케일을 설정합니다.</summary>
    /// <param name="Scale">설정할 부모 기준 스케일입니다.</param>
    /// <returns>RootComponent가 있어 스케일을 설정했으면 true입니다.</returns>
    bool SetActorRelativeScale3D(const FVector3& Scale);

    /// <summary>Actor가 BeginPlay 수명 주기를 완료했는지 반환합니다.</summary>
    bool HasBegunPlay() const;
    /// <summary>파괴 요청을 받아 추가 동작을 중단한 상태인지 반환합니다.</summary>
    bool IsBeingDestroyed() const;
    /// <summary>컴포넌트 등록과 플레이를 진행할 수 있도록 생성 구성을 마쳤는지 반환합니다.</summary>
    bool HasFinishedSpawning() const;
    /// <summary>현재 Tick 활성 상태와 별개로 Actor가 Tick을 지원하는지 반환합니다.</summary>
    bool CanEverTick() const;
    /// <summary>Actor의 Tick 지원 여부를 설정하고 World의 Tick 등록을 갱신합니다.</summary>
    void SetCanEverTick(bool CanEverTick);
    bool IsTickEnabled() const;
    void SetTickEnabled(bool TickEnabled);
    bool IsTickInEditor() const;
    void SetTickInEditor(bool TickInEditor);
    /// <summary>World가 호출하는 Actor 자체의 프레임 갱신입니다.</summary>
    /// <param name="DeltaTime">이전 프레임 이후 경과 시간입니다.</param>
    virtual void Tick(float DeltaTime);

    /// <summary>직렬화 데이터에서 Component 인스턴스와 Object 등록 정보를 먼저 생성합니다.</summary>
    /// <param name="Archive">로드 중인 아카이브입니다.</param>
    /// <returns>모든 Component를 만들고 등록했으면 true입니다.</returns>
    bool PreLoadComponents(FArchive& Archive, bool RegisterComponents = true);
    /// <summary>Component와 RootComponent의 지연 참조를 해석합니다.</summary>
    /// <returns>모든 참조를 해석했으면 true입니다.</returns>
    bool ResolveLoadedReferences();

private:
    /// <summary>Actor가 World에 추가된 직후 호출됩니다.</summary>
    virtual void OnAddedToWorld();
    /// <summary>등록된 Component를 초기화할 때 호출됩니다.</summary>
    void InitializeComponents();
    /// <summary>Actor와 Component의 플레이 시작 시점에 호출됩니다.</summary>
    virtual void BeginPlay();
    /// <summary>Actor와 Component의 플레이 종료 시점에 호출됩니다.</summary>
    virtual void EndPlay(EEndPlayReason Reason);
    /// <summary>새 Actor가 World에 추가된 뒤 생성 구성을 시작하기 전에 호출됩니다.</summary>
    virtual void PostActorCreated();
    /// <summary>장면 데이터와 객체 참조를 복원한 뒤 컴포넌트 등록 전에 호출됩니다.</summary>
    virtual void PostLoad();
    /// <summary>Actor 생성 완료 단계에서 컴포넌트와 초기 속성을 구성합니다.</summary>
    virtual void OnConstruction(const FTransform& Transform);
    /// <summary>소유 컴포넌트의 플레이용 초기화를 시작하기 전에 호출됩니다.</summary>
    virtual void PreInitializeComponents();
    /// <summary>소유 컴포넌트의 플레이용 초기화를 마친 뒤 호출됩니다.</summary>
    virtual void PostInitializeComponents();
    /// <summary>명시적인 Actor 파괴 요청을 처리할 때 호출됩니다.</summary>
    virtual void Destroyed();
    /// <summary>Actor가 World에서 제거되기 전에 호출됩니다.</summary>
    virtual void OnRemovedFromWorld();

    void Serialize(FArchive& Archive) override;
    bool CanChangeOuter(const UObject* NewOuter) const override;
    void OnIdentityChanged() override;

private:
    friend class UActorComponent;
    friend class UWorld;
    friend class FSceneSerializer;

    void SetWorld(UWorld* InWorld);
    void DispatchBeginPlay();
    /// <summary>종료 이유를 전달하고 Actor와 컴포넌트의 플레이 및 초기화 상태를 정리합니다.</summary>
    void DispatchEndPlay(EEndPlayReason Reason);
    /// <summary>파괴 중이 아닌 소유 컴포넌트를 World에 등록합니다.</summary>
    void RegisterAllComponents();
    /// <summary>콜백에서 소유 목록이 바뀌어도 순회할 수 있도록 현재 컴포넌트 포인터를 복사합니다.</summary>
    TArray<UActorComponent*> GetComponentSnapshot() const;
    void FinishAddingComponent(UActorComponent& Component);
    void RemoveOwnedComponent(UActorComponent* Component);
    void UpdateTickRegistration();

private:
    std::vector<std::unique_ptr<UActorComponent>> mComponents{};
    // 파괴 처리 후 메모리 해제를 기다리는 컴포넌트입니다.
    std::vector<std::unique_ptr<UActorComponent>> mPendingDestroyComponents{};
    std::size_t mTickIndex{std::numeric_limits<std::size_t>::max()};
    USceneComponent* mRootComponent{nullptr};

    FGuid mPendingRootComponentGuid{};

    UWorld* mWorld{nullptr};
    bool mBHasBegunPlay{false};
    bool mInitializingComponents{};
    // 현재 Tick 활성 상태와 별개인 Actor의 Tick 지원 여부입니다.
    bool mCanEverTick{};
    bool mBTickEnabled{};
    bool mTickInEditor{};
    // 소유 컴포넌트의 플레이용 초기화 단계를 완료했는지 나타냅니다.
    bool mActorInitialized{};
    // 생성 구성을 마쳐 컴포넌트 등록과 플레이 시작이 가능한 상태입니다.
    bool mHasFinishedSpawning{};
    // 생성 완료 처리의 중복 진입을 막습니다.
    bool mFinishingSpawning{};
    // 플레이 시작 처리 중에는 종료 요청을 뒤로 미룹니다.
    bool mBeginningPlay{};
    // 플레이 종료 처리의 중복 진입을 막습니다.
    bool mEndingPlay{};
    // 초기화 또는 플레이 시작 콜백 이후 처리할 종료 요청입니다.
    bool mEndPlayRequested{};
    // 파괴가 시작되어 새 동작을 허용하지 않는 상태입니다.
    bool mBIsBeingDestroyed{};
    // 보류한 플레이 종료 요청의 이유입니다.
    EEndPlayReason mEndPlayReason{EEndPlayReason::RemovedFromWorld};
};

template <typename T>
    requires std::is_base_of_v<UActorComponent, T>
T* AActor::AddComponent() {
    static_assert(std::is_same_v<typename T::TypeInfoOwner, T>);

    return static_cast<T*>(AddComponent(*T::StaticTypeInfo()));
}

template <typename T>
    requires std::is_base_of_v<UActorComponent, T>
T* AActor::GetComponent() {
    for (const auto& Component : mComponents) {
        if (Component->GetTypeInfo()->IsA(T::StaticTypeInfo())) {
            return static_cast<T*>(Component.get());
        }
    }

    return nullptr;
}
