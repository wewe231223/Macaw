/**
 * @file FEditorContext.h
 * @brief UI와 독립적인 편집 대상, 선택, 장면 상태와 공통 편집 경로를 정의합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/FObjectHandle.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include "Engine/FWorldHandle.h"

#include <filesystem>
#include <functional>

class UEngine;
class FProject;
class UWorld;
class UObject;
class AActor;
class AStaticMeshActor;
class ACameraActor;
class UCameraComponent;
class UActorComponent;
class UClass;
class FProperty;
class FWorldContext;
class IRendererModule;
enum class EPrimitiveType : Uint8;

/**
 * @brief 명시적인 편집 월드와 선택을 소유하고 패널, 콘솔과 기즈모의 편집 요청을 처리합니다.
 * @ingroup Editor
 * @note 엔진과 객체를 소유하지 않으며 ImGui, 패널과 창에 의존하지 않습니다. 실행 중인 월드 선택과 편집 대상 월드는 별개입니다.
 */
class FEditorContext {
public:
    /** @brief 리플렉션에서 사용할 소유 타입 별칭입니다. */
    ReflectTypes(FEditorContext);

private:
    /** @brief 재진입을 제한하는 연산 플래그의 수명을 관리합니다. */
    class FOperationScope {
    public:
        /** @brief 리플렉션에서 사용할 소유 타입 별칭입니다. */
        ReflectTypes(FOperationScope);

    public:
        /** @brief 연산 플래그를 설정합니다. @param[in,out] Flag 범위 동안 설정할 플래그입니다. */
        explicit FOperationScope(bool* Flag);
        /** @brief 연산 플래그를 해제합니다. */
        ~FOperationScope();
        /** @brief 범위의 중복 해제를 막기 위해 복사를 금지합니다. */
        FOperationScope(const FOperationScope&) = delete;
        /** @brief 범위 대입을 금지합니다. @return 대입 대상의 참조입니다. */
        FOperationScope& operator=(const FOperationScope&) = delete;

    public:
        /** @brief 정적 구조체 메타데이터를 반환합니다. @return 메타데이터의 비소유 주소입니다. */
        ReflectStruct(FOperationScope);

    private:
        /** @brief 생성된 멤버 등록 함수의 접근 권한입니다. */
        ReflectMembers();

    private:
        /** @brief 이 범위가 관리하는 플래그의 비소유 주소입니다. */
        bool* mFlag{};
        ReflectProperty(mFlag, "Flag", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    };

public:
    /** @brief 편집에 사용할 엔진을 연결합니다. @param[in,out] Engine 컨텍스트보다 오래 유지할 엔진입니다. */
    explicit FEditorContext(UEngine* Engine);
    /** @brief 비소유 연결과 편집 상태의 수명을 끝냅니다. */
    ~FEditorContext();
    /** @brief 편집 상태와 콜백 연결의 복사를 금지합니다. */
    FEditorContext(const FEditorContext&) = delete;
    /** @brief 편집 상태의 대입을 금지합니다. @return 대입 대상의 참조입니다. */
    FEditorContext& operator=(const FEditorContext&) = delete;

public:
    /** @brief 정적 구조체 메타데이터를 반환합니다. @return 메타데이터의 비소유 주소입니다. */
    ReflectStruct(FEditorContext);

    /** @brief 편집 대상을 연결하고 선택과 장면 상태를 초기화합니다. @param[in] Handle 같은 엔진이 소유한 월드입니다. @return 연결에 성공하면 true입니다. */
    bool SetWorld(FWorldHandle Handle);
    /** @brief 편집 월드 연결, 선택과 장면 상태를 초기화합니다. */
    void Reset();
    /** @brief 편집 대상 월드 핸들을 반환합니다. @return 현재 편집 월드의 세대 핸들입니다. */
    FWorldHandle GetWorldHandle() const;
    /** @brief 핸들을 검증하여 편집 월드를 조회합니다. @return 편집 월드의 비소유 주소이며 만료되면 nullptr입니다. */
    UWorld* GetWorld() const;
    /** @brief 편집 월드의 수정 허용 여부를 검사합니다. @return 정지한 Editor 또는 EditorPreview 월드이면 true입니다. */
    bool CanEditWorld() const;
    /**
     * @brief 편집 상태 변경을 처리 중이어서 재진입이 제한되는지 반환합니다.
     * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
     */
    bool IsChangingState() const;
    /** @brief 엔진의 렌더러를 조회합니다. @return 렌더러의 비소유 주소이며 헤드리스 환경에서는 nullptr입니다. */
    IRendererModule* GetRenderer() const;
    /** @brief 편집 월드에서 살아 있는 액터 핸들의 스냅샷을 만듭니다. @return 삭제 예정 액터를 제외한 핸들 목록입니다. */
    TArray<FObjectHandle> GetActorHandles() const;
    /** @brief 프로퍼티 패널이 표시할 객체 핸들의 스냅샷을 만듭니다. @return 액터 선택 시 액터와 컴포넌트 목록이며 개별 객체 선택 시 해당 객체입니다. */
    TArray<FObjectHandle> GetDetailsObjects() const;
    /** @brief 전역 핸들과 편집 월드 소속을 함께 검사합니다. @param[in] Handle 조회할 객체 핸들입니다. @return 해당 월드 객체의 비소유 주소이며 검증에 실패하면 nullptr입니다. */
    UObject* ResolveObject(FObjectHandle Handle) const;

