#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include <cstddef>
#include "World/Tick/FActorComponentTickFunction.h"
#include "CoreUObject/UObject.h"
#include "Core/Archive/FArchive.h"
#include "Core/Base/TStateFlags.h"
#include "World/EEndPlayReason.h"

class AActor;
class UWorld;

class UActorComponent : public UObject {
public:
    ReflectTypes(UActorComponent, UObject);

private:
    enum class EStateFlag {
        WantsInitialize,
        Registering,
        Unregistering,
        Registered,
        Initialized,
        HasBegunPlay,
        BeingDestroyed,
        DestroyRequested,
        RenderStateCreated,
        RenderStateDirty,
        RenderTransformDirty,
        RenderDynamicDataDirty,
        Count
    };

public:
    UActorComponent();
    ~UActorComponent() override = default;

    UActorComponent(const UActorComponent&) = delete;
    UActorComponent& operator=(const UActorComponent&) = delete;

    UActorComponent(UActorComponent&&) = delete;
    UActorComponent& operator=(UActorComponent&&) = delete;

public:
    ReflectClass(UActorComponent, EClassFlags::None);

    AActor* GetOwner() const;
    FName GetDefaultComponentName() const;

    virtual void OnRegister();
    virtual void InitializeComponent();
    /// <summary>플레이 종료나 파괴 시 초기화한 상태를 정리하는 콜백입니다.</summary>
    virtual void UninitializeComponent();
    virtual void BeginPlay();
    /// <summary>플레이가 끝날 때 종료 이유와 함께 호출됩니다.</summary>
    virtual void EndPlay(EEndPlayReason Reason);
    /// <summary>World가 Actor Tick과 별도로 호출하는 컴포넌트의 프레임 갱신입니다.</summary>
    virtual void TickComponent(float DeltaTime);
    virtual void OnUnregister();

    virtual void OnRenderStateChanged();
    virtual bool ShouldCreateRenderState() const;
    virtual void CreateRenderState();
    virtual void DestroyRenderState();
    virtual void SendRenderTransform();
    virtual void SendRenderDynamicData();

    bool IsRenderStateCreated() const;
    void MarkRenderStateDirty();
    void MarkRenderTransformDirty();
    void MarkRenderDynamicDataDirty();
    void DoDeferredRenderUpdates();
    ReflectFunction(IsActive);
    bool IsActive() const;
    ReflectFunction(SetActive, FName{"BInActive"});
    void SetActive(bool BInActive);
    /// <summary>컴포넌트를 활성화하고 Tick을 켭니다.</summary>
    ReflectFunction(Activate);
    virtual void Activate();
    /// <summary>컴포넌트를 비활성화하고 Tick을 끕니다.</summary>
    ReflectFunction(Deactivate);
    virtual void Deactivate();

    /// <summary>현재 Tick 활성 상태와 별개로 컴포넌트가 Tick을 지원하는지 반환합니다.</summary>
    bool CanEverTick() const;
    /// <summary>Tick 지원 여부를 설정하고 World의 Tick 등록을 갱신합니다.</summary>
    void SetCanEverTick(bool CanEverTick);
    ReflectFunction(IsTickEnabled);
    bool IsTickEnabled() const;
    ReflectFunction(SetTickEnabled, FName{"TickEnabled"});
    void SetTickEnabled(bool TickEnabled);
    bool IsTickInEditor() const;
    void SetTickInEditor(bool TickInEditor);
    bool IsTickWhenPaused() const;
    void SetTickWhenPaused(bool TickWhenPaused);
    const FActorComponentTickFunction& GetTickFunction() const;

    /// <summary>플레이 초기화 과정에서 InitializeComponent 호출이 필요한지 지정합니다.</summary>
    void SetWantsInitializeComponent(bool WantsInitializeComponent);
    /// <summary>컴포넌트 파괴 처리가 시작됐는지 반환합니다.</summary>
    bool IsBeingDestroyed() const;

    bool IsRegistered() const;
    bool IsInitialized() const;
    bool HasBegunPlay() const;
    UWorld* GetBelongingWorld() const;

    void RegisterComponent(UWorld* World);
    void UnregisterComponent();

    /// <summary>Component를 등록 해제하고 소유 Actor에서 제거합니다.</summary>
    /// <param name="bPromoteChildren">SceneComponent 자식을 부모에게 승격할지 여부입니다.</param>
    virtual void DestroyComponent(bool BPromoteChildren = false);

    virtual bool ResolveLoadedReferences();

    void Serialize(FArchive& Archive) override;

private:
    friend class AActor;
    friend class FWorldSerializer;
    friend class UWorld;
    friend class USceneComponent;

    void SetOwner(AActor* InOwner);
    bool CanChangeOuter(const UObject* NewOuter) const override;
    void OnIdentityChanged() override;
    void UpdateTickRegistration();
    bool DeferComponentDestruction(bool BPromoteChildren);
    void ApplyDestruction(bool BPromoteChildren);
    /// <summary>초기화를 요청한 등록 컴포넌트의 상태를 갱신하고 초기화 콜백을 호출합니다.</summary>
    void DispatchInitializeComponent();
    /// <summary>초기화된 상태를 해제하고 정리 콜백을 한 번 호출합니다.</summary>
    void DispatchUninitializeComponent();
    void DispatchBeginPlay();
    /// <summary>플레이 상태와 Tick 등록을 갱신하고 종료 이유를 전달합니다.</summary>
    void DispatchEndPlay(EEndPlayReason Reason);

private:
    FName mDefaultComponentName{};
    AActor* mOwner{nullptr};
    UWorld* mParentWorld{nullptr};
    FActorComponentTickFunction mPrimaryComponentTick{*this};

    TStateFlags<EStateFlag> mState{};
    ReflectAccessor(mBActive, IsActive, SetActive, "bActive", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mBActive), ReflectEditor("Component", "Active"));
    bool mBActive{true};

    ReflectMembers();
};
