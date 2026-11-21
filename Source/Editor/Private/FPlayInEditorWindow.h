/**
 * @file FPlayInEditorWindow.h
 * @brief 별도 PIE 창과 스왑 체인·게임 뷰포트 및 입력 캡처를 관리합니다.
 * @ingroup Editor
 */

#pragma once

#include "Platform/Window.h"
#include "Engine/FGameViewportClient.h"
#include "Engine/FSceneViewport.h"
#include "RenderCore/IRendererModule.h"

#include <d3d11.h>
#include <dxgi.h>
#include <memory>
#include <wrl/client.h>

namespace Editor {

    /**
     * @brief 별도 PIE 창과 스왑 체인·게임 뷰포트 및 입력 캡처를 관리합니다.
     */
    class FPlayInEditorWindow {
    public:
        /**
         * @brief FPlayInEditorWindow의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        FPlayInEditorWindow();

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FPlayInEditorWindow();

    public:
        /**
         * @brief 별도 PIE 창과 스왑 체인·게임 뷰포트를 만들고 입력 캡처를 준비합니다.
         * @param[in] Renderer 렌더 대상과 GPU 리소스를 제공하는 렌더러입니다.
         * @param[in] Client 게임 뷰포트를 구동할 클라이언트의 비소유 주소입니다.
         * @param[in] Width 대상 영역의 너비이며 단위는 픽셀입니다.
         * @param[in] Height 대상 영역의 높이이며 단위는 픽셀입니다.
         * @param[in] CaptureInput 시작 시 게임 입력을 캡처할지 지정합니다.
         * @return 모든 창·렌더링 리소스를 초기화했으면 true입니다.
         */
        bool Initialize(RenderCore::IRendererModule* Renderer, Engine::FGameViewportClient* Client, Uint32 Width, Uint32 Height, bool CaptureInput);

        /**
         * @brief 입력 캡처·게임 뷰포트·스왑 체인과 별도 창을 정리합니다.
         */
        void Shutdown();

        /**
         * @brief 별도 창의 메시지·크기·입력을 처리하고 게임 프레임을 표시합니다.
         * @return 프레임을 표시하거나 최소화로 생략했으면 true이며 렌더링에 실패하면 false입니다.
         */
        bool Draw();

        /**
         * @brief PIE 창이 열려 있고 닫기 요청이 없는지 검사합니다.
         * @return 창이 실행 중이면 true입니다.
         */
        bool IsOpen() const;

        /**
         * @brief PIE 창의 입력 캡처 상태를 갱신하고 조회합니다.
         * @return 게임 입력이 캡처되어 있으면 true입니다.
         */
        bool IsInputCaptured();

        /**
         * @brief 게임의 마우스 캡처를 해제하고 입력 전달 상태를 정리합니다.
         */
        void ReleaseInput();

        /**
         * @brief PIE 창이 보관하는 프레임 입력 상태를 반환합니다.
         * @return 창의 입력 상태 참조입니다.
         */
        Core::FInput& GetInput();

    private:
        /**
         * @brief Win32 창·입력·파일 선택을 제공하는 플랫폼 애플리케이션입니다.
         */
        Platform::FWindowsApplication mPlatform{};

        /**
         * @brief 렌더링과 GPU 리소스를 제공하는 렌더러의 비소유 주소입니다.
         */
        RenderCore::IRendererModule* mRenderer{};

        /**
         * @brief 별도 PIE 창의 게임 뷰포트 소유 객체입니다.
         */
        std::unique_ptr<Engine::FSceneViewport> mViewport{};

        /**
         * @brief 별도 PIE 창의 스왑 체인 소유 객체입니다.
         */
        Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain{};

        /**
         * @brief 창 또는 렌더 대상의 너비입니다.
         */
        Uint32 mWidth{};

        /**
         * @brief 창 또는 드로어의 표시 높이입니다.
         */
        Uint32 mHeight{};

        /**
         * @brief 별도 PIE 창의 닫기 요청 여부입니다.
         */
        bool mCloseRequested{};

        /**
         * @brief 별도 PIE 창이 게임 입력을 캡처했는지 나타냅니다.
         */
        bool mInputCaptured{};
    };

}