    /** @brief 유효한 액터를 선택하고 액터를 편집 대상으로 지정합니다. @param[in] Handle 같은 편집 월드의 액터 핸들입니다. @return 선택을 수락하면 true이며 잘못된 요청은 기존 선택을 유지합니다. */
    bool SelectActor(FObjectHandle Handle);
    /** @brief 객체를 선택하고 컴포넌트의 경우 소유 액터도 함께 선택합니다. @param[in] Handle 같은 편집 월드의 객체 핸들입니다. @return 선택을 수락하면 true입니다. */
    bool SelectObject(FObjectHandle Handle);
    /** @brief 액터와 객체 선택을 해제합니다. */
    void ClearSelection();
    /** @brief 선택한 액터를 검증하여 조회합니다. @return 살아 있는 선택 액터의 비소유 주소이며 선택이 없으면 nullptr입니다. */
    AActor* GetSelectedActor() const;
    TArray<FObjectHandle> GetSelectedActors() const;
    TArray<FObjectHandle> GetTransformTargets() const;
    bool SetSelectedActors(const TArray<FObjectHandle>& Actors);
    bool ToggleActorSelection(FObjectHandle Actor);
    Uint64 GetSelectionRevision() const;
    TArray<FObjectHandle> DuplicateSelectedActors();

    /** @brief 현재 프로퍼티 편집 대상을 조회합니다. @return 선택 객체의 비소유 주소이며 선택이 없으면 nullptr입니다. */
    UObject* GetSelectedObject() const;

    /** @brief 액터를 생성하고 배치한 뒤 선택과 장면 변경 상태를 갱신합니다. @param[in] Type 생성할 기본 메시 종류입니다. @return 생성한 액터의 비소유 주소이며 실패하면 nullptr입니다. */
    AStaticMeshActor* SpawnPrimitive(EPrimitiveType Type);
    /**
     * @brief 편집 월드에 기본 카메라 액터를 생성합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
     */
    ACameraActor* SpawnCamera();
    AActor* SpawnText();
    bool CanAddComponent(const UClass* Class) const;
    UActorComponent* AddComponent(const UClass* Class);
    /**
     * @brief 현재 선택에서 사용할 수 있는 카메라 컴포넌트를 조회합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
     */
    UCameraComponent* GetSelectedCamera() const;
    /**
     * @brief 선택한 카메라를 월드의 활성 카메라로 지정합니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool UseSelectedCamera();
    /** @brief 선택한 액터들을 삭제하고 관련 선택을 해제합니다. @return 삭제 요청을 수락하면 true입니다. */
    bool DestroySelectedActor();
    /** @brief 객체 소속과 프로퍼티 편집 권한을 검사합니다. @param[in] Handle 편집할 객체 핸들입니다. @param[in] Property 객체 클래스의 프로퍼티입니다. @return 현재 편집할 수 있으면 true입니다. */
    bool CanEditProperty(FObjectHandle Handle, const FProperty* Property) const;
    /** @brief 공통 검증과 변경 전후 알림을 거쳐 프로퍼티를 편집합니다. @param[in] Handle 편집할 객체 핸들입니다. @param[in] Property 객체 클래스의 프로퍼티입니다. @param[in] Value 적용할 값입니다. @param[in] Interactive 드래그 중 연속 변경인지 지정합니다. @return 값 쓰기에 성공하면 true입니다. @note 콜백 중 삭제와 재진입을 검사하며 성공한 변경은 장면을 수정 상태로 만듭니다. */
    bool EditProperty(FObjectHandle Handle, const FProperty* Property, const FPropertyValue& Value, bool Interactive = false);
    /** @brief 액터 루트 또는 공간 컴포넌트의 변환을 공통 프로퍼티 편집 경로로 변경합니다. @param[in] Handle 변환할 액터 또는 공간 컴포넌트의 핸들입니다. @param[in] Transform 적용할 변환입니다. @param[in] Interactive 연속 편집 여부입니다. @return 변환을 적용하면 true입니다. */
    bool ApplyTransform(FObjectHandle Handle, const FTransform& Transform, bool Interactive = false);

