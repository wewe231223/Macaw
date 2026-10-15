/**
 * @file UEditorEngine.h
 * @brief 에디터 엔진의 실행 수명과 GetEditorEngine 전역 접근점을 정의합니다.
 */
#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include <filesystem>
#include "Engine/UEngine.h"
#include "Editor/PIE/EPIEState.h"

class FWorldEditorContext;

/** @brief 에디터 월드, PIE 세션과 편집 컨텍스트를 관리하는 전역 에디터 엔진입니다. */
class UEditorEngine final : public UEngine {
public:
    /** @brief UEngine 기반 리플렉션 타입 별칭입니다. */
    ReflectTypes(UEditorEngine, UEngine);

public:
    /** @brief 전역으로 등록하지 않은 에디터 엔진 객체를 생성합니다. */
    UEditorEngine();
    /** @brief PIE 세션과 편집 컨텍스트를 종료하고 전역 에디터 접근점을 해제합니다. */
    ~UEditorEngine() override;

public:
    /** @brief 에디터 엔진 클래스의 리플렉션 정보를 반환합니다. */
    ReflectClass(UEditorEngine);

    /** @brief 엔진과 편집 컨텍스트를 초기화하고 전역 설정을 기본값으로 재설정한 뒤 파일에서 읽어 GetEditorEngine 접근점을 공개합니다. @note 설정 읽기 실패 시 기본값을 사용하며 이미 초기화된 경우 설정을 다시 읽지 않습니다. */
    void Initialize() override;
    /** @brief PIE 종료 콜백까지 전역 접근을 유지한 뒤 편집 컨텍스트와 엔진을 해제합니다. @note 월드 콜백 또는 PIE 처리 중 요청은 거절하며 전역 접근점을 유지합니다. */
    void Shutdown() override;
    /** @brief PIE 요청과 월드 Tick을 안전한 프레임 경계에서 처리합니다. @param DeltaTime 프레임 경과 시간(초)입니다. */
    void Tick(float DeltaTime) override;

    /** @brief 다음 프레임 경계에서 PIE 시작을 요청합니다. @return 요청이 수락되면 true입니다. */
    bool RequestStartPIE();
    /** @brief PIE 종료를 요청합니다. @return 요청이 수락되면 true입니다. */
    bool RequestStopPIE();
    /** @brief 실행 중인 PIE의 일시 정지를 요청합니다. @return 요청이 수락되면 true입니다. */
    bool RequestPausePIE();
    /** @brief 일시 정지된 PIE의 재개를 요청합니다. @return 요청이 수락되면 true입니다. */
    bool RequestResumePIE();
    /** @brief 현재 PIE 세션 상태를 반환합니다. */
    EPIEState GetPIEState() const;
    /** @brief 시작과 종료 대기를 포함해 PIE 세션이 존재하는지 반환합니다. */
    bool HasPIESession() const;
    /** @brief 현재 PIE 월드 컨텍스트를 반환하며 생성 전과 종료 후에는 nullptr입니다. */
    FWorldContext* GetPIEWorldContext() const;
    /** @brief 전용 수명으로 관리하는 에디터 및 PIE 월드를 제외한 컨텍스트를 제거합니다. @param Context 제거할 컨텍스트입니다. @return 제거에 성공하면 true입니다. */
    bool DestroyWorldContext(FWorldContext& Context) override;

    /** @brief 에디터 월드 컨텍스트를 반환합니다. @pre 에디터 엔진이 초기화되어 있어야 합니다. */
    FWorldContext& GetEditorWorldContext();
    /** @brief 선택 상태와 편집 요청을 관리하는 에디터 컨텍스트를 반환합니다. @pre 에디터 엔진이 초기화되어 있어야 합니다. */
    FWorldEditorContext& GetEditorContext();

