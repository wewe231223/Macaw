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
         * @brief Game 모듈의 리플렉션 타입을 등록합니다.
         */
        FGameApplication();
        /**
         * @brief 메시지 처리기, 뷰포트, 엔진, 렌더러와 창을 정리합니다.
         */
        ~FGameApplication();

    public:
        /**
         * @brief 프로젝트를 초기화하고 창이 닫히거나 월드가 정지할 때까지 프레임 루프를 실행합니다.
         * @param[in] ProjectFile 실행할 프로젝트(.mproject) 파일 경로입니다.
         * @return 정상 종료하면 0, 초기화나 프레임 처리에 실패하면 1입니다.
         */
        int Run(const std::filesystem::path& ProjectFile);

    private:
        /**
         * @brief 프로젝트를 열고 창, 렌더러, 엔진을 초기화한 뒤 시작 장면으로 게임 월드를 시작합니다.
         * @param[in] ProjectFile 열 프로젝트(.mproject) 파일 경로입니다.
         * @param[out] Error 실패 원인을 받을 문자열입니다.
         * @return 시작 장면의 BeginPlay까지 성공하면 true, 실패하면 false입니다.
         * @note 시작 장면에 활성 카메라가 없으면 실패합니다.
         */
        bool Initialize(const std::filesystem::path& ProjectFile, Core::FString& Error);
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

        /**
         * @brief 애플리케이션이 소유한 렌더러입니다.
         */
        Renderer::FRenderer mRenderer{};

        /**
         * @brief 실행 중인 프로젝트입니다.
         */
        Engine::FProject mProject{};

        /**
         * @brief 게임 월드를 소유하고 갱신하는 엔진입니다.
         */
        Engine::UEngine mEngine{};

        /**
         * @brief 게임 월드의 입력과 장면 뷰를 처리하는 뷰포트 클라이언트입니다.
         */
        Engine::FGameViewportClient mGameClient{&mEngine};

        /**
         * @brief 게임 화면을 그릴 오프스크린 뷰포트이며, 초기화 전에는 비어 있습니다.
         */
        std::unique_ptr<Engine::FSceneViewport> mViewport{};

        /**
         * @brief 프레임 경과 시간 측정기입니다.
         */
        Core::FFrameTimer mTimer{};
    };

}