    /** @brief 편집 월드를 장면 파일로 저장합니다. @param[in] Path 절대 경로 또는 Scenes 아래 상대 경로입니다. @return 저장에 성공하면 true입니다. */
    bool SaveScene(const std::filesystem::path& Path);
    /** @brief 장면을 새 월드에 읽고 편집 대상 월드를 교체합니다. @param[in] Path 절대 경로 또는 Scenes 아래 상대 경로입니다. @return 교체에 성공하면 true입니다. @note 엔진의 현재 대상이 다른 실행 월드이면 해당 월드와 현재 선택을 유지합니다. */
    bool LoadScene(const std::filesystem::path& Path);
    /** @brief 빈 편집 월드로 교체하고 선택과 장면 파일 상태를 초기화합니다. @return 교체에 성공하면 true입니다. */
    bool NewScene();
    /** @brief 현재 장면 파일 경로를 반환합니다. @return 컨텍스트가 유지하는 경로의 참조입니다. */
    const std::filesystem::path& GetScenePath() const;
    /** @brief 마지막 저장 이후 변경 여부를 반환합니다. @return 저장하지 않은 변경이 있으면 true입니다. */
    bool IsSceneDirty() const;
    /**
     * @brief 현재 연결된 프로젝트의 비소유 주소를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
     */
    FProject* GetProject() const;
    /**
     * @brief 프로젝트의 메시 에셋을 사용하는 액터를 편집 월드에 생성합니다.
     * @param[in] Path 조회·로드·저장 또는 정규화할 파일 경로입니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
     */
    AStaticMeshActor* SpawnMesh(const std::filesystem::path& Path);
    AStaticMeshActor* SpawnMesh(const std::filesystem::path& Path, const FTransform& Placement);
    /**
     * @brief 애플리케이션에 프로젝트 전환을 요청할 콜백을 설정합니다.
     * @param[in] Handler 요청 또는 명령을 처리할 콜백입니다.
     */
    void SetProjectRequestHandler(std::function<void(const std::filesystem::path&, bool)> Handler);
    /**
     * @brief 등록된 처리기에 프로젝트 생성 또는 열기 요청을 전달합니다.
     * @param[in] Path 조회·로드·저장 또는 정규화할 파일 경로입니다.
     * @param[in] Create true이면 새 프로젝트를 생성하고 false이면 기존 프로젝트를 엽니다.
     */
    void RequestProject(const std::filesystem::path& Path, bool Create);

