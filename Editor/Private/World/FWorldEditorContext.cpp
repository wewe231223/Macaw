#include "pch.h"
#include "CoreUObject/Reflection/UClass.h"
#include "World/Component/UBillboardComponent.h"
#include "Editor/UEditorEngine.h"
#include "Editor/Settings/FEditorConfigManager.h"
#include "Asset/FAssetRegistry.h"
#include "Editor/World/FWorldEditorContext.h"
#include "World/AActor.h"
#include "World/Component/UActorComponent.h"
#include "World/Component/USceneComponent.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "World/UWorld.h"

#include <random>
#include "Core/Stat/Stat.h"
#include "Core/Console/Console.h"
#include "World/Component/ULightComponent.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"

FWorldEditorContext::~FWorldEditorContext() {
    SetWorld(nullptr);
}

void FWorldEditorContext::SetWorld(UWorld* InWorld) {
    if (mWorld == InWorld) {
        return;
    }

    if (mWorld != nullptr) {
        mWorld->RemoveObserver(*this);
    }

    ClearSelection();
    mEditorToWorld.Clear();
    mWorld = InWorld;
    mGizmoInteracting = false;

    if (mWorld != nullptr) {
        mWorld->AddObserver(*this);
    }
}

void FWorldEditorContext::OnWorldChanged(UWorld& World, EWorldChange Change, AActor* Actor) {
    if (&World != mWorld) {
        return;
    }

    if (Change == EWorldChange::Destroying) {
        ClearSelection();
        mEditorToWorld.Clear();
        mWorld = nullptr;
    } else if (Change == EWorldChange::ActorRemoving && IsActorSelected(Actor)) {
        TArray<AActor*> Remaining{GetSelectedActors()};
        std::erase(Remaining, Actor);
        SetSelectedActors(Remaining);
    }
}

void FWorldEditorContext::InitializeChannels() {
    if (mWorld == nullptr)
        return;

    mEditorToWorld.TryBind<FMessageSpawnComponent>([this](const FMessageSpawnComponent& Message) {
        HandleSpawnComponent(Message);
    });

    mEditorToWorld.TryBind<FMessageSaveScene>([](const FMessageSaveScene& Message) {
        if (UEditorEngine* Engine{GetEditorEngine()}) {
            Engine->SaveScene(Message.mSceneName);
        }
    });

    mEditorToWorld.TryBind<FMessageLoadScene>([](const FMessageLoadScene& Message) {
        if (UEditorEngine* Engine{GetEditorEngine()}) {
            Engine->LoadScene(std::filesystem::path{Message.mFilePath.c_str()});
        }
    });
}

void FWorldEditorContext::Dispatch() {
    mEditorToWorld.Dispatch();
}

bool FWorldEditorContext::IsWorldEditingAllowed() const {
    return mWorld != nullptr && mWorld->IsInitialized() && !mWorld->IsBusy();
}

bool FWorldEditorContext::IsSceneLoadingAllowed() const {
    const UEditorEngine* Engine{GetEditorEngine()};

    return IsWorldEditingAllowed() && (Engine == nullptr || !Engine->HasPIESession());
}

FMessageChannel::FSender FWorldEditorContext::GetEditorToWorldSender() {
    return mEditorToWorld.GetSender();
}

const std::size_t FWorldEditorContext::GetRenderModeState() const noexcept {
    return mRenderModeIndex;
}

void FWorldEditorContext::SetRenderModeState(const std::size_t State) {
    mRenderModeIndex = State;
}

EGizmoMode FWorldEditorContext::GetGizmoMode() const {
    return mGizmoMode;
}

void FWorldEditorContext::SetGizmoMode(EGizmoMode Mode) {
    if (!mGizmoInteracting) {
        mGizmoMode = Mode;
    }
}

EGizmoCoordinateSpace FWorldEditorContext::GetGizmoCoordinateSpace() const {
    return mGizmoCoordinateSpace;
}

