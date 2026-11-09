/**
 * @file FObjViewerApplication.h
 * @brief 프로젝트 브라우저 없이 실행하는 OBJ 뷰어입니다.
 * @ingroup App
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Types.h"
#include "Platform/Window.h"
#include "Renderer/Renderer.h"
#include "Editor/FImGuiUIScale.h"

#include <filesystem>

namespace Asset {
    class FAssetManager;
}

namespace Editor {
    class FMeshViewer;
}

namespace App {

    /**
     * @brief 에디터와 같은 임포터·렌더러·메시 뷰어를 사용하는 독립 실행 호스트입니다.
     */
    class FObjViewerApplication {
    public:
        /**
         * @brief 비어 있는 독립 뷰어 호스트를 구성합니다.
         */
        FObjViewerApplication();

        /**
         * @brief 뷰어, ImGui와 GPU 자원을 역순으로 정리합니다.
         */
        ~FObjViewerApplication();

    public:
        /**
         * @brief 독립 뷰어의 창과 이벤트 루프를 실행합니다.
         * @return 정상 종료는 0이며 초기화 실패는 1입니다.
         */
        int Run();

    private:
        /**
         * @brief 외부 메시를 뷰어 Content에 임포트하고 엽니다.
         * @param Source 원본 OBJ 등의 경로입니다.
         * @return 임포트와 미리보기에 성공하면 true입니다.
         */
        bool OpenSource(const std::filesystem::path& Source);

        /**
         * @brief 원본 메시 파일을 선택합니다.
         * @return 선택한 경로이며 취소하면 빈 경로입니다.
         */
        std::filesystem::path ChooseSource();

        /**
         * @brief ImGui와 렌더러를 안전하게 해제합니다.
         */
        void Shutdown();

    private:
        /**
         * @brief 독립 실행 창과 메시지 처리기입니다.
         */
        Platform::FWindowsApplication mPlatform{};

        /**
         * @brief 뷰어 전용 렌더러입니다.
         */
        Renderer::FRenderer mRenderer{};

        /**
         * @brief 임포트한 뷰어 에셋의 소유 관리자입니다.
         */
        std::unique_ptr<Asset::FAssetManager> mAssets{};

        /**
         * @brief 에디터에서도 사용하는 메시 미리보기입니다.
         */
        std::unique_ptr<Editor::FMeshViewer> mViewer{};

        /**
         * @brief 최근 임포트 오류입니다.
         */
        Core::FString mError{};

        /**
         * @brief 에디터 ImGui 스타일의 화면 배율 관리자입니다.
         */
        Editor::FImGuiUIScale mUIScale{};

        /**
         * @brief ImGui 컨텍스트의 생성 상태입니다.
         */
        bool mContextCreated{};

        /**
         * @brief Win32 백엔드 초기화 상태입니다.
         */
        bool mWin32Ready{};

        /**
         * @brief Direct3D 백엔드 초기화 상태입니다.
         */
        bool mDxReady{};
    };

}