    /** @brief UI와 독립적인 콘솔 로그를 추가합니다. @param[in] Message 추가할 메시지입니다. */
    void AddLog(const FString& Message);
    /** @brief 수집한 로그를 지우고 로그 세대를 갱신합니다. */
    void ClearLogs();
    /** @brief 표시할 로그 목록을 반환합니다. @return 컨텍스트가 소유한 메시지 목록의 참조입니다. */
    const TArray<FString>& GetLogs() const;
    /** @brief 로그 목록 변경 세대를 반환합니다. @return 추가 또는 초기화마다 증가하는 값입니다. */
    Uint64 GetLogRevision() const;
    /** @brief 콘솔 문자열을 공통 편집 동작으로 전달합니다. @param[in] Command 실행할 문자열입니다. */
    void ExecuteConsoleCommand(const FString& Command);
    /**
     * @brief 뷰포트 관련 콘솔 명령을 처리할 콜백을 설정합니다.
     * @param[in] Handler 요청 또는 명령을 처리할 콜백입니다.
     */
    void SetViewportCommandHandler(std::function<bool(const FString&, FString&)> Handler);
    /** @brief 새 액터의 배치 변환을 제공하는 경로를 연결합니다. @param[in] Provider 뷰포트 카메라 등에 따른 배치 변환 제공자이며 비어 있으면 기본 변환을 사용합니다. */
    void SetPlacementProvider(std::function<FTransform()> Provider);

private:
    /**
     * @brief PIE 세션이 편집 잠금과 원본·복제 조회 상태를 관리하도록 접근을 허용합니다.
     */
    friend class FPlayInEditorSession;
    /** @brief 리플렉션 등록 함수의 접근 권한입니다. */
    ReflectMembers();
    /**
     * @brief 재진입을 차단하고 단일·다중 선택 대상을 공통 검증하여 적용합니다.
     * @param[in] Actors 순서와 중복 여부를 검증할 액터 선택 목록입니다.
     * @param[in] Object 편집 대상 핸들입니다.
     * @return 요청을 수락하면 true이며 잘못된 요청은 기존 선택을 유지합니다.
     */
    bool SetSelection(const TArray<FObjectHandle>& Actors, FObjectHandle Object);
    /**
     * @brief 검증한 선택 목록과 편집 대상을 저장하고 선택 변경 번호를 갱신합니다.
     * @param[in] Actors 마지막 항목을 대표 액터로 사용하는 선택 목록입니다.
     * @param[in] Object 프로퍼티 편집 대상입니다.
     */
    void AssignSelection(const TArray<FObjectHandle>& Actors, FObjectHandle Object);
    /** @brief 새 컨텍스트를 엔진에 등록하고 이전 편집 월드를 교체합니다. @param[in] Context 엔진으로 소유권을 이전할 새 월드입니다. @param[in] Path 새 장면 파일 경로입니다. @return 교체에 성공하면 true입니다. */
    bool ReplaceScene(std::unique_ptr<FWorldContext> Context, const std::filesystem::path& Path);

private:
    /** @brief 월드의 소유권과 렌더러를 제공하는 엔진의 비소유 주소입니다. */
    UEngine* mEngine{};
    ReflectProperty(mEngine, "Engine", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief PIE 실행 중 원본 편집 상태의 수정을 제한하는 플래그입니다.
     */
    bool mPlayLocked{};
    ReflectProperty(mPlayLocked, "PlayLocked", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 엔진의 현재 실행 대상과 독립적인 편집 월드 핸들입니다. */
    FWorldHandle mWorld{};
    ReflectProperty(mWorld, "World", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 프로퍼티 편집 대상의 세대 핸들입니다. */
    FObjectHandle mSelectedObject{};
    ReflectProperty(mSelectedObject, "SelectedObject", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 현재 장면 파일 경로입니다. */
    std::filesystem::path mScenePath{};
    ReflectProperty(mScenePath, "ScenePath", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 저장 이후 장면이 변경되었는지 여부입니다. */
    bool mDirty{};
    ReflectProperty(mDirty, "Dirty", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 선택과 월드 변경의 재진입 제한입니다. */
    bool mChangingState{};
    ReflectProperty(mChangingState, "ChangingState", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 프로퍼티 변경 알림의 재진입 제한입니다. */
    bool mEditingProperty{};
    ReflectProperty(mEditingProperty, "EditingProperty", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief UI 수명과 독립적으로 유지할 콘솔 로그입니다. */
    TArray<FString> mLogs{};
    ReflectProperty(mLogs, "Logs", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 로그 추가와 초기화를 구분하는 변경 세대입니다. */
    Uint64 mLogRevision{};
    ReflectProperty(mLogRevision, "LogRevision", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 활성 뷰포트 등에 따른 새 액터의 배치 변환 제공자입니다. */
    std::function<FTransform()> mPlacementProvider{};
    ReflectProperty(mPlacementProvider, "PlacementProvider", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 뷰포트 관련 명령의 처리 여부와 결과를 반환하는 콜백입니다.
     */
    std::function<bool(const FString&, FString&)> mViewportCommandHandler{};
    ReflectProperty(mViewportCommandHandler, "ViewportCommandHandler", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 선택한 액터의 세대 핸들 목록이며 마지막 유효 항목이 대표 액터입니다.
     */
    TArray<FObjectHandle> mSelectedActors{};
    ReflectProperty(mSelectedActors, "SelectedActors", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint64 mSelectionRevision{};
    ReflectProperty(mSelectionRevision, "SelectionRevision", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 프로젝트 생성 또는 열기를 애플리케이션에 전달하는 콜백입니다.
     */
    std::function<void(const std::filesystem::path&, bool)> mProjectRequestHandler{};
    ReflectProperty(mProjectRequestHandler, "ProjectRequestHandler", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
