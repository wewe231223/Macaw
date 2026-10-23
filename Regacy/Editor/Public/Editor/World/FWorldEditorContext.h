/**
 * @file FWorldEditorContext.h
 * @brief 월드별 선택 상태와 편집 명령 채널을 정의합니다.
 */
#pragma once
#include "Core/Base/FAssetHandle.h"
#include "CoreUObject/TObjectRef.h"
#include "Core/Channel/FMessageChannel.h"
#include "Editor/Message/FEditorInfo.h"
#include "World/AActor.h"
#include "World/Component/UActorComponent.h"
#include "World/Component/USceneComponent.h"
#include "World/IWorldObserver.h"
#include "Editor/Input/Messages/FKeyboardCameraMoveRequestMessage.h"
#include "Editor/Input/Messages/FMouseCameraDollyRequestMessage.h"
#include "Editor/Input/Messages/FMouseCameraMoveRequestMessage.h"
#include "Editor/Input/Messages/FMouseCameraRotateRequestMessage.h"
#include "Editor/Input/Messages/FMousePickRequestMessage.h"

/** @brief 월드별 선택 상태와 편집 요청을 관리합니다. 공용 설정과 에셋은 각 사용처에서 전역 서비스로 조회합니다. */
class FWorldEditorContext final : public IWorldObserver {
public:
    /** @brief 월드와 연결하지 않은 편집 컨텍스트를 생성합니다. */
    FWorldEditorContext() = default;
    /** @brief 월드 관찰과 선택 상태를 해제합니다. */
    ~FWorldEditorContext() override;

    /** @brief 월드 관찰자의 복사 생성을 금지합니다. */
    FWorldEditorContext(const FWorldEditorContext&) = delete;
    /** @brief 월드 관찰자의 복사 대입을 금지합니다. */
    FWorldEditorContext& operator=(const FWorldEditorContext&) = delete;
    /** @brief 등록된 관찰자 주소를 유지하기 위해 이동 생성을 금지합니다. */
    FWorldEditorContext(FWorldEditorContext&&) = delete;
    /** @brief 등록된 관찰자 주소를 유지하기 위해 이동 대입을 금지합니다. */
    FWorldEditorContext& operator=(FWorldEditorContext&&) = delete;

public:
    /** @brief 관찰할 월드를 교체하고 선택 상태를 초기화합니다. @param InWorld 새 월드이며 nullptr이면 연결을 해제합니다. */
    void SetWorld(UWorld* InWorld);
    /**
     * @brief 이 월드에 적용할 편집 명령 채널을 연결합니다.
     * @details 엔진과 에셋 서비스는 명령 실행 시 전역 접근점에서 조회합니다. 엔진 인스턴스나 레지스트리 포인터를 캡처하지 않습니다.
     */
    void InitializeChannels();
    /** @brief 대기 중인 편집 명령을 실행합니다. */
    void Dispatch();
    /** @brief 월드가 초기화되어 편집 가능한 상태인지 반환합니다. */
    bool IsWorldEditingAllowed() const;
    /** @brief 월드 실행 상태와 PIE 세션을 고려해 씬 로드 가능 여부를 반환합니다. */
    bool IsSceneLoadingAllowed() const;

    /** @brief 마우스 피킹 요청으로 선택 상태를 갱신합니다. @param Message 피킹 요청입니다. */
    void HandleMousePickRequest(const FMousePickRequestMessage& Message);
    /**
     * @brief 전역 에셋 서비스에서 필요한 에셋을 조회해 이 월드에 컴포넌트를 생성합니다.
     * @param Message 생성 요청입니다.
     */
    void HandleSpawnComponent(const FMessageSpawnComponent& Message);
#ifdef OBJ_VIEWER
    /** @brief 뷰어의 마우스 카메라 회전 요청을 처리합니다. @param Message 회전 요청입니다. */
    void HandleMouseCameraRotateRequest(const FMouseCameraRotateRequestMessage& Message);
    /** @brief 뷰어의 키보드 카메라 이동 요청을 처리합니다. @param Message 이동 요청입니다. */
    void HandleKeyboardCameraMoveRequest(const FKeyboardCameraMoveRequestMessage& Message);
    /** @brief 뷰어의 마우스 카메라 이동 요청을 처리합니다. @param Message 이동 요청입니다. */
    void HandleMouseCameraMoveRequestMessage(const FMouseCameraMoveRequestMessage& Message);
    /** @brief 뷰어의 마우스 카메라 돌리 요청을 처리합니다. @param Message 돌리 요청입니다. */
    void HandleMouseCameraDollyRequestMessage(const FMouseCameraDollyRequestMessage& Message);
#endif

