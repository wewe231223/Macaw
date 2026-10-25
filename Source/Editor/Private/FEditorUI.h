/**
 * @file FEditorUI.h
 * @brief 에디터 창, 패널과 뷰포트 표시 수명을 묶는 UI 호스트를 정의합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "Editor/Panel/FOutlinerPanel.h"
#include "Editor/Panel/FDetailsPanel.h"
#include "Editor/Panel/FConsolePanel.h"

#include <filesystem>

class FEditorContext;
class FEditorViewport;
class FWindowsApplication;
class IRendererModule;
class FPlayInEditorSession;

/**
 * @brief 편집 모델을 참조하여 창과 패널을 표시하고 ImGui 백엔드 수명을 관리합니다.
 * @ingroup Editor
 * @note 선택, 장면 경로와 변경 상태는 FEditorContext가 소유합니다.
 */
class FEditorUI {
public:
    /**
     * @brief 리플렉션에서 사용할 소유 타입 별칭입니다.
     */
    ReflectTypes(FEditorUI);

public:
    /**
     * @brief UI가 참조할 편집 경로와 뷰포트를 연결합니다.
     * @param[in,out] Context 패널이 공유하는 비소유 편집 컨텍스트입니다.
     * @param[in,out] Viewport 화면에 배치할 독립 뷰포트입니다.
     */
    FEditorUI(FEditorContext* Context, FEditorViewport* Viewport, FPlayInEditorSession* PlaySession);
    /**
     * @brief UI 백엔드와 소유 표시 리소스를 정리합니다.
     */
    ~FEditorUI();

public:
    /**
     * @brief 정적 구조체 메타데이터를 반환합니다.
     * @return 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FEditorUI);
    /**
     * @brief 독립 뷰포트를 배치할 창의 ImGui 백엔드를 준비합니다.
     * @param[in,out] Renderer ImGui 표시 리소스를 제공하는 렌더러입니다.
     * @param[in,out] Platform UI 입력을 연결할 플랫폼 창입니다.
     * @return 준비에 성공하면 true입니다.
     */
    bool Initialize(IRendererModule* Renderer, FWindowsApplication* Platform);
    /**
     * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
     */
    void Shutdown();
    /**
     * @brief ImGui 프레임과 입력, 패널 UI를 구성합니다.
     * @param[in] DeltaTime 이번 프레임의 초 단위 시간입니다.
     * @return UI가 초기화되어 프레임을 시작하면 true입니다.
     */
    bool BeginFrame(float DeltaTime);
    /**
     * @brief 장면과 통계를 표시하고 UI 렌더 명령을 제출합니다.
     * @param[in] DeltaTime 통계에 표시할 프레임 시간입니다.
     * @note BeginFrame() 이후 엔진 갱신과 월드 렌더 동기화를 수행한 뒤 호출합니다.
     */
    void EndFrame(float DeltaTime);
    /**
     * @brief 에디터에서 프로그램 종료를 요청했는지 검사합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool WantsExit() const;

private:
    /**
     * @brief 생성된 멤버 등록 함수의 접근 권한입니다.
     */
    ReflectMembers();
    /**
     * @brief 장면 파일과 패널 표시 메뉴를 그립니다.
     */
    void DrawMenu();
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
     * @brief 패널이 공유하는 편집 컨텍스트의 비소유 주소입니다.
     */
    FEditorContext* mContext{};
    ReflectProperty(mContext, "Context", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief UI에 배치할 독립 뷰포트의 비소유 주소입니다.
     */
    FEditorViewport* mViewport{};
    ReflectProperty(mViewport, "Viewport", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FPlayInEditorSession* mPlaySession{};
    ReflectProperty(mPlaySession, "PlaySession", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief Windows 창과 메시지 처리 객체입니다.
     */
    FWindowsApplication* mPlatform{};
    ReflectProperty(mPlatform, "Platform", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


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
     * @brief ImGui 설정 파일 경로입니다.
     */
    FString mIniPath{};
    ReflectProperty(mIniPath, "IniPath", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 에디터 초기화 완료 여부입니다.
     */
    bool mInitialized{};
    ReflectProperty(mInitialized, "Initialized", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

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
     * @brief 프로그램 종료 요청 여부입니다.
     */
    bool mExit{};
    ReflectProperty(mExit, "Exit", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

};