void FWorldEditorContext::SetGizmoCoordinateSpace(EGizmoCoordinateSpace CoordinateSpace) {
    if (!mGizmoInteracting) {
        mGizmoCoordinateSpace = CoordinateSpace;
    }
}

void FWorldEditorContext::SetSelectedActor(AActor* Actor) {
    if (!IsWorldEditingAllowed()) {
        return;
    }

    if (Actor == nullptr) {
        ClearSelection();
        return;
    }

    mSelectedActor.Set(Actor);
    mSelectedComponent.Set(Actor->GetRootComponent());
    mSelectedActors = {TObjectRef<AActor>{Actor}};
    ResetGizmoPivot();
    ++mSelectionRevision;
}

void FWorldEditorContext::SetSelectedComponent(UActorComponent* Component) {
    if (!IsWorldEditingAllowed()) {
        return;
    }

    if (Component == nullptr || Component->GetOwner() == nullptr) {
        ClearSelection();
        return;
    }

    mSelectedActor.Set(Component->GetOwner());
    mSelectedComponent.Set(Component);
    mSelectedActors = {TObjectRef<AActor>{Component->GetOwner()}};
    ResetGizmoPivot();
    ++mSelectionRevision;
}

void FWorldEditorContext::ClearSelection() {
    mSelectedComponent.Reset();
    mSelectedActor.Reset();
    mSelectedActors.clear();
    ResetGizmoPivot();
    ++mSelectionRevision;
}

void FWorldEditorContext::ToggleSelectedActor(AActor* Actor) {
    if (Actor == nullptr || Actor->GetWorld() != mWorld || !IsWorldEditingAllowed()) {
        return;
    }

    TArray<AActor*> Actors{GetSelectedActors()};
    const auto Found{std::find(Actors.begin(), Actors.end(), Actor)};

    if (Found == Actors.end()) {
        Actors.push_back(Actor);
    } else {
        Actors.erase(Found);
    }

    SetSelectedActors(Actors);
}

void FWorldEditorContext::SetSelectedActors(const TArray<AActor*>& Actors) {
    mSelectedActors.clear();
    mSelectedActor.Reset();
    mSelectedComponent.Reset();

    for (AActor* Actor : Actors) {
        if (Actor != nullptr && Actor->GetWorld() == mWorld && !Actor->IsBeingDestroyed() && !IsActorSelected(Actor)) {
            mSelectedActors.push_back(TObjectRef<AActor>{Actor});
            mSelectedActor.Set(Actor);
            mSelectedComponent.Set(Actor->GetRootComponent());
        }
    }

    ResetGizmoPivot();
    ++mSelectionRevision;
}

bool FWorldEditorContext::IsActorSelected(const AActor* Actor) const {
    return Actor != nullptr && std::any_of(mSelectedActors.begin(), mSelectedActors.end(), [Actor](const TObjectRef<AActor>& Entry) {
        return Entry.Get() == Actor;
    });
}

TArray<AActor*> FWorldEditorContext::GetSelectedActors() const {
    TArray<AActor*> Actors{};

    for (const TObjectRef<AActor>& Entry : mSelectedActors) {
        if (AActor* Actor{Entry.Get()}; Actor != nullptr && !Actor->IsBeingDestroyed()) {
            Actors.push_back(Actor);
        }
    }

    return Actors;
}

TArray<USceneComponent*> FWorldEditorContext::GetSelectedTransformTargets() const {
    TArray<USceneComponent*> Targets{};
    const TArray<AActor*> Actors{GetSelectedActors()};

    if (Actors.size() <= 1) {
        if (USceneComponent* Target{GetSelectedTransformTarget()}) {
            Targets.push_back(Target);
        }

        return Targets;
    }

    for (AActor* Actor : Actors) {
        USceneComponent* Root{Actor->GetRootComponent()};
        bool HasSelectedParent{};

        for (USceneComponent* Parent{Root != nullptr ? Root->GetParent() : nullptr}; Parent != nullptr; Parent = Parent->GetParent()) {
            HasSelectedParent = HasSelectedParent || IsActorSelected(Parent->GetOwner());
        }

        if (Root != nullptr && !HasSelectedParent) {
            Targets.push_back(Root);
        }
    }

    return Targets;
}