    /** @brief 편집 요청을 월드로 전달하는 송신자를 반환합니다. */
    FMessageChannel::FSender GetEditorToWorldSender();

    /** @brief 이 편집 컨텍스트의 렌더 모드 인덱스를 반환합니다. */
    const std::size_t GetRenderModeState() const noexcept;
    /** @brief 이 편집 컨텍스트의 렌더 모드를 변경합니다. @param State 새 모드 인덱스입니다. */
    void SetRenderModeState(const std::size_t State);

    /** @brief 현재 기즈모 조작 모드를 반환합니다. */
    EGizmoMode GetGizmoMode() const;
    /** @brief 기즈모 조작 중이 아니면 모드를 변경합니다. @param Mode 새 조작 모드입니다. */
    void SetGizmoMode(EGizmoMode Mode);
    /** @brief 현재 기즈모 좌표계를 반환합니다. */
    EGizmoCoordinateSpace GetGizmoCoordinateSpace() const;
    /** @brief 기즈모 조작 중이 아니면 좌표계를 변경합니다. @param CoordinateSpace 새 좌표계입니다. */
    void SetGizmoCoordinateSpace(EGizmoCoordinateSpace CoordinateSpace);

    /** @brief 이 월드의 단일 액터를 선택합니다. @param Actor 선택할 액터이며 nullptr이면 선택을 해제합니다. */
    void SetSelectedActor(AActor* Actor);
    /** @brief 선택한 컴포넌트를 변경합니다. @param Component 새 컴포넌트입니다. */
    void SetSelectedComponent(UActorComponent* Component);
    /** @brief 이 컨텍스트의 액터와 컴포넌트 선택을 해제합니다. */
    void ClearSelection();
    /** @brief 액터 선택 여부를 반전합니다. @param Actor 이 월드에 속한 대상 액터입니다. */
    void ToggleSelectedActor(AActor* Actor);
    /** @brief 이 월드의 액터들로 선택 목록을 교체합니다. @param Actors 새 선택 목록입니다. */
    void SetSelectedActors(const TArray<AActor*>& Actors);
    /** @brief 지정 액터가 선택되었는지 반환합니다. @param Actor 검사할 액터입니다. */
    bool IsActorSelected(const AActor* Actor) const;
    /** @brief 현재 유효한 선택 액터 목록을 반환합니다. */
    TArray<AActor*> GetSelectedActors() const;
    /** @brief 선택 상태에 따라 변환을 적용할 씬 컴포넌트 목록을 반환합니다. */
    TArray<USceneComponent*> GetSelectedTransformTargets() const;
    /** @brief 선택 상태 변경을 감지하는 리비전을 반환합니다. */
    Uint64 GetSelectionRevision() const;
    /** @brief 현재 기즈모 피벗 위치를 반환합니다. */
    FVector3 GetGizmoPivot() const;
    /** @brief 기즈모의 명시적 피벗 위치를 설정합니다. @param Pivot 새 피벗 위치입니다. */
    void SetGizmoPivot(const FVector3& Pivot);
    /** @brief 명시적 기즈모 피벗을 해제합니다. */
    void ResetGizmoPivot();
    /** @brief 명시적 기즈모 피벗이 있는지 반환합니다. */
    bool HasGizmoPivot() const;
    /** @brief 현재 기즈모를 조작 중인지 반환합니다. */
    bool IsGizmoInteracting() const;
    /** @brief 기즈모 조작 상태를 변경합니다. @param Interacting 조작 중 여부입니다. */
    void SetGizmoInteracting(bool Interacting);

