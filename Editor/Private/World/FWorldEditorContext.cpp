#include "pch.h"
#include "Editor/UEditorEngine.h"
#include "Asset/FAssetRegistry.h"
#include "Serialization/FJsonFile.h"
#include "Editor/World/FWorldEditorContext.h"
#include "World/AActor.h"
#include "World/Component/UActorComponent.h"
#include "World/Component/USceneComponent.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "World/UWorld.h"

#include <random>
#include "Core/Stat/Stat.h"
#include "Core/Console/Console.h"
#include "World/Component/UNameTagComponent.h"
#include "World/Component/ULightComponent.h"
#include "CoreUObject/TypeRegistry.h"
#include <rapidjson/document.h>

namespace {
    bool ReWriteObjFilePath(const std::filesystem::path& MetaPath, const FString& NewObjFilePath) {
        rapidjson::Document Document{};

        if (!FJsonFile::Load(MetaPath, Document)) {
            return false;
        }

        rapidjson::Document::AllocatorType& Allocator{Document.GetAllocator()};

        if (Document.HasMember("FilePath")) {
            Document["FilePath"].SetString(NewObjFilePath.c_str(), Allocator);
        } else {
            Document.AddMember("FilePath", rapidjson::Value(NewObjFilePath.c_str(), Allocator), Allocator);
        }

        return FJsonFile::Save(MetaPath, Document);
    }
}

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
    mWorldToEditor.Clear();
    mWorld = InWorld;

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
        mWorldToEditor.Clear();
        mWorld = nullptr;
    } else if (Change == EWorldChange::ActorRemoving && GetSelectedActor() == Actor) {
        ClearSelection();
    }
}

void FWorldEditorContext::InitializeChannels(UEditorEngine& EditorEngine) {
    if (mWorld == nullptr)
        return;

    const IAssetRegistry* AssetRegistry{&EditorEngine.GetAssetRegistry()};
    mEditorToWorld.TryBind<FMessageSpawnComponent>([this, AssetRegistry](const FMessageSpawnComponent& Message) {
        HandleSpawnComponent(Message, AssetRegistry);
    });

    mEditorToWorld.TryBind<FMessageSaveScene>([&EditorEngine](const FMessageSaveScene& Message) {
        EditorEngine.SaveScene(Message.mSceneName);
    });

    mEditorToWorld.TryBind<FMessageLoadScene>([&EditorEngine](const FMessageLoadScene& Message) {
        EditorEngine.LoadScene(std::filesystem::path{Message.mFilePath.c_str()});
    });
}

void FWorldEditorContext::Dispatch() {
    mEditorToWorld.Dispatch();
    mWorldToEditor.Dispatch();
}

FMessageChannel::FSender FWorldEditorContext::GetEditorToWorldSender() {
    return mEditorToWorld.GetSender();
}

FMessageChannel::FSender FWorldEditorContext::GetWorldToEditorSender() {
    return mWorldToEditor.GetSender();
}

FEditorSettings FWorldEditorContext::GetEditorSettings() const {
    return mSharedState.GetReader().Peek().mEditorSettings;
}

void FWorldEditorContext::SetEditorSettings(const FEditorSettings& Settings) {
    mSharedState.GetWriter().Modify([&Settings](FWorldEditorSharedState& Shared) {
        Shared.mEditorSettings = Settings;
    });
}

void FWorldEditorContext::SetMoveSensitivity(float Value) {
    mSharedState.GetWriter().Modify([Value](FWorldEditorSharedState& Shared) {
        Shared.mEditorSettings.mMoveSensitivity = Value;
    });
}

void FWorldEditorContext::SetRotationSensitivity(float Value) {
    mSharedState.GetWriter().Modify([Value](FWorldEditorSharedState& Shared) {
        Shared.mEditorSettings.mRotationSensitivity = Value;
    });
}

void FWorldEditorContext::SetGridSize(float Value) {
    mSharedState.GetWriter().Modify([Value](FWorldEditorSharedState& Shared) {
        Shared.mEditorSettings.mGridSize = Value;
    });
}

void FWorldEditorContext::SetGridSnapEnabled(bool Enabled) {
    mSharedState.GetWriter().Modify([Enabled](FWorldEditorSharedState& Shared) {
        Shared.mEditorSettings.mGridSnapEnabled = Enabled;
    });
}

void FWorldEditorContext::SetGridVisible(bool Visible) {
    mSharedState.GetWriter().Modify([Visible](FWorldEditorSharedState& Shared) {
        Shared.mEditorSettings.mGridVisible = Visible;
    });
}

void FWorldEditorContext::SetAxisVisible(bool Visible) {
    mSharedState.GetWriter().Modify([Visible](FWorldEditorSharedState& Shared) {
        Shared.mEditorSettings.mAxisVisible = Visible;
    });
}

const std::size_t FWorldEditorContext::GetRenderModeState() const noexcept {
    return mSharedState.GetReader().Peek().mModeIndex;
}