Uint64 FWorldEditorContext::GetSelectionRevision() const {
    return mSelectionRevision;
}

FVector3 FWorldEditorContext::GetGizmoPivot() const {
    if (mHasGizmoPivot) {
        return mGizmoPivot;
    }

    const TArray<USceneComponent*> Targets{GetSelectedTransformTargets()};
    FVector3 Pivot{};

    for (const USceneComponent* Target : Targets) {
        Pivot += Target->GetComponentLocation();
    }

    return Targets.empty() ? Pivot : Pivot / static_cast<float>(Targets.size());
}

void FWorldEditorContext::SetGizmoPivot(const FVector3& Pivot) {
    mGizmoPivot = Pivot;
    mHasGizmoPivot = true;
}

void FWorldEditorContext::ResetGizmoPivot() {
    mGizmoPivot = {};
    mHasGizmoPivot = false;
}

bool FWorldEditorContext::HasGizmoPivot() const {
    return mHasGizmoPivot;
}

bool FWorldEditorContext::IsGizmoInteracting() const {
    return mGizmoInteracting;
}

void FWorldEditorContext::SetGizmoInteracting(bool Interacting) {
    mGizmoInteracting = Interacting;
}

AActor* FWorldEditorContext::GetSelectedActor() const noexcept {
    return mSelectedActor.Get();
}

UActorComponent* FWorldEditorContext::GetSelectedComponent() const noexcept {
    return mSelectedComponent.Get();
}

USceneComponent* FWorldEditorContext::GetSelectedTransformTarget() const noexcept {
    UActorComponent* Component{mSelectedComponent.Get()};

    if (Component != nullptr && Component->GetClass()->IsChildOf(USceneComponent::StaticClass())) {
        return static_cast<USceneComponent*>(Component);
    }

    AActor* Actor{mSelectedActor.Get()};

    return Actor != nullptr ? Actor->GetRootComponent() : nullptr;
}

UWorld* FWorldEditorContext::GetWorld() const {
    return mWorld;
}

void FWorldEditorContext::SetPreviewMesh(const FAssetHandle& Handle) {
    mPreviewMesh = Handle;
    mBPreviewOpenRequested = true;
}

FAssetHandle FWorldEditorContext::GetPreviewMesh() const noexcept {
    return mPreviewMesh;
}

FAssetHandle FWorldEditorContext::ConsumePreviewMesh() noexcept {
    const FAssetHandle Handle{mPreviewMesh};

    mPreviewMesh = {};

    return Handle;
}

bool FWorldEditorContext::ConsumePreviewOpenRequest() noexcept {
    const bool Requested{mBPreviewOpenRequested};

    mBPreviewOpenRequested = false;

    return Requested;
}

