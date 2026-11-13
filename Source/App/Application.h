/**
 * @file Application.h
 * @brief 플랫폼, 렌더러와 에디터를 연결하는 애플리케이션 수명을 관리합니다.
 * @ingroup App
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/FrameTimer.h"
#include "Editor/EditorEngine.h"
#include "Platform/Window.h"
#include "Renderer/Renderer.h"
#include "Engine/FProject.h"

namespace App {

    /**
     * @brief Windows 창, 렌더러와 에디터의 초기화 순서 및 메인 루프를 관리합니다.
     * @ingroup App
     */
    class FApplication {
    public:

        /**
         * @brief FApplication의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        FApplication();

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FApplication();

    public:

        /**
         * @brief 창과 렌더러·에디터를 초기화하고 종료 요청까지 프레임 루프를 실행합니다.
         * @return 정상 종료 시 0입니다.
         */
        int Run();

    private:

        /**
         * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
         */
        void Shutdown();

        /**
         * @brief 프로젝트 선택 화면을 반복 표시하고 선택한 프로젝트를 엽니다.
         * @param[in] Path 처음 선택해 둘 프로젝트 경로이며 비어 있을 수 있습니다.
         * @param[in] Create 새 프로젝트를 생성할지 지정합니다.
         * @return 성공하면 true, 실패하면 false입니다.
         */
        bool BrowseProjects(std::filesystem::path Path, bool Create);

        /**
         * @brief 메인 창 대신 스플래시 창을 띄운 채 프로젝트를 불러오고, 끝나면 메인 창을 다시 표시합니다.
         * @param[in] Path 열거나 생성할 프로젝트 경로입니다.
         * @param[in] Create true이면 새 프로젝트를 생성하고 false이면 기존 프로젝트를 엽니다.
         * @param[out] Error 프로젝트를 열지 못한 원인입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool OpenProject(const std::filesystem::path& Path, bool Create, Core::FString& Error);

        /**
         * @brief 현재 실행을 정리하고 프로젝트를 생성하거나 열어 시작 장면을 연결합니다.
         * @param[in] Path 열거나 생성할 프로젝트 경로입니다.
         * @param[in] Create true이면 새 프로젝트를 생성하고 false이면 기존 프로젝트를 엽니다.
         * @param[out] Error 프로젝트를 열지 못한 원인입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool LoadProject(const std::filesystem::path& Path, bool Create, Core::FString& Error);

    private:

        /**
         * @brief Windows 창과 메시지 처리 객체입니다.
         */
        Platform::FWindowsApplication mPlatform{};

        /**
         * @brief 애플리케이션이 소유한 렌더러입니다.
         */
        Renderer::FRenderer mRenderer{};

        /**
         * @brief 연결된 에디터 엔진입니다.
         */
        Editor::UEditorEngine mEngine{};

        /**
         * @brief 프레임 경과 시간 측정기입니다.
         */
        Core::FFrameTimer mTimer{};

        /**
         * @brief 현재 프로젝트를 단독 소유하는 객체입니다.
         */
        std::unique_ptr<Engine::FProject> mProject{};

        /**
         * @brief 새 프로젝트에 복사할 템플릿의 기준 폴더입니다.
         */
        std::filesystem::path mTemplateDirectory{};

        /**
         * @brief 프로젝트 생성에 사용할 템플릿 루트 디렉터리입니다.
         */
        std::filesystem::path mTemplatesDirectory{};

        /**
         * @brief 다음 안전한 전환 시점에 처리할 프로젝트 파일 경로입니다.
         */
        std::filesystem::path mPendingProject{};

        /**
         * @brief 보류 중인 프로젝트 요청이 새 프로젝트 생성인지 나타냅니다.
         */
        bool mCreateProject{};

        /**
         * @brief 다음 프레임의 프로젝트 브라우저 전환 요청입니다.
         */
        bool mBrowseProjects{};
    };

}