void FWorldEditorContext::SetRenderModeState(const std::size_t State) {
    mSharedState.GetWriter().Modify([&State](FWorldEditorSharedState& Shared) {
        Shared.mModeIndex = State;
    });
}

void FWorldEditorContext::SetSelectedActor(AActor* Actor) {
    if (Actor == nullptr) {
        ClearSelection();
        return;
    }

    mSelectedActor.Set(Actor);
    mSelectedComponent.Set(Actor->GetRootComponent());
}

void FWorldEditorContext::SetSelectedComponent(UActorComponent* Component) {
    if (Component == nullptr || Component->GetOwner() == nullptr) {
        ClearSelection();
        return;
    }

    mSelectedActor.Set(Component->GetOwner());
    mSelectedComponent.Set(Component);
}

void FWorldEditorContext::ClearSelection() {
    mSelectedComponent.Reset();
    mSelectedActor.Reset();
}

AActor* FWorldEditorContext::GetSelectedActor() const noexcept {
    return mSelectedActor.Get();
}

UActorComponent* FWorldEditorContext::GetSelectedComponent() const noexcept {
    return mSelectedComponent.Get();
}

USceneComponent* FWorldEditorContext::GetSelectedTransformTarget() const noexcept {
    UActorComponent* Component{mSelectedComponent.Get()};

    if (Component != nullptr && Component->GetTypeInfo()->IsA(USceneComponent::StaticTypeInfo())) {
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

            AActor* PreviousActor{GetSelectedActor()};
            AActor* SelectedActor{NearestPrimitive != nullptr ? NearestPrimitive->GetOwner() : nullptr};

            if (PreviousActor != nullptr && PreviousActor != SelectedActor) {
                if (UNameTagComponent * NameTag{PreviousActor->GetComponent<UNameTagComponent>()}) {
                    NameTag->SetActive(false);
                }
            }

            if (SelectedActor != nullptr) {
                if (mWorld != nullptr) {
                    SetSelectedComponent(NearestPrimitive);
                }

                if (UNameTagComponent * NameTag{SelectedActor->GetComponent<UNameTagComponent>()}) {
                    NameTag->SetActive(true);
                }
            } else if (mWorld != nullptr) {
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

    const FEditorSettings Settings{GetEditorSettings()};
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

    const FEditorSettings Settings{GetEditorSettings()};

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

    const FEditorSettings Settings{GetEditorSettings()};
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

    const FEditorSettings Settings{GetEditorSettings()};
    const float DollySpeed{Settings.mMoveSensitivity * 0.3f};

    Camera->SetRelativeLocation(Camera->GetRelativeLocation() + ForwardDirection * (Message.Steps * DollySpeed));
}
#endif

void FWorldEditorContext::HandleSpawnComponent(const FMessageSpawnComponent& Message, const IAssetRegistry* AssetRegistry) {
    if (mWorld == nullptr || AssetRegistry == nullptr) {
        return;
    }

    static std::mt19937 RandomEngine{std::random_device{}()};
    const FTypeInfo* ComponentType{TypeRegistry::Find(Message.mComponentType)};

    if (ComponentType == nullptr || ComponentType->mCreator == nullptr ||
        !ComponentType->IsA(UActorComponent::StaticTypeInfo())) {
        return;
    }

    const bool BIsStaticMesh{ComponentType->IsA(UStaticMeshComponent::StaticTypeInfo())};
    const FAssetHandle MeshHandle{BIsStaticMesh ? AssetRegistry->FindAsset(FAssetPath{Message.mMeshType}) : FAssetHandle{}};

    if (BIsStaticMesh && AssetRegistry->ResolveAsset<UMesh>(MeshHandle) == nullptr) {
        return;
    }

    const FAssetHandle PipelineHandle{BIsStaticMesh ? AssetRegistry->FindAsset(FAssetPath{"/Game/Pipeline/Base"}) : FAssetHandle{}};
    const FAssetHandle Materials[]{AssetRegistry->FindAsset(FAssetPath{"/Game/System/Material/Default.mtl"}), AssetRegistry->FindAsset(FAssetPath{"/Game/System/Material/Red.mtl"}), AssetRegistry->FindAsset(FAssetPath{"/Game/System/Material/Green.mtl"}), AssetRegistry->FindAsset(FAssetPath{"/Game/System/Material/Blue.mtl"})};
    const bool IsBillboard{ComponentType->IsA(UBillboardComponent::StaticTypeInfo())};
    const bool IsLight{ComponentType->IsA(ULightComponent::StaticTypeInfo())};
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
        AActor* Actor{mWorld->AdoptActor<AActor>()};

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

        if (ComponentType->IsA(USceneComponent::StaticTypeInfo())) {
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

        auto Tag{Actor->AddComponent<UNameTagComponent>()};

        Tag->SetActive(false);
    }

    mWorld->FlushPendingDestroyActors();
}