void FWorldEditorContext::HandleMousePickRequest(const FMousePickRequestMessage& Message) {
    if (!IsWorldEditingAllowed()) {
        return;
    }

    if (mWorld == nullptr) {
        return;
    }

    if (Message.mViewportWidth != 0 && Message.mViewportHeight != 0) {
        const Stat::FScopedPickingStatTimer PickingTimer{};
        const float NdcX{(2.0f * (static_cast<float>(Message.mScreenX) - static_cast<float>(Message.mViewportLeft)) / static_cast<float>(Message.mViewportWidth)) - 1.0f};
        const float NdcY{1.0f - (2.0f * (static_cast<float>(Message.mScreenY) - static_cast<float>(Message.mViewportTop)) / static_cast<float>(Message.mViewportHeight))};

        FMatrix InverseViewProjection{};

        if (!Message.mViewProjection.TryInverse(InverseViewProjection))
            return;

        FVector3 RayOrigin{}, RayEnd{};

        if (!InverseViewProjection.TransformCoord({NdcX, NdcY, 0.0f}, RayOrigin) || !InverseViewProjection.TransformCoord({NdcX, NdcY, 1.0f}, RayEnd))
            return;

        FVector3 RayDirection{RayEnd - RayOrigin};

        if (RayDirection.LengthSquared() > 0.0f) {
            RayDirection.Normalize();

            UPrimitiveComponent* NearestPrimitive{nullptr};
            float NearestDistance{0.0f};
            FMatrix CameraWorld{};

            if (!Message.mView.TryInverse(CameraWorld))
                return;

            mWorld->GetPickingSubsystem().Raycast(FRay{RayOrigin.ToSimpleMath(), RayDirection.ToSimpleMath()}, NearestPrimitive, NearestDistance, &CameraWorld);
#if defined(MacawEnablePickingLog) && MacawEnablePickingLog
            if (NearestPrimitive != nullptr) {
                Console::AddLog(Console::STDOutHandle, ELogLevel::Log, ELogCategory::Etc, "Raycast hit primitive component %f", NearestDistance);
            }
#endif

            AActor* SelectedActor{NearestPrimitive != nullptr ? NearestPrimitive->GetOwner() : nullptr};

            if (SelectedActor != nullptr) {
                if (mWorld != nullptr) {
                    if (Message.mAdditiveSelection) {
                        ToggleSelectedActor(SelectedActor);
                    } else {
                        SetSelectedComponent(NearestPrimitive);
                    }
                }
            } else if (mWorld != nullptr && !Message.mAdditiveSelection) {
                ClearSelection();
            }
        }
    }
}

#ifdef OBJ_VIEWER
void FWorldEditorContext::HandleMouseCameraRotateRequest(const FMouseCameraRotateRequestMessage& Message) {
    if (mWorld == nullptr) {
        return;
    }

    UCameraComponent* Camera{mWorld->GetCameraSubsystem().GetMainCamera()};

    if (Camera == nullptr) {
        return;
    }

    const FEditorSettings& Settings{FEditorConfigManager::Get().GetSettings()};
    const float RotationSensitivity{Settings.mRotationSensitivity * 0.001f};
    constexpr float MaximumForwardUp{0.99f};
    FTransform CameraTransform{Camera->GetRelativeTransform()};
    FVector3 Forward{CameraTransform.ToMatrixNoScale().Forward()};

    Forward.Normalize();

    const float CurrentYaw{std::atan2(Forward.mY, Forward.mX)};
    const float CurrentElevation{std::asin(std::clamp(Forward.mZ, -1.0f, 1.0f))};
    const float MaximumElevation{std::asin(MaximumForwardUp)};
    const float NewYaw{CurrentYaw + Message.DeltaX * RotationSensitivity};
    const float NewElevation{std::clamp(CurrentElevation + Message.DeltaY * RotationSensitivity, -MaximumElevation, MaximumElevation)};

    const FQuat YawRotation{FQuat::CreateFromAxisAngle(FVector3::UnitZ, NewYaw)};
    const FQuat PitchRotation{FQuat::CreateFromAxisAngle(FVector3::UnitY, -NewElevation)};
    FQuat NewRotation{FQuat::Concatenate(YawRotation, PitchRotation)};

    NewRotation.Normalize();
    CameraTransform.SetRotation(NewRotation);

    Camera->SetRelativeTransform(CameraTransform);
}

