#pragma once
#include "Core/Base/FAssetHandle.h"
#include "CoreUObject/TObjectRef.h"
#include "Core/Channel/FMessageChannel.h"
#include "Core/Channel/FStateChannel.h"
#include "Editor/Message/FEditorInfo.h"
#include "Editor/Settings/FEditorSettings.h"
#include "World/AActor.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "World/Component/UActorComponent.h"
#include "World/Component/USceneComponent.h"
#include "World/IWorldObserver.h"
#include "Editor/Input/Messages/FKeyboardCameraMoveRequestMessage.h"
#include "Editor/Input/Messages/FMouseCameraDollyRequestMessage.h"
#include "Editor/Input/Messages/FMouseCameraMoveRequestMessage.h"
#include "Editor/Input/Messages/FMouseCameraRotateRequestMessage.h"
#include "Editor/Input/Messages/FMousePickRequestMessage.h"

class UEditorEngine;

struct FWorldEditorSharedState {
    FEditorSettings mEditorSettings{};
    std::size_t mModeIndex{2};
};

class FWorldEditorContext final : public IWorldObserver {
public:
    FWorldEditorContext() = default;
    ~FWorldEditorContext() override;

    FWorldEditorContext(const FWorldEditorContext&) = delete;
    FWorldEditorContext& operator=(const FWorldEditorContext&) = delete;
    FWorldEditorContext(FWorldEditorContext&&) = delete;
    FWorldEditorContext& operator=(FWorldEditorContext&&) = delete;

public:
    void SetWorld(UWorld* InWorld);
    void InitializeChannels(UEditorEngine& EditorEngine);
    void Dispatch();

    void HandleMousePickRequest(const FMousePickRequestMessage& Message);
    void HandleSpawnComponent(const FMessageSpawnComponent& Message, const IAssetRegistry* AssetRegistry);
#ifdef OBJ_VIEWER
    void HandleMouseCameraRotateRequest(const FMouseCameraRotateRequestMessage& Message);
    void HandleKeyboardCameraMoveRequest(const FKeyboardCameraMoveRequestMessage& Message);
    void HandleMouseCameraMoveRequestMessage(const FMouseCameraMoveRequestMessage& Message);
    void HandleMouseCameraDollyRequestMessage(const FMouseCameraDollyRequestMessage& Message);
#endif

    FMessageChannel::FSender GetEditorToWorldSender();
    FMessageChannel::FSender GetWorldToEditorSender();

    FEditorSettings GetEditorSettings() const;
    void SetEditorSettings(const FEditorSettings& Settings);
    void SetMoveSensitivity(float Value);
    void SetRotationSensitivity(float Value);
    void SetGridSize(float Value);
    void SetGridSnapEnabled(bool Enabled);
    void SetGridVisible(bool Visible);
    void SetAxisVisible(bool Visible);

    const std::size_t GetRenderModeState() const noexcept;
    void SetRenderModeState(const std::size_t State);

    void SetSelectedActor(AActor* Actor);
    void SetSelectedComponent(UActorComponent* Component);
    void ClearSelection();

    AActor* GetSelectedActor() const noexcept;
    UActorComponent* GetSelectedComponent() const noexcept;
    USceneComponent* GetSelectedTransformTarget() const noexcept;

    UWorld* GetWorld() const;

    // 프리뷰 대상을 바꾸면 Viewer 창을 띄워달라는 요청도 같이 세운다.
    void SetPreviewMesh(const FAssetHandle& Handle);
    FAssetHandle GetPreviewMesh() const noexcept;
    FAssetHandle ConsumePreviewMesh() noexcept;

    // 요청을 한 번만 처리하도록 읽으면서 내린다.
    bool ConsumePreviewOpenRequest() noexcept;

private:
    void OnWorldChanged(UWorld& World, EWorldChange Change, AActor* Actor) override;

private:
    UWorld* mWorld{nullptr};
    TObjectRef<AActor> mSelectedActor{};
    TObjectRef<UActorComponent> mSelectedComponent{};
    FStateChannel<FWorldEditorSharedState> mSharedState{std::in_place};
    FMessageChannel mEditorToWorld{64};
    FMessageChannel mWorldToEditor{64};

    FAssetHandle mPreviewMesh{};
    bool mBPreviewOpenRequested{false};
};
