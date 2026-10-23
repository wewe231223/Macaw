/**
 * @file EditorEngine.h
 * @brief 에디터 UI, 입력과 장면 파일 편집을 관리합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/Engine.h"
#include "Editor/ViewportClient.h"
#include "Editor/Panel/FOutlinerPanel.h"
#include "Editor/Panel/FDetailsPanel.h"
#include "Editor/Panel/FConsolePanel.h"
#include "Editor/Panel/FStatisticsPanel.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include "Platform/Window.h"

#include <filesystem>

class AActor;
class AStaticMeshActor;
class FRenderTarget;
enum class EPrimitiveType : Uint8;

/**
 * @brief ImGui 패널, 선택, 프로퍼티 편집, 뷰포트 및 장면 파일을 관리합니다.
 * @ingroup Editor
 */
class UEditorEngine final : public UEngine {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(UEditorEngine, UEngine);

public:
    /**
     * @brief UEditorEngine의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    UEditorEngine();
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~UEditorEngine() override;

public:
    /**
     * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectClass(UEditorEngine, EClassFlags::NoFactory);
    /**
     * @fn UClass* UEditorEngine::GetClass() const
     * @brief 객체의 실제 클래스 메타데이터를 반환합니다.
     * @return 실제 클래스 메타데이터의 비소유 주소입니다.
     */

    /**
     * @brief 엔진, ImGui 백엔드, 뷰포트와 초기 장면을 준비합니다.
     * @param[in,out] Renderer 뷰포트 및 엔진이 참조할 렌더러입니다.
     * @param[in,out] Platform Win32 창과 메시지를 제공하는 플랫폼 객체입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool InitializeEditor(IRendererModule* Renderer, FWindowsApplication* Platform);
    /**
     * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
     */
    void Shutdown() override;
    /**
     * @brief 입력을 수집하고 패널, 월드 동기화와 에디터 프레임 렌더링을 진행합니다.
     * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
     */
    void TickFrame(float DeltaTime);

    /**
     * @brief 지정한 기본 메시 종류의 액터를 생성하여 영속 레벨에 추가합니다.
     * @param[in] Type 생성, 조회 또는 표시할 타입입니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    AStaticMeshActor* SpawnPrimitive(EPrimitiveType Type);
    /**
     * @brief 현재 선택한 액터를 제거하고 관련 선택 상태를 초기화합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool DestroySelectedActor();

    /**
     * @brief 유효한 액터 핸들을 선택하고 프로퍼티 편집 대상으로 지정합니다.
     * @param[in] Handle 조회 또는 제거할 객체의 슬롯 세대 핸들입니다.
     */
    void SelectActor(FObjectHandle Handle);
    /**
     * @brief 액터와 객체 선택 핸들을 모두 무효화합니다.
     */
    void ClearSelection();
    /**
     * @brief 선택한 액터의 세대 핸들을 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    AActor* GetSelectedActor() const;
    /**
     * @brief 프로퍼티 편집 대상의 세대 핸들을 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    UObject* GetSelectedObject() const;
    /**
     * @brief 레지스트리에서 유효한 객체를 확인한 뒤 편집 대상으로 선택합니다.
     * @param[in] Handle 조회 또는 제거할 객체의 슬롯 세대 핸들입니다.
     */
    void SelectObject(FObjectHandle Handle);

    /**
     * @brief 프로퍼티 소속과 편집 권한을 확인하고 변경 전후 알림과 함께 값을 씁니다.
     * @param[in,out] Object 대상 객체입니다.
     * @param[in] Property 읽기, 쓰기 또는 알림을 적용할 프로퍼티 메타데이터입니다.
     * @param[in] Value 프로퍼티 타입에 대응하는 variant 값입니다.
     * @param[in] Interactive 드래그 등 연속 편집 중인지 지정합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool EditProperty(UObject* Object, const FProperty* Property, const FPropertyValue& Value, bool Interactive = false);
    /**
     * @brief 액터 루트의 변환 프로퍼티를 에디터 편집 경로로 변경합니다.
     * @param[in,out] Actor 생성, 편집 또는 직렬화할 액터입니다.
     * @param[in] Transform 적용할 위치, 회전과 스케일입니다.
     * @param[in] Interactive 드래그 등 연속 편집 중인지 지정합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool ApplyTransform(AActor* Actor, const FTransform& Transform, bool Interactive = false);

    /**
     * @brief 현재 월드를 지정한 장면 파일로 저장하고 성공하면 변경 표시를 해제합니다.
     * @param[in] Path 읽거나 쓸 파일 경로입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool SaveScene(const std::filesystem::path& Path);
    /**
     * @brief 장면 파일을 새 컨텍스트에 복원한 뒤 현재 월드와 선택 상태를 교체합니다.
     * @param[in] Path 읽거나 쓸 파일 경로입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool LoadScene(const std::filesystem::path& Path);
    /**
     * @brief 빈 월드 컨텍스트로 교체하고 선택 및 장면 파일 경로를 초기화합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool NewScene();

    /**
     * @brief 콘솔 패널에 메시지를 추가하고 스크롤 갱신을 요청합니다.
     * @param[in] Message 보고할 내용 또는 Windows 메시지 번호입니다.
     */
    void AddLog(const FString& Message);
    /**
     * @brief 입력한 콘솔 명령을 해석하여 에디터 동작을 실행합니다.
     * @param[in] Command 실행할 콘솔 명령 문자열입니다.
     */
    void ExecuteConsoleCommand(const FString& Command);