void FWorldEditorContext::HandleKeyboardCameraMoveRequest(const FKeyboardCameraMoveRequestMessage& Message) {
    if (mWorld == nullptr) {
        return;
    }

    UCameraComponent* Camera{mWorld->GetCameraSubsystem().GetMainCamera()};

    if (Camera == nullptr || Message.DeltaTime <= 0.0f) {
        return;
    }

    const FMatrix CameraWorldMatrix{Camera->GetComponentToWorld()};
    FVector3 MoveDirection{CameraWorldMatrix.Forward() * Message.ForwardAxis + CameraWorldMatrix.Right() * Message.RightAxis};

    if (MoveDirection.LengthSquared() <= 0.0f) {
        return;
    }

    MoveDirection.Normalize();

    const FEditorSettings& Settings{FEditorConfigManager::Get().GetSettings()};

    Camera->SetRelativeLocation(Camera->GetRelativeLocation() + MoveDirection * Settings.mMoveSensitivity * Message.DeltaTime);
}

void FWorldEditorContext::HandleMouseCameraMoveRequestMessage(const FMouseCameraMoveRequestMessage& Message) {
    if (mWorld == nullptr) {
        return;
    }

    UCameraComponent* Camera{mWorld->GetCameraSubsystem().GetMainCamera()};

    if (Camera == nullptr) {
        return;
    }

    const FMatrix CameraWorld{Camera->GetComponentToWorld()};
    FVector3 Right{CameraWorld.Right()};
    FVector3 Up{CameraWorld.Up()};

    Right.Normalize();
    Up.Normalize();

    const FEditorSettings& Settings{FEditorConfigManager::Get().GetSettings()};
    const float PanScale{Settings.mMoveSensitivity * 0.01f};
    const FVector3 Offset{Right * (-Message.DeltaX * PanScale) + Up * (-Message.DeltaY * PanScale)};

    Camera->SetRelativeLocation(Camera->GetRelativeLocation() + Offset);
}

void FWorldEditorContext::HandleMouseCameraDollyRequestMessage(const FMouseCameraDollyRequestMessage& Message) {
    if (mWorld == nullptr) {
        return;
    }

    UCameraComponent* Camera{mWorld->GetCameraSubsystem().GetMainCamera()};

    if (Camera == nullptr) {
        return;
    }

    FVector3 ForwardDirection{Camera->GetComponentToWorld().Forward()};

    ForwardDirection.Normalize();

    const FEditorSettings& Settings{FEditorConfigManager::Get().GetSettings()};
    const float DollySpeed{Settings.mMoveSensitivity * 0.3f};

    Camera->SetRelativeLocation(Camera->GetRelativeLocation() + ForwardDirection * (Message.Steps * DollySpeed));
}
#endif