    /** @brief 대표 선택 액터를 반환하며 유효하지 않으면 nullptr입니다. */
    AActor* GetSelectedActor() const noexcept;
    /** @brief 선택 컴포넌트를 반환하며 유효하지 않으면 nullptr입니다. */
    UActorComponent* GetSelectedComponent() const noexcept;
    /** @brief 대표 변환 대상을 반환하며 유효하지 않으면 nullptr입니다. */
    USceneComponent* GetSelectedTransformTarget() const noexcept;

    /** @brief 이 컨텍스트가 관찰하는 월드를 반환하며 미연결 상태이면 nullptr입니다. */
    UWorld* GetWorld() const;

    // 프리뷰 대상을 바꾸면 Viewer 창을 띄워달라는 요청도 같이 세운다.
    /** @brief 프리뷰 메시와 뷰어 열기 요청을 설정합니다. @param Handle 표시할 메시 핸들입니다. */
    void SetPreviewMesh(const FAssetHandle& Handle);
    /** @brief 현재 프리뷰 메시 핸들을 반환합니다. */
    FAssetHandle GetPreviewMesh() const noexcept;
    /** @brief 대기 중인 프리뷰 메시 핸들을 반환하고 요청을 소비합니다. */
    FAssetHandle ConsumePreviewMesh() noexcept;

    // 요청을 한 번만 처리하도록 읽으면서 내린다.
    /** @brief 뷰어 열기 요청을 반환하고 소비합니다. */
    bool ConsumePreviewOpenRequest() noexcept;

private:
    /** @brief 월드 제거와 액터 제거에 따라 선택 상태를 정리합니다. @param World 변경된 월드입니다. @param Change 변경 종류입니다. @param Actor 관련 액터입니다. */
    void OnWorldChanged(UWorld& World, EWorldChange Change, AActor* Actor) override;

private:
    /** @brief 관찰하는 월드의 비소유 참조입니다. */
    UWorld* mWorld{nullptr};
    /** @brief 대표 선택 액터의 검증 가능한 참조입니다. */
    TObjectRef<AActor> mSelectedActor{};
    /** @brief 선택 컴포넌트의 검증 가능한 참조입니다. */
    TObjectRef<UActorComponent> mSelectedComponent{};
    /** @brief 이 컨텍스트에 한정된 다중 선택 목록입니다. */
    TArray<TObjectRef<AActor>> mSelectedActors{};
    /** @brief 선택 상태 변경 리비전입니다. */
    Uint64 mSelectionRevision{};
    /** @brief 명시적으로 지정한 기즈모 피벗입니다. */
    FVector3 mGizmoPivot{};
    /** @brief 명시적 피벗의 유효 여부입니다. */
    bool mHasGizmoPivot{};
    /** @brief 기즈모 조작 진행 여부입니다. */
    bool mGizmoInteracting{};
    /** @brief 컨텍스트별 렌더 모드 인덱스입니다. */
    std::size_t mRenderModeIndex{2};
    /** @brief 컨텍스트별 기즈모 조작 모드입니다. */
    EGizmoMode mGizmoMode{EGizmoMode::Translate};
    /** @brief 컨텍스트별 기즈모 좌표계입니다. */
    EGizmoCoordinateSpace mGizmoCoordinateSpace{EGizmoCoordinateSpace::World};
    /** @brief 이 월드에 적용할 편집 명령 채널입니다. */
    FMessageChannel mEditorToWorld{64};

    /** @brief 프리뷰에서 표시할 메시 핸들입니다. */
    FAssetHandle mPreviewMesh{};
    /** @brief 소비되기 전의 뷰어 열기 요청입니다. */
    bool mBPreviewOpenRequested{false};
};