    /**
     * @brief 에디터 뷰포트 클라이언트를 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FEditorViewportClient& GetViewportClient();
    /**
     * @brief 에디터 뷰포트 렌더 타깃의 비소유 주소를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    FRenderTarget* GetViewportTarget() const;

    /**
     * @brief 저장 후 변경되어 저장이 필요한 장면인지 검사합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool IsSceneDirty() const;
    /**
     * @brief 에디터에서 프로그램 종료를 요청했는지 검사합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool WantsExit() const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

    /**
     * @brief 장면 파일, 프리미티브 생성과 패널 표시 메뉴를 그립니다.
     */
    void DrawMenu();
    /**
     * @brief 장면 타깃을 표시하고 뷰포트 입력 및 크기 변경을 전달합니다.
     */
    void DrawViewport();

    /**
     * @brief 에디터 패널의 초기 ImGui 도킹 레이아웃을 구성합니다.
     */
    void BuildLayout();
    /**
     * @brief 장면 로드 또는 저장용 Windows 파일 대화상자를 표시합니다.
     * @param[in] Save 저장용 대화상자를 열지 지정합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    std::filesystem::path ChooseFile(bool Save);

private:
    /**
     * @brief Windows 창과 메시지 처리 객체입니다.
     */
    FWindowsApplication* mPlatform{};
    ReflectProperty(mPlatform, "Platform", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 뷰포트 클라이언트의 비소유 주소입니다.
     */
    FEditorViewportClient mClient;
    ReflectProperty(mClient, "Client", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 에디터가 소유한 장면 뷰포트입니다.
     */
    std::unique_ptr<FSceneViewport> mViewport{};
    ReflectProperty(mViewport, "Viewport", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 액터·컴포넌트 계층 패널입니다.
     */
    FOutlinerPanel mOutlinerPanel;
    ReflectProperty(mOutlinerPanel, "OutlinerPanel", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 선택 객체의 프로퍼티 패널입니다.
     */
    FDetailsPanel mDetailsPanel;
    ReflectProperty(mDetailsPanel, "DetailsPanel", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 로그와 명령 입력 패널입니다.
     */
    FConsolePanel mConsolePanel;
    ReflectProperty(mConsolePanel, "ConsolePanel", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 프레임 및 리소스 통계 패널입니다.
     */
    FStatisticsPanel mStatisticsPanel;
    ReflectProperty(mStatisticsPanel, "StatisticsPanel", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 선택한 액터의 세대 핸들입니다.
     */
    FObjectHandle mSelectedActor{};
    ReflectProperty(mSelectedActor, "SelectedActor", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 프로퍼티 편집 대상의 세대 핸들입니다.
     */
    FObjectHandle mSelectedObject{};
    ReflectProperty(mSelectedObject, "SelectedObject", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 현재 장면 파일 경로입니다.
     */
    std::filesystem::path mScenePath{};
    ReflectProperty(mScenePath, "ScenePath", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief ImGui 설정 파일 경로입니다.
     */
    FString mIniPath{};
    ReflectProperty(mIniPath, "IniPath", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 현재 프레임의 경과 시간(초)입니다.
     */
    float mDeltaTime{0.016f};
    ReflectProperty(mDeltaTime, "DeltaTime", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 에디터 초기화 완료 여부입니다.
     */
    bool mEditorInitialized{};
    ReflectProperty(mEditorInitialized, "EditorInitialized", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief ImGui 컨텍스트 생성 여부입니다.
     */
    bool mContextCreated{};
    ReflectProperty(mContextCreated, "ContextCreated", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief ImGui Win32 백엔드 초기화 여부입니다.
     */
    bool mWin32Ready{};
    ReflectProperty(mWin32Ready, "Win32Ready", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief ImGui Direct3D 11 백엔드 초기화 여부입니다.
     */
    bool mDxReady{};
    ReflectProperty(mDxReady, "DxReady", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 저장 이후 장면이 변경되었는지 여부입니다.
     */
    bool mDirty{};
    ReflectProperty(mDirty, "Dirty", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 프로그램 종료 요청 여부입니다.
     */
    bool mExit{};
    ReflectProperty(mExit, "Exit", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