void FWorldEditorContext::HandleSpawnComponent(const FMessageSpawnComponent& Message) {
    const IAssetRegistry* AssetRegistry{FAssetRegistry::TryGet()};
    if (!IsWorldEditingAllowed()) {
        return;
    }

    if (mWorld == nullptr || AssetRegistry == nullptr) {
        return;
    }

    static std::mt19937 RandomEngine{std::random_device{}()};
    const UClass* ComponentType{FReflectionRegistry::FindClass(Message.mComponentType)};

    if (ComponentType == nullptr || !ComponentType->CanCreateInstance() ||
        !ComponentType->IsChildOf(UActorComponent::StaticClass())) {
        return;
    }

    const bool BIsStaticMesh{ComponentType->IsChildOf(UStaticMeshComponent::StaticClass())};
    const FAssetHandle MeshHandle{BIsStaticMesh ? AssetRegistry->FindAsset(FAssetPath{Message.mMeshType}) : FAssetHandle{}};

    if (BIsStaticMesh && AssetRegistry->ResolveAsset<UMesh>(MeshHandle) == nullptr) {
        return;
    }

    const FAssetHandle PipelineHandle{BIsStaticMesh ? AssetRegistry->FindAsset(FAssetPath{"/Game/Pipeline/Base"}) : FAssetHandle{}};
    const FAssetHandle Materials[]{AssetRegistry->FindAsset(FAssetPath{"/Game/System/Material/Default.mtl"}), AssetRegistry->FindAsset(FAssetPath{"/Game/System/Material/Red.mtl"}), AssetRegistry->FindAsset(FAssetPath{"/Game/System/Material/Green.mtl"}), AssetRegistry->FindAsset(FAssetPath{"/Game/System/Material/Blue.mtl"})};
    const bool IsBillboard{ComponentType->IsChildOf(UBillboardComponent::StaticClass())};
    const bool IsLight{ComponentType->IsChildOf(ULightComponent::StaticClass())};
    const FAssetHandle BillboardPipeline{IsBillboard || IsLight ? AssetRegistry->FindAsset(FAssetPath{"/Game/Pipeline/Billboard.json"}) : FAssetHandle{}};
    const FAssetHandle BillboardTexture{IsBillboard ? AssetRegistry->FindAsset(FAssetPath{"/Game/Texture/Fire+Sparks-Sheet.png"}) : FAssetHandle{}};
    const FAssetHandle LightProxyTexture{IsLight ? AssetRegistry->FindAsset(FAssetPath{"/Game/System/Light.png"}) : FAssetHandle{}};

    std::uniform_int_distribution<std::size_t> MaterialIndex{0, std::size(Materials) - 1};
    const FAssetHandle MaterialHandle{BIsStaticMesh ? Materials[MaterialIndex(RandomEngine)] : FAssetHandle{}};

    std::uniform_real_distribution<float> RandomX{-5.0f, 5.0f};
    std::uniform_real_distribution<float> RandomY{-5.0f, 5.0f};
    std::uniform_real_distribution<float> RandomZ{-3.0f, 3.0f};

    const FVector3 SpawnCenter{0.0f, 0.0f, 5.0f};

    for (Uint32 Index{0}; Index < Message.mSpawnCount; ++Index) {
        AActor* Actor{mWorld->SpawnActorDeferred<AActor>()};

        if (Actor == nullptr) {
            continue;
        }

        UBillboardComponent* LightProxy{};

        if (IsLight) {
            LightProxy = Actor->AddComponent<UBillboardComponent>();

            if (LightProxy == nullptr || !Actor->SetRootComponent(LightProxy)) {
                mWorld->DestroyActor(Actor);
                continue;
            }

            LightProxy->SetPipelineHandle(BillboardPipeline);
            LightProxy->SetTextureHandle(LightProxyTexture);
        }

        UActorComponent* Component{Actor->AddComponent(*ComponentType)};

        if (Component == nullptr) {
            mWorld->DestroyActor(Actor);
            continue;
        }

        if (ComponentType->IsChildOf(USceneComponent::StaticClass())) {
            USceneComponent* SceneComponent{static_cast<USceneComponent*>(Component)};

            if (IsLight) {
                if (!SceneComponent->AttachToComponent(LightProxy)) {
                    mWorld->DestroyActor(Actor);
                    continue;
                }
            } else {
                Actor->SetRootComponent(SceneComponent);
            }

            USceneComponent* SpawnRoot{IsLight ? LightProxy : SceneComponent};

            SpawnRoot->SetRelativeLocation(FVector3{SpawnCenter.mX + RandomX(RandomEngine), SpawnCenter.mY + RandomY(RandomEngine), SpawnCenter.mZ + RandomZ(RandomEngine)});
        }

        if (BIsStaticMesh) {
            auto* StaticMeshComponent{static_cast<UStaticMeshComponent*>(Component)};

            StaticMeshComponent->SetMeshHandle(MeshHandle);
            StaticMeshComponent->SetPipelineHandle(PipelineHandle);
            StaticMeshComponent->SetMaterialHandle(MaterialHandle);
        }

        if (IsBillboard) {
            auto* Billboard{static_cast<UBillboardComponent*>(Component)};

            Billboard->SetPipelineHandle(BillboardPipeline);
            Billboard->SetTextureHandle(BillboardTexture);
        }

        mWorld->FinishSpawningActor(Actor, Actor->GetActorTransform());
    }

    mWorld->FlushPendingDestroyActors();
}
