/**
 * @file FGameApplication.h
 * @brief 내보낸 프로젝트의 시작 장면을 에디터 없이 실행하는 게임 애플리케이션입니다.
 * @ingroup Game
 */

#pragma once

#include "Core/FrameTimer.h"
#include "Engine/Engine.h"
#include "Engine/FGameViewportClient.h"
#include "Engine/FProject.h"
#include "Engine/FSceneViewport.h"
#include "Engine/FViewportInput.h"
#include "Platform/Window.h"
#include "Renderer/Renderer.h"

namespace Game {

    /**
     * @brief 프로젝트를 열고 시작 장면을 Game 월드로 불러와 창, 입력, 렌더링 루프를 실행합니다.
     * @ingroup Game
     */
    class FGameApplication {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FGameApplication);

    public:
        /**
         * @brief Game 모듈의 리플렉션 타입을 등록합니다.
         */
        FGameApplication();
        /**
         * @brief 메시지 처리기, 뷰포트, 엔진, 렌더러와 창을 정리합니다.
         */
        ~FGameApplication();

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FGameApplication);

        /**
         * @brief 프로젝트를 초기화하고 창이 닫히거나 월드가 정지할 때까지 프레임 루프를 실행합니다.
         * @param[in] ProjectFile 실행할 프로젝트(.mproject) 파일 경로입니다.
         * @return 정상 종료하면 0, 초기화나 프레임 처리에 실패하면 1입니다.
         */
        int Run(const std::filesystem::path& ProjectFile);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

        /**
         * @brief 프로젝트를 열고 창, 렌더러, 엔진을 초기화한 뒤 시작 장면으로 게임 월드를 시작합니다.
         * @param[in] ProjectFile 열 프로젝트(.mproject) 파일 경로입니다.
         * @param[out] Error 실패 원인을 받을 문자열입니다.
         * @return 시작 장면의 BeginPlay까지 성공하면 true, 실패하면 false입니다.
         * @note 시작 장면에 활성 카메라가 없으면 실패합니다.
         */
        bool Initialize(const std::filesystem::path& ProjectFile, Core::FString& Error);
        /**
         * @brief 현재 마우스·키보드 상태를 읽어 이번 프레임의 뷰포트 입력을 만듭니다.
         * @return 포커스, 마우스 위치·이동량, 버튼과 WASD/QE 이동 키 상태를 담은 입력입니다.
         * @note 창에 포커스가 없거나 커서 위치를 읽지 못하면 버튼과 키 상태를 채우지 않습니다. 호출 후 누적 휠 값은 0으로 초기화됩니다.
         */
        Engine::FViewportInput ReadInput();
        /**
         * @brief 마우스 휠을 누적하고 버튼 입력 동안 마우스 캡처를 관리합니다.
         * @param[in] Window 메시지를 받은 창 핸들입니다.
         * @param[in] Message Windows 메시지 번호입니다.
         * @param[in] WParam 메시지의 WPARAM 값입니다.
         * @param[in] LParam 메시지의 LPARAM 값이며 사용하지 않습니다.
         * @return 메시지를 직접 처리했으면 true, 기본 처리로 넘길 메시지면 false입니다.
         */
        bool HandleMessage(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);
        /**
         * @brief 오류를 기록하고 오류 대화상자를 표시합니다.
         * @param[in] Error 보고할 오류 메시지입니다.
         * @return 프로세스 종료 코드로 사용할 1입니다.
         */
        int ReportFailure(const Core::FString& Error) const;
        /**
         * @brief 메시지 처리기, 뷰포트, 엔진, 렌더러와 창을 역순으로 정리하며 여러 번 호출해도 안전합니다.
         */
        void Shutdown();

    private:
        /**
         * @brief 게임 창과 Windows 메시지 처리 객체입니다.
         */
        Platform::FWindowsApplication mPlatform{};
        ReflectProperty(mPlatform, "Platform", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 애플리케이션이 소유한 렌더러입니다.
         */
        Renderer::FRenderer mRenderer{};
        ReflectProperty(mRenderer, "Renderer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 실행 중인 프로젝트입니다.
         */
        Engine::FProject mProject{};
        ReflectProperty(mProject, "Project", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 게임 월드를 소유하고 갱신하는 엔진입니다.
         */
        Engine::UEngine mEngine{};
        ReflectProperty(mEngine, "Engine", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 게임 월드의 입력과 장면 뷰를 처리하는 뷰포트 클라이언트입니다.
         */
        Engine::FGameViewportClient mGameClient{&mEngine};
        ReflectProperty(mGameClient, "GameClient", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 게임 화면을 그릴 오프스크린 뷰포트이며, 초기화 전에는 비어 있습니다.
         */
        std::unique_ptr<Engine::FSceneViewport> mViewport{};
        ReflectProperty(mViewport, "Viewport", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 프레임 경과 시간 측정기입니다.
         */
        Core::FFrameTimer mTimer{};
        ReflectProperty(mTimer, "Timer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 이전 프레임의 클라이언트 좌표 마우스 위치입니다.
         */
        Core::FVector2 mPreviousMouse{};
        ReflectProperty(mPreviousMouse, "PreviousMouse", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 다음 입력 읽기 전까지 누적한 마우스 휠 칸 수입니다.
         */
        float mWheel{};
        ReflectProperty(mWheel, "Wheel", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 이전 마우스 위치가 유효해 이동량을 계산할 수 있는지 나타냅니다.
         */
        bool mMouseReady{};
        ReflectProperty(mMouseReady, "MouseReady", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 이전 프레임의 왼쪽 버튼 눌림 상태입니다.
         */
        bool mLeftDown{};
        ReflectProperty(mLeftDown, "LeftDown", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 이전 프레임의 가운데 버튼 눌림 상태입니다.
         */
        bool mMiddleDown{};
        ReflectProperty(mMiddleDown, "MiddleDown", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