    /** @brief 전역 설정의 마지막 씬 또는 기본 씬을 로드합니다. @return 로드에 성공하면 true입니다. */
    bool LoadStartupScene();
    /** @brief 씬을 로드하고 마지막 씬 경로를 전역 설정에 저장합니다. @param ScenePath 읽을 씬 경로입니다. @return 로드에 성공하면 true입니다. */
    bool LoadScene(const std::filesystem::path& ScenePath);
    /** @brief 에디터 월드를 지정한 이름의 씬으로 저장합니다. @param SceneName 디렉터리를 포함하지 않는 씬 이름입니다. @return 저장에 성공하면 true입니다. */
    bool SaveScene(const FString& SceneName);
    /**
     * @brief 전역 편집 설정에 현재 씬 경로를 반영하고 설정 파일에 저장합니다.
     * @return 편집 컨텍스트가 존재하고 파일 저장까지 성공하면 true입니다. 파일 저장 실패 시 메모리 설정은 유지합니다.
     */
    bool SaveEditorSettings();
    /** @brief 현재 기억하는 씬 파일 경로를 반환합니다. */
    const std::filesystem::path& GetScenePath() const;

private:
    /** @brief 대기 중인 PIE 시작, 종료와 일시 정지 상태를 적용합니다. */
    void ProcessPIERequests();
    /** @brief 에디터 월드를 복제해 PIE 컨텍스트를 만들고 BeginPlay를 호출합니다. */
    void StartPIE();
    /** @brief PIE의 EndPlay와 월드 해제를 수행합니다. */
    void StopPIE();
    /** @brief PIE 컨텍스트 참조와 세션 상태를 초기화합니다. */
    void ResetPIEState();
    /** @brief 편집 상태를 검사하고 씬을 로드합니다. @param ScenePath 읽을 씬 경로입니다. @param BRememberScene 성공한 경로를 설정에 저장할지 지정합니다. @return 로드에 성공하면 true입니다. */
    bool LoadSceneInternal(const std::filesystem::path& ScenePath, bool BRememberScene);
    /** @brief 정규화한 씬 경로를 기억하고 전역 설정과 파일에 저장합니다. @param ScenePath 새 씬 경로입니다. */
    void SetScenePath(const std::filesystem::path& ScenePath);

private:
    /** @brief 기반 엔진이 소유하는 에디터 월드의 비소유 참조입니다. */
    FWorldContext* mEditorWorldContext{};
    /** @brief 기반 엔진이 소유하는 PIE 월드의 비소유 참조입니다. */
    FWorldContext* mPIEWorldContext{};
    /** @brief 현재 PIE 요청 및 실행 상태입니다. */
    EPIEState mPIEState{EPIEState::Stopped};
    /** @brief 다음 프레임에 적용할 PIE 일시 정지 요청입니다. */
    bool mPauseRequested{};
    /** @brief PIE 처리와 종료의 재진입을 방지하는 상태입니다. */
    bool mProcessingPIE{};
    /** @brief 에디터 수명에 귀속된 편집 컨텍스트입니다. */
    std::unique_ptr<FWorldEditorContext> mEditorContext{};
    /** @brief 현재 에디터가 기억하는 정규화된 씬 경로입니다. */
    std::filesystem::path mScenePath{};
};

/**
 * @brief 현재 활성 엔진이 에디터 엔진이면 해당 인스턴스에 전역으로 접근합니다.
 * @return 편집 컨텍스트 초기화를 완료한 에디터 엔진의 비소유 포인터입니다. 일반 엔진 실행 중, 에디터 초기화 전과 종료 후에는 nullptr입니다.
 * @details GetEngine이 반환하는 동일한 객체를 에디터 타입으로 조회합니다. PIE 월드가 시작되어도 에디터 엔진 인스턴스는 바뀌지 않습니다.
 * @note 초기화와 종료는 외부에서 동기화해야 합니다. 반환한 포인터를 에디터 종료 후 사용하지 않습니다.
 */
UEditorEngine* GetEditorEngine() noexcept;
