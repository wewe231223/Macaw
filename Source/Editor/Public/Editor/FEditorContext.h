/**
 * @file FEditorContext.h
 * @brief UI와 독립적인 편집 대상, 선택, 장면 상태와 공통 편집 경로를 정의합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "Editor/FConsoleCommandRegistry.h"
#include "Editor/FViewportStatistics.h"
#include "CoreUObject/TObjectRef.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include "Engine/FWorldHandle.h"
#include "Engine/FEditorCameraState.h"

#include <filesystem>
#include <functional>

namespace Script {
    struct FScriptField;
}

namespace Engine {
    class ALevelSequenceActor;
    class UEngine;
    class FProject;
    class UWorld;
    class AActor;
    class AStaticMeshActor;
    class ASkeletalMeshActor;
    class ACameraActor;
    class UCameraComponent;
    class UActorComponent;
    class USceneComponent;
    class FWorldContext;

    enum class EPrimitiveType : Core::Uint8;
}

namespace CoreUObject {
    class UObject;
    class UClass;
    class FProperty;
}

namespace RenderCore {
    class IRendererModule;
}

namespace Editor {

    class FEditorViewport;

    /**
     * @brief 명시적인 편집 월드와 선택을 소유하고 패널, 콘솔과 기즈모의 편집 요청을 처리합니다.
     * @ingroup Editor
     * @note 엔진과 객체를 소유하지 않으며 ImGui, 패널과 창에 의존하지 않습니다. 실행 중인 월드 선택과 편집 대상 월드는
     * 별개입니다.
     */
    class FEditorContext {
    private:

        /**
         * @brief 장면의 직렬화 바이트와 선택 객체 GUID를 보관하는 복원 데이터입니다.
         * @ingroup Editor
         */
        struct FSceneMemento {

            /**
             * @brief 장면을 직렬화한 바이트 배열입니다.
             */
            Core::TArray<Core::Uint8> mBytes{};

            /**
             * @brief 복원할 선택 액터 GUID 목록입니다.
             */
            Core::TArray<Core::FGuid> mSelectedActors{};

            /**
             * @brief 복원할 상세 선택 객체 GUID입니다.
             */
            Core::FGuid mSelectedObject{};
        };

        /**
         * @brief 장면 변경 전후의 스냅샷과 리비전을 보관하는 실행 취소 명령입니다.
         * @ingroup Editor
         */
        struct FSceneEditCommand {

            /**
             * @brief 직전 장면 편집 명령의 변경 전 스냅샷을 복원합니다.
             * @param Context 장면을 조회·복원·편집할 에디터 컨텍스트입니다.
             * @return 장면 복원에 성공했으면 true입니다.
             */
            bool Undo(FEditorContext& Context) const;

            /**
             * @brief 다음 장면 편집 명령의 변경 후 스냅샷을 복원합니다.
             * @param Context 장면을 조회·복원·편집할 에디터 컨텍스트입니다.
             * @return 장면 복원에 성공했으면 true입니다.
             */
            bool Redo(FEditorContext& Context) const;

            /**
             * @brief 명령 적용 전 장면 스냅샷입니다.
             */
            FSceneMemento mBefore{};

            /**
             * @brief 명령 적용 후 장면 스냅샷입니다.
             */
            FSceneMemento mAfter{};

            /**
             * @brief 명령 적용 전 장면 리비전입니다.
             */
            Core::Uint64 mBeforeRevision{};

            /**
             * @brief 명령 적용 후 장면 리비전입니다.
             */
            Core::Uint64 mAfterRevision{};
        };

        /**
         * @brief 생성부터 소멸까지의 장면 변경을 하나의 실행 취소 작업으로 묶는 범위 객체입니다.
         * @ingroup Editor
         */
        class FSceneEditScope {
        public:

            /**
             * @brief 장면 편집 범위를 시작하고 소멸 시 종료할 토큰을 보관합니다.
             * @param Context 장면을 조회·복원·편집할 에디터 컨텍스트입니다.
             */
            explicit FSceneEditScope(FEditorContext& Context);

            /**
             * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
             */
            ~FSceneEditScope();

            /**
             * @brief 소유 리소스와 편집 상태의 복사 생성을 금지합니다.
             */
            FSceneEditScope(const FSceneEditScope&) = delete;

            /**
             * @brief 소유 리소스와 편집 상태의 복사 대입을 금지합니다.
             */
            FSceneEditScope& operator=(const FSceneEditScope&) = delete;

        private:

            /**
             * @brief 편집 범위가 참조하는 에디터 컨텍스트입니다.
             */
            FEditorContext& mContext;

            /**
             * @brief 이 범위가 시작한 장면 편집 토큰입니다.
             */
            Core::Uint64 mToken{};
        };

        /**
         * @brief 재진입을 제한하는 연산 플래그의 수명을 관리합니다.
         */
        class FOperationScope {
        public:

            /**
             * @brief 연산 플래그를 설정합니다.
             * @param[in,out] Flag 범위 동안 설정할 플래그입니다.
             */
            explicit FOperationScope(bool* Flag);

            /**
             * @brief 연산 플래그를 해제합니다.
             */
            ~FOperationScope();

            /**
             * @brief 범위의 중복 해제를 막기 위해 복사를 금지합니다.
             */
            FOperationScope(const FOperationScope&) = delete;

            /**
             * @brief 범위 대입을 금지합니다.
             * @return 대입 대상의 참조입니다.
             */
            FOperationScope& operator=(const FOperationScope&) = delete;

        private:

            /**
             * @brief 이 범위가 관리하는 플래그의 비소유 주소입니다.
             */
            bool* mFlag{};
        };

    public:

        /**
         * @brief 편집에 사용할 엔진을 연결합니다.
         * @param[in,out] Engine 컨텍스트보다 오래 유지할 엔진입니다.
         */
        explicit FEditorContext(Engine::UEngine* Engine);

        /**
         * @brief 비소유 연결과 편집 상태의 수명을 끝냅니다.
         */
        ~FEditorContext();

        /**
         * @brief 편집 상태와 콜백 연결의 복사를 금지합니다.
         */
        FEditorContext(const Editor::FEditorContext&) = delete;

        /**
         * @brief 편집 상태의 대입을 금지합니다.
         * @return 대입 대상의 참조입니다.
         */
        Editor::FEditorContext& operator=(const Editor::FEditorContext&) = delete;

    public:

        /**
         * @brief 편집 대상을 연결하고 선택과 장면 상태를 초기화합니다.
         * @param[in] Handle 같은 엔진이 소유한 월드입니다.
         * @return 연결에 성공하면 true입니다.
         */
        bool SetWorld(Engine::FWorldHandle Handle);

        /**
         * @brief 편집 월드 연결, 선택과 장면 상태를 초기화합니다.
         */
        void Reset();

        /**
         * @brief 편집 대상 월드 핸들을 반환합니다.
         * @return 현재 편집 월드의 세대 핸들입니다.
         */
        Engine::FWorldHandle GetWorldHandle() const;

        /**
         * @brief 핸들을 검증하여 편집 월드를 조회합니다.
         * @return 편집 월드의 비소유 주소이며 만료되면 nullptr입니다.
         */
        Engine::UWorld* GetWorld() const;

        /**
         * @brief 편집 월드의 수정 허용 여부를 검사합니다.
         * @return 정지한 Editor 또는 EditorPreview 월드이면 true입니다.
         */
        bool CanEditWorld() const;

        /**
         * @brief 편집 상태 변경을 처리 중이어서 재진입이 제한되는지 반환합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        bool IsChangingState() const;

        /**
         * @brief 실행 취소용 변경 전 스냅샷을 확보하고 편집 범위를 시작합니다.
         * @return 이 범위를 종료할 토큰이며 새 범위를 시작하지 못하면 0입니다.
         */
        Core::Uint64 BeginSceneEdit();

        /**
         * @brief 일치하는 토큰의 편집을 종료하고 변경 전후를 실행 취소 이력에 기록합니다.
         * @param Token BeginSceneEdit가 반환한 편집 범위 토큰입니다.
         */
        void EndSceneEdit(Core::Uint64 Token);

        /**
         * @brief 장면 편집 범위가 진행 중인지 확인합니다.
         * @return 유효한 편집 토큰이 활성화되어 있으면 true입니다.
         */
        bool IsSceneEditActive() const;

        /**
         * @brief 실행 취소할 장면 편집 명령이 있는지 확인합니다.
         * @return 현재 장면을 수정할 수 있고 취소할 명령이 있으면 true입니다.
         */
        bool CanUndo() const;

        /**
         * @brief 다시 실행할 장면 편집 명령이 있는지 확인합니다.
         * @return 현재 장면을 수정할 수 있고 다시 실행할 명령이 있으면 true입니다.
         */
        bool CanRedo() const;

        /**
         * @brief 직전 장면 편집 명령의 변경 전 스냅샷을 복원합니다.
         * @return 장면 복원에 성공했으면 true입니다.
         */
        bool Undo();

        /**
         * @brief 다음 장면 편집 명령의 변경 후 스냅샷을 복원합니다.
         * @return 장면 복원에 성공했으면 true입니다.
         */
        bool Redo();

        /**
         * @brief 엔진의 렌더러를 조회합니다.
         * @return 렌더러의 비소유 주소이며 헤드리스 환경에서는 nullptr입니다.
         */
        RenderCore::IRendererModule* GetRenderer() const;

        /**
         * @brief 편집 월드에서 살아 있는 액터 핸들의 스냅샷을 만듭니다.
         * @return 삭제 예정 액터를 제외한 핸들 목록입니다.
         */
        Core::TArray<CoreUObject::TObjectRef<Engine::AActor>> GetActorRefs() const;

        /**
         * @brief 프로퍼티 패널이 표시할 객체 핸들의 스냅샷을 만듭니다.
         * @return 액터 선택 시 액터와 컴포넌트 목록이며 개별 객체 선택 시 해당 객체입니다.
         */
        Core::TArray<CoreUObject::TObjectRef<CoreUObject::UObject>> GetDetailsObjects() const;

        /**
         * @brief 전역 핸들과 편집 월드 소속을 함께 검사합니다.
         * @param[in] Handle 조회할 객체 핸들입니다.
         * @return 해당 월드 객체의 비소유 주소이며 검증에 실패하면 nullptr입니다.
         */
        CoreUObject::UObject* ResolveObject(CoreUObject::TObjectRef<CoreUObject::UObject> Handle) const;

        /**
         * @brief 유효한 액터를 선택하고 액터를 편집 대상으로 지정합니다.
         * @param[in] Handle 같은 편집 월드의 액터 핸들입니다.
         * @return 선택을 수락하면 true이며 잘못된 요청은 기존 선택을 유지합니다.
         */
        bool SelectActor(CoreUObject::TObjectRef<Engine::AActor> Handle);

        /**
         * @brief 객체를 선택하고 컴포넌트의 경우 소유 액터도 함께 선택합니다.
         * @param[in] Handle 같은 편집 월드의 객체 핸들입니다.
         * @return 선택을 수락하면 true입니다.
         */
        bool SelectObject(CoreUObject::TObjectRef<CoreUObject::UObject> Handle);

        /**
         * @brief 액터와 객체 선택을 해제합니다.
         */
        void ClearSelection();

        /**
         * @brief 선택한 액터를 검증하여 조회합니다.
         * @return 살아 있는 선택 액터의 비소유 주소이며 선택이 없으면 nullptr입니다.
         */
        Engine::AActor* GetSelectedActor() const;

        /**
         * @brief 아직 살아 있는 선택 액터들의 세대 핸들을 수집합니다.
         * @return 대상 객체들의 세대 핸들을 담은 배열입니다.
         */
        Core::TArray<CoreUObject::TObjectRef<Engine::AActor>> GetSelectedActors() const;

        /**
         * @brief 선택 컴포넌트 또는 선택 액터들의 루트 변환 대상을 수집합니다.
         * @return 대상 객체들의 세대 핸들을 담은 배열입니다.
         */
        Core::TArray<CoreUObject::TObjectRef<Engine::USceneComponent>> GetTransformTargets() const;

        /**
         * @brief 유효한 액터 핸들들로 현재 다중 선택을 교체합니다.
         * @param[in] Actors 선택하거나 복제할 액터들의 세대 핸들입니다.
         * @return 조건을 만족하거나 변경을 적용하면 true입니다.
         */
        bool SetSelectedActors(const Core::TArray<CoreUObject::TObjectRef<Engine::AActor>>& Actors);

        /**
         * @brief 지정 액터의 다중 선택 포함 상태를 반전합니다.
         * @param[in] Actor 선택 상태를 반전할 액터의 핸들입니다.
         * @return 조건을 만족하거나 변경을 적용하면 true입니다.
         */
        bool ToggleActorSelection(CoreUObject::TObjectRef<Engine::AActor> Actor);

        /**
         * @brief 선택 대상 변경을 추적할 개정 번호를 반환합니다.
         * @return 단조 증가하는 선택 개정 번호입니다.
         */
        Core::Uint64 GetSelectionRevision() const;

        /**
         * @brief 현재 선택 액터들을 새 GUID로 복제하고 복제본들을 선택합니다.
         * @return 대상 객체들의 세대 핸들을 담은 배열입니다.
         */
        Core::TArray<CoreUObject::TObjectRef<Engine::AActor>> DuplicateSelectedActors();

        /**
         * @brief 현재 프로퍼티 편집 대상을 조회합니다.
         * @return 선택 객체의 비소유 주소이며 선택이 없으면 nullptr입니다.
         */
        CoreUObject::UObject* GetSelectedObject() const;

        /**
         * @brief 액터를 생성하고 배치한 뒤 선택과 장면 변경 상태를 갱신합니다.
         * @param[in] Type 생성할 기본 메시 종류입니다.
         * @return 생성한 액터의 비소유 주소이며 실패하면 nullptr입니다.
         */
        Engine::AStaticMeshActor* SpawnPrimitive(Engine::EPrimitiveType Type);

        /**
         * @brief 지정한 프리셋의 액터를 생성·배치하고 선택과 장면 변경 상태를 갱신합니다.
         * @param[in] Preset 생성하거나 표시할 액터 프리셋의 식별자입니다.
         * @return 월드가 소유한 새 액터의 비소유 주소이며 실패하면 nullptr입니다.
         */
        Engine::AActor* SpawnActorPreset(const Core::FString& Preset);

        /**
         * @brief 지정한 프리셋의 액터를 생성·배치하고 선택과 장면 변경 상태를 갱신합니다.
         * @param[in] Preset 생성하거나 표시할 액터 프리셋의 식별자입니다.
         * @param[in] Placement 새 액터에 적용할 월드 변환입니다.
         * @return 월드가 소유한 새 액터의 비소유 주소이며 실패하면 nullptr입니다.
         */
        Engine::AActor* SpawnActorPreset(const Core::FString& Preset, const Core::FTransform& Placement);

        /**
         * @brief 편집 월드에 기본 카메라 액터를 생성합니다.
         * @return 생성한 객체의 비소유 주소이며, 생성에 실패하면 nullptr입니다.
         */
        Engine::ACameraActor* SpawnCamera();

        /**
         * @brief 배치 위치에 빌보드 텍스트 액터를 생성하고 선택합니다.
         * @return 생성한 월드 소유 액터이며 실패하면 nullptr입니다.
         */
        Engine::AActor* SpawnText();

        /**
         * @brief 배치 위치에 텍스처 빌보드 액터를 생성하고 선택과 장면 변경 상태를 갱신합니다.
         * @param[in] SpritePath Sprite에 할당할 Content 상대 텍스처 경로이며 빈 문자열이면 기본 아틀라스를 사용합니다.
         * @return 생성한 월드 소유 액터의 비소유 주소이며 생성, 텍스처 할당 또는 배치에 실패하면 nullptr입니다.
         */
        Engine::AActor* SpawnBillboard(const Core::FString& SpritePath = {});

        /**
         * @brief 현재 선택 액터와 편집 상태에서 해당 컴포넌트를 생성할 수 있는지 검사합니다.
         * @param[in] Class 생성하거나 이름을 표시할 리플렉션 클래스입니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool CanAddComponent(const CoreUObject::UClass* Class) const;

        /**
         * @brief 선택한 액터에 리플렉션 클래스로 컴포넌트를 생성하여 등록합니다.
         * @param[in] Class 생성하거나 이름을 표시할 리플렉션 클래스입니다.
         * @return 생성한 객체의 비소유 주소이며, 생성에 실패하면 nullptr입니다.
         */
        Engine::UActorComponent* AddComponent(const CoreUObject::UClass* Class);

        /**
         * @brief 현재 선택에서 사용할 수 있는 카메라 컴포넌트를 조회합니다.
         * @return 대상의 비소유 주소이며, 없거나 유효하지 않으면 nullptr입니다.
         */
        Engine::UCameraComponent* GetSelectedCamera() const;

        /**
         * @brief 선택한 카메라를 월드의 활성 카메라로 지정합니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool UseSelectedCamera();

        /**
         * @brief 선택한 액터들을 삭제하고 관련 선택을 해제합니다.
         * @return 삭제 요청을 수락하면 true입니다.
         */
        bool DestroySelectedActor();

        /**
         * @brief 대상 액터들의 가시성을 변경하고 실행 취소 이력에 반영합니다.
         * @param Actors 가시성을 변경할 액터 핸들 목록입니다.
         * @param Visible 설정할 액터 가시성입니다.
         * @return 액터의 가시성을 변경했으면 true입니다.
         */
        bool SetActorsVisible(const Core::TArray<CoreUObject::TObjectRef<Engine::AActor>>& Actors, bool Visible);

        /**
         * @brief 객체 소속과 프로퍼티 편집 권한을 검사합니다.
         * @param[in] Handle 편집할 객체 핸들입니다.
         * @param[in] Property 객체 클래스의 프로퍼티입니다.
         * @return 현재 편집할 수 있으면 true입니다.
         */
        bool CanEditProperty(CoreUObject::TObjectRef<CoreUObject::UObject> Handle, const CoreUObject::FProperty* Property) const;

        /**
         * @brief 공통 검증과 변경 전후 알림을 거쳐 프로퍼티를 편집합니다.
         * @param[in] Handle 편집할 객체 핸들입니다.
         * @param[in] Property 객체 클래스의 프로퍼티입니다.
         * @param[in] Value 적용할 값입니다.
         * @param[in] Interactive 드래그 중 연속 변경인지 지정합니다.
         * @return 값 쓰기에 성공하면 true입니다.
         * @note 콜백 중 삭제와 재진입을 검사하며 성공한 변경은 장면을 수정 상태로 만듭니다.
         */
        bool EditProperty(CoreUObject::TObjectRef<CoreUObject::UObject> Handle, const CoreUObject::FProperty* Property, const CoreUObject::FPropertyValue& Value, bool Interactive = false);

        /**
         * @brief 액터 루트 또는 공간 컴포넌트의 변환을 공통 프로퍼티 편집 경로로 변경합니다.
         * @param[in] Handle 변환할 액터 또는 공간 컴포넌트의 핸들입니다.
         * @param[in] Transform 적용할 변환입니다.
         * @param[in] Interactive 연속 편집 여부입니다.
         * @return 변환을 적용하면 true입니다.
         */
        bool ApplyTransform(CoreUObject::TObjectRef<CoreUObject::UObject> Handle, const Core::FTransform& Transform, bool Interactive = false);

        /**
         * @brief 편집 월드를 장면 파일로 저장합니다.
         * @param[in] Path 절대 경로 또는 Scenes 아래 상대 경로입니다.
         * @return 저장에 성공하면 true입니다.
         */
        bool SaveScene(const std::filesystem::path& Path);

        /**
         * @brief 장면을 새 월드에 읽고 편집 대상 월드를 교체합니다.
         * @param[in] Path 절대 경로 또는 Scenes 아래 상대 경로입니다.
         * @return 교체에 성공하면 true입니다.
         * @note 엔진의 현재 대상이 다른 실행 월드이면 해당 월드와 현재 선택을 유지합니다.
         */
        bool LoadScene(const std::filesystem::path& Path);

        /**
         * @brief 빈 편집 월드로 교체하고 선택과 장면 파일 상태를 초기화합니다.
         * @return 교체에 성공하면 true입니다.
         */
        bool NewScene();

        /**
         * @brief 현재 장면 파일 경로를 반환합니다.
         * @return 컨텍스트가 유지하는 경로의 참조입니다.
         */
        const std::filesystem::path& GetScenePath() const;

        /**
         * @brief 마지막 저장 이후 변경 여부를 반환합니다.
         * @return 저장하지 않은 변경이 있으면 true입니다.
         */
        bool IsSceneDirty() const;

        /**
         * @brief 프로퍼티 편집을 거치지 않은 장면 변경을 수정 상태로 표시합니다.
         */
        void MarkSceneDirty();

        /**
         * @brief 시퀀서 프리뷰가 월드에 쓴 값을 되돌리는 콜백을 설정합니다.
         * @param[in] Restorer 저장, 장면 교체와 PIE 시작 전에 호출할 콜백이며 비어 있으면 해제합니다.
         */
        void SetPreviewRestorer(std::function<void()> Restorer);

        /**
         * @brief 시퀀서 프리뷰가 월드에 쓴 값을 되돌립니다.
         */
        void RestorePreview();

        /**
         * @brief 시퀀스 액터를 시퀀서에서 여는 콜백을 설정합니다.
         * @param[in] Opener 시퀀스 액터의 핸들을 받아 패널을 여는 콜백이며 비어 있으면 해제합니다.
         */
        void SetSequencerOpener(std::function<void(CoreUObject::TObjectRef<Engine::ALevelSequenceActor>)> Opener);

        /**
         * @brief 객체가 시퀀스 액터이면 시퀀서에서 엽니다.
         * @param[in] Handle 열 객체의 핸들입니다.
         * @return 시퀀서를 열었으면 true이며 시퀀스 액터가 아니거나 콜백이 없으면 false입니다.
         */
        bool OpenInSequencer(CoreUObject::TObjectRef<Engine::ALevelSequenceActor> Handle);

        /**
         * @brief 원근 편집 뷰포트가 대신 사용할 시퀀서 프리뷰 카메라를 설정합니다.
         * @param[in] Camera 사용할 카메라 컴포넌트의 핸들이며 비어 있으면 편집 카메라로 돌아갑니다.
         */
        void SetPreviewCamera(CoreUObject::TObjectRef<Engine::UCameraComponent> Camera);

        /**
         * @brief 시퀀서 프리뷰 카메라를 조회합니다.
         * @return 살아 있는 카메라의 비소유 주소이며 없으면 nullptr입니다.
         */
        Engine::UCameraComponent* GetPreviewCamera() const;

        /**
         * @brief 현재 연결된 프로젝트의 비소유 주소를 반환합니다.
         * @return 대상의 비소유 주소이며, 없거나 유효하지 않으면 nullptr입니다.
         */
        Engine::FProject* GetProject() const;

        /**
         * @brief 배치 제공자가 반환한 위치에 메시 에셋 액터를 생성합니다.
         * @param[in] Path 프로젝트 메시 에셋 경로입니다.
         * @return 생성한 월드 소유 액터이며 실패하면 nullptr입니다.
         */
        Engine::AStaticMeshActor* SpawnMesh(const std::filesystem::path& Path);

        /**
         * @brief 지정한 변환에 메시 에셋 액터를 생성하고 선택합니다.
         * @param[in] Path 프로젝트 메시 에셋 경로입니다.
         * @param[in] Placement 생성할 액터의 월드 변환입니다.
         * @return 생성한 월드 소유 액터이며 실패하면 nullptr입니다.
         */
        Engine::AStaticMeshActor* SpawnMesh(const std::filesystem::path& Path, const Core::FTransform& Placement);

        /**
         * @brief 지정 위치에 스켈레탈 메시 액터를 배치하고 선택합니다.
         * @param Path 스켈레탈 메시 에셋 경로입니다.
         * @param Placement 월드 변환입니다.
         * @return 생성한 액터이며 실패하면 nullptr입니다.
         */
        Engine::ASkeletalMeshActor* SpawnSkeletalMesh(const std::filesystem::path& Path, const Core::FTransform& Placement);

        /**
         * @brief 애플리케이션에 프로젝트 전환을 요청할 콜백을 설정합니다.
         * @param[in] Handler 프로젝트 열기·생성 요청을 처리할 콜백입니다.
         */
        void SetProjectRequestHandler(std::function<void(const std::filesystem::path&, bool)> Handler);

        /**
         * @brief 등록된 처리기에 프로젝트 생성 또는 열기 요청을 전달합니다.
         * @param[in] Path 열거나 생성할 프로젝트 경로입니다.
         * @param[in] Create true이면 새 프로젝트를 생성하고 false이면 기존 프로젝트를 엽니다.
         */
        void RequestProject(const std::filesystem::path& Path, bool Create);

        /**
         * @brief UI와 독립적인 콘솔 로그를 추가합니다.
         * @param[in] Message 추가할 메시지입니다.
         */
        void AddLog(const Core::FString& Message);

        /**
         * @brief 수집한 로그를 지우고 로그 세대를 갱신합니다.
         */
        void ClearLogs();

        /**
         * @brief 표시할 로그 목록을 반환합니다.
         * @return 컨텍스트가 소유한 메시지 목록의 참조입니다.
         */
        const Core::TArray<Core::FString>& GetLogs() const;

        /**
         * @brief 로그 목록 변경 세대를 반환합니다.
         * @return 추가 또는 초기화마다 증가하는 값입니다.
         */
        Core::Uint64 GetLogRevision() const;

        /**
         * @brief 콘솔 문자열을 공통 편집 동작으로 전달합니다.
         * @param[in] Command 실행할 문자열입니다.
         */
        void ExecuteConsoleCommand(const Core::FString& Command);

        /**
         * @brief 콘솔 명령에서 입력 접두어에 맞는 후보를 조회합니다.
         * @param[in] Input 해석할 원본 입력 문자열입니다.
         * @return 입력에 맞는 자동 완성 목록입니다.
         */
        Core::TArray<Core::FString> GetConsoleSuggestions(std::string_view Input) const;

        /**
         * @brief 편집 컨텍스트가 소유하는 통계 그래프 창을 반환합니다.
         * @return 편집 컨텍스트가 소유하는 통계 그래프 창입니다.
         */
        Editor::FStatGraphs& GetStatGraphs();

        /**
         * @brief 그림자 아틀라스 검사 창의 표시 상태를 반환합니다.
         * @return 검사 창이 열려 있으면 true입니다.
         */
        bool IsShadowAtlasOpen() const;

        /**
         * @brief 그림자 아틀라스 검사 창의 표시 상태를 변경합니다.
         * @param[in] Open 창을 열지 지정하며 그리기 함수에서는 닫기 동작도 반영합니다.
         */
        void SetShadowAtlasOpen(bool Open);

        /**
         * @brief 콘솔 명령이 사용할 뷰포트 제공자를 설정합니다.
         * @param[in] Provider 명령을 실행할 때 활성 뷰포트를 반환합니다.
         */
        void SetConsoleViewportProvider(std::function<Editor::FEditorViewport*()> Provider);

        /**
         * @brief 새 액터의 배치 변환을 제공하는 경로를 연결합니다.
         * @param[in] Provider 뷰포트 카메라 등에 따른 배치 변환 제공자이며 비어 있으면 기본 변환을 사용합니다.
         */
        void SetPlacementProvider(std::function<Core::FTransform()> Provider);

        /**
         * @brief 장면 저장·복원에 사용할 원근 카메라 콜백을 연결합니다.
         * @param Provider 현재 원근 카메라를 반환합니다.
         * @param Restorer 읽은 카메라를 적용합니다.
         */
        void SetCameraHandlers(std::function<Engine::FEditorCameraState()> Provider, std::function<void(const Engine::FEditorCameraState&)> Restorer);

        /**
         * @brief 슬롯 재질을 변경하고 장면을 수정 상태로 표시합니다.
         * @param Handle 대상 정적 메시 컴포넌트입니다.
         * @param Slot 재질 슬롯 번호입니다.
         * @param Path Content 기준 재질 경로입니다.
         * @return 변경을 적용하면 true입니다.
         */
        bool SetMaterialOverride(CoreUObject::TObjectRef<CoreUObject::UObject> Handle, Core::Index_t Slot, const Core::FString& Path);

        /**
         * @brief C# 스크립트 필드 값을 저장하고 장면을 수정 상태로 표시합니다.
         * @param Handle 대상 스크립트 컴포넌트입니다.
         * @param Field 저장할 필드입니다.
         * @return 변경을 적용하면 true입니다.
         */
        bool SetScriptField(CoreUObject::TObjectRef<CoreUObject::UObject> Handle, const Script::FScriptField& Field);

        /**
         * @brief 저장된 C# 스크립트 필드를 지우고 장면을 수정 상태로 표시합니다.
         * @param Handle 대상 스크립트 컴포넌트입니다.
         * @param Name C# 필드 이름입니다.
         * @return 변경을 적용하면 true입니다.
         */
        bool RemoveScriptField(CoreUObject::TObjectRef<CoreUObject::UObject> Handle, const Core::FString& Name);

    private:

        /**
         * @brief 편집 기능·렌더 설정·통계 명령을 콘솔에 등록합니다.
         */
        void RegisterConsoleCommands();

        /**
         * @brief 콘솔 인자로 화면 찢김 허용 설정을 조회하거나 변경합니다.
         * @param[in] Invocation 명령 이름·사용법·입력·인자입니다.
         */
        void ExecuteAllowTearing(const Editor::FConsoleInvocation& Invocation);

        /**
         * @brief PIE 세션이 편집 잠금과 원본·복제 조회 상태를 관리하도록 접근을 허용합니다.
         */
        friend class FPlayInEditorSession;

        /**
         * @brief 재진입을 차단하고 단일·다중 선택 대상을 공통 검증하여 적용합니다.
         * @param[in] Actors 순서와 중복 여부를 검증할 액터 선택 목록입니다.
         * @param[in] Object 편집 대상 핸들입니다.
         * @return 요청을 수락하면 true이며 잘못된 요청은 기존 선택을 유지합니다.
         */
        bool SetSelection(const Core::TArray<CoreUObject::TObjectRef<Engine::AActor>>& Actors, CoreUObject::TObjectRef<CoreUObject::UObject> Object);

        /**
         * @brief 검증한 선택 목록과 편집 대상을 저장하고 선택 변경 번호를 갱신합니다.
         * @param[in] Actors 마지막 항목을 대표 액터로 사용하는 선택 목록입니다.
         * @param[in] Object 프로퍼티 편집 대상입니다.
         */
        void AssignSelection(const Core::TArray<CoreUObject::TObjectRef<Engine::AActor>>& Actors, CoreUObject::TObjectRef<CoreUObject::UObject> Object);

        /**
         * @brief 현재 실행·전환 상태에서 에디터 장면을 수정할 수 있는지 확인합니다.
         * @return 장면 수정을 허용하는 상태이면 true입니다.
         */
        bool CanModifyScene() const;

        /**
         * @brief 새 액터에 배치 변환을 적용하고 실패하면 생성한 액터를 제거합니다.
         * @param[in,out] Actor 새로 생성한 월드 소유 액터입니다.
         * @param[in] Placement 적용할 월드 변환입니다.
         * @return 배치에 성공하면 true입니다.
         */
        bool ApplySpawnPlacement(Engine::AActor* Actor, const Core::FTransform& Placement);

        /**
         * @brief 생성한 액터를 선택하고 장면 변경 상태와 로그를 갱신합니다.
         * @param[in] Actor 배치와 개별 설정을 마친 액터입니다.
         * @param[in] Action 로그에 사용할 동작 접두사입니다.
         */
        void CompleteActorSpawn(Engine::AActor* Actor, const char* Action);

        /**
         * @brief 새 컨텍스트를 엔진에 등록하고 이전 편집 월드를 교체합니다.
         * @param[in] Context 엔진으로 소유권을 이전할 새 월드입니다.
         * @param[in] Path 새 장면 파일 경로입니다.
         * @return 교체에 성공하면 true입니다.
         */
        bool ReplaceScene(std::unique_ptr<Engine::FWorldContext> Context, const std::filesystem::path& Path);

        /**
         * @brief 새 월드 컨텍스트로 장면을 교체하고 에디터 참조를 갱신합니다.
         * @param Context 장면을 조회·복원·편집할 에디터 컨텍스트입니다.
         * @return 월드 교체에 성공했으면 true입니다.
         */
        bool ReplaceWorld(std::unique_ptr<Engine::FWorldContext> Context);

        /**
         * @brief 현재 장면과 선택 GUID를 복원용 스냅샷에 기록합니다.
         * @param State 준비·저장·복원할 상태 데이터입니다.
         * @return 스냅샷을 만들었으면 true입니다.
         */
        bool CaptureSceneMemento(FSceneMemento& State);

        /**
         * @brief 스냅샷에서 장면을 다시 만들고 GUID로 선택 상태를 복원합니다.
         * @param State 준비·저장·복원할 상태 데이터입니다.
         * @return 장면과 선택을 복원했으면 true입니다.
         */
        bool RestoreSceneMemento(const FSceneMemento& State);

        /**
         * @brief 진행 중인 편집에 변경을 표시하거나 장면 리비전을 갱신합니다.
         */
        void NoteSceneEdit();

        /**
         * @brief 실행 취소 이력과 진행 중인 장면 편집 상태를 초기화합니다.
         */
        void ClearSceneHistory();

        /**
         * @brief 실행 취소 이력의 보관 한도를 초과하는 오래된 명령을 제거합니다.
         */
        void TrimSceneHistory();

    private:

        /**
         * @brief 월드의 소유권과 렌더러를 제공하는 엔진의 비소유 주소입니다.
         */
        Engine::UEngine* mEngine{};

        /**
         * @brief PIE 실행 중 원본 편집 상태의 수정을 제한하는 플래그입니다.
         */
        bool mPlayLocked{};

        /**
         * @brief 엔진의 현재 실행 대상과 독립적인 편집 월드 핸들입니다.
         */
        Engine::FWorldHandle mWorld{};

        /**
         * @brief 프로퍼티 편집 대상의 세대 핸들입니다.
         */
        CoreUObject::TObjectRef<CoreUObject::UObject> mSelectedObject{};

        /**
         * @brief 현재 장면 파일 경로입니다.
         */
        std::filesystem::path mScenePath{};

        /**
         * @brief 실행 취소·다시 실행할 장면 편집 명령 목록입니다.
         */
        Core::TArray<FSceneEditCommand> mSceneHistory{};

        /**
         * @brief 진행 중인 장면 편집의 변경 전 스냅샷입니다.
         */
        FSceneMemento mPendingSceneEdit{};

        /**
         * @brief 다음 다시 실행 명령의 위치이며 앞쪽 명령은 적용된 상태입니다.
         */
        Core::Index_t mSceneHistoryPosition{};

        /**
         * @brief 편집 토큰 발급용 단조 증가 번호입니다.
         */
        Core::Uint64 mSceneEditSerial{};

        /**
         * @brief 현재 장면 편집 범위를 종료할 토큰이며 비활성 시 0입니다.
         */
        Core::Uint64 mSceneEditToken{};

        /**
         * @brief 현재 장면 내용의 리비전입니다.
         */
        Core::Uint64 mSceneRevision{};

        /**
         * @brief 마지막 저장 시점의 장면 리비전입니다.
         */
        Core::Uint64 mSavedSceneRevision{};

        /**
         * @brief 진행 중인 편집에서 장면 변경이 보고되었는지 여부입니다.
         */
        bool mSceneEditChanged{};

        /**
         * @brief 선택과 월드 변경의 재진입 제한입니다.
         */
        bool mChangingState{};

        /**
         * @brief 프로퍼티 변경 알림의 재진입 제한입니다.
         */
        bool mEditingProperty{};

        /**
         * @brief UI 수명과 독립적으로 유지할 콘솔 로그입니다.
         */
        Core::TArray<Core::FString> mLogs{};

        /**
         * @brief 로그 추가와 초기화를 구분하는 변경 세대입니다.
         */
        Core::Uint64 mLogRevision{};

        /**
         * @brief 활성 뷰포트 등에 따른 새 액터의 배치 변환 제공자입니다.
         */
        std::function<Core::FTransform()> mPlacementProvider{};

        /**
         * @brief 편집·렌더·통계 명령 레지스트리입니다.
         */
        Editor::FConsoleCommandRegistry mConsoleCommands{};

        /**
         * @brief 콘솔·통계 패널이 공유하는 그래프 창입니다.
         */
        Editor::FStatGraphs mStatGraphs{};

        /**
         * @brief 콘솔 명령 대상 뷰포트를 찾는 콜백입니다.
         */
        std::function<Editor::FEditorViewport*()> mConsoleViewportProvider{};

        /**
         * @brief 그림자 아틀라스 검사 창의 표시 상태입니다.
         */
        bool mShadowAtlasOpen{};

        /**
         * @brief 마지막 항목을 주 대상으로 사용하는 편집 월드의 액터 선택 집합입니다.
         */
        Core::TArray<CoreUObject::TObjectRef<Engine::AActor>> mSelectedActors{};

        /**
         * @brief 선택 변경을 감지하거나 드래그 시작 시 선택을 검증하는 세대 값입니다.
         */
        Core::Uint64 mSelectionRevision{};

        /**
         * @brief 프로젝트 생성 또는 열기를 애플리케이션에 전달하는 콜백입니다.
         */
        std::function<void(const std::filesystem::path&, bool)> mProjectRequestHandler{};

        /**
         * @brief 저장할 원근 편집 카메라의 조회 함수입니다.
         */
        std::function<Engine::FEditorCameraState()> mCameraProvider{};

        /**
         * @brief 장면 교체 후 원근 카메라를 적용하는 함수입니다.
         */
        std::function<void(const Engine::FEditorCameraState&)> mCameraRestorer{};

        /**
         * @brief 시퀀서 프리뷰가 월드에 쓴 값을 되돌리는 함수입니다.
         */
        std::function<void()> mPreviewRestorer{};

        /**
         * @brief 시퀀스 액터를 시퀀서에서 여는 함수입니다.
         */
        std::function<void(CoreUObject::TObjectRef<Engine::ALevelSequenceActor>)> mSequencerOpener{};

        /**
         * @brief 원근 편집 뷰포트가 대신 사용할 시퀀서 프리뷰 카메라의 핸들입니다.
         */
        CoreUObject::TObjectRef<Engine::UCameraComponent> mPreviewCamera{};
    };

}
