/**
 * @file FAssetViewer.h
 * @brief 에셋별 콘텐츠의 카메라, 렌더 타깃, Fit과 이미지 캡처를 공통으로 관리합니다.
 * @ingroup Viewer
 */

#pragma once

#include "Viewer/FViewerCamera.h"
#include "Viewer/FViewerTypes.h"
#include "Asset/FAssetFile.h"
#include "Engine/FEngineShowFlags.h"
#include "Engine/EViewMode.h"
#include "Core/Math/FVector4.h"

#include <memory>

namespace Asset {
    class FAssetManager;
}

namespace RenderCore {
    class IRendererModule;
}

namespace Engine {
    class FRenderTarget;
    class FPrimitiveDrawInterface;
}

namespace CoreUObject {
    class UObject;
}

namespace Viewer {
    class IViewerContent;
    class FViewerCapture;

    /**
     * @brief 에셋별 콘텐츠의 카메라, 렌더 타깃, Fit과 이미지 캡처를 공통으로 관리합니다.
     * @ingroup Viewer
     */
    class FAssetViewer {
    public:

        /**
         * @brief 렌더러·에셋 서비스를 참조하고 독립 뷰어의 기본 표시 옵션을 구성합니다.
         * @param Renderer 외부에서 소유하는 렌더러 서비스입니다.
         * @param Assets 외부에서 소유하는 에셋 관리자입니다.
         */
        FAssetViewer(RenderCore::IRendererModule* Renderer, Asset::FAssetManager* Assets);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FAssetViewer();

    public:

        /**
         * @brief 해당 에셋 종류에 공통 뷰어 콘텐츠가 제공되는지 확인합니다.
         * @param Type 처리할 에셋 또는 노드 종류입니다.
         * @return 현재 뷰어에서 지원하는 종류이면 true입니다.
         */
        static bool Supports(Asset::EAssetType Type);

        /**
         * @brief 에셋 종류와 실제 UObject 형식을 검증하여 대응 콘텐츠를 연결합니다.
         * @param Object 외부에서 수명을 보장하는 에셋 객체입니다.
         * @param Type Object의 실제 형식과 일치해야 하는 에셋 종류입니다.
         * @return 연결에 성공하면 true이며 실패 원인은 GetError로 조회하고 기존 콘텐츠는 유지합니다.
         */
        bool SetAsset(CoreUObject::UObject* Object, Asset::EAssetType Type);

        /**
         * @brief 최근 에셋·콘텐츠 교체 오류를 반환합니다.
         * @return 뷰어가 보관하는 오류 문자열의 읽기 전용 참조입니다.
         */
        const Core::FString& GetError() const;

        /**
         * @brief 준비된 콘텐츠의 소유권을 받아 현재 콘텐츠를 교체하고 카메라를 초기화합니다.
         * @param Content 뷰어로 소유권을 이전할 준비된 콘텐츠입니다.
         * @return 콘텐츠를 연결했으면 true입니다.
         */
        bool SetContent(std::unique_ptr<Viewer::IViewerContent> Content);

        /**
         * @brief 현재 콘텐츠와 렌더 타깃을 해제합니다.
         */
        void Close();

        /**
         * @brief 현재 에셋의 형식별 콘텐츠를 조회합니다.
         * @return 뷰어 소유의 비소유 포인터이며 콘텐츠가 없으면 nullptr입니다.
         */
        Viewer::IViewerContent* GetContent() const;

        /**
         * @brief 출력 크기를 검증하고 필요한 장면 렌더 타깃을 생성하거나 재사용합니다.
         * @param Width 출력 또는 UI 영역의 너비(픽셀)입니다.
         * @param Height 출력 또는 UI 영역의 높이(픽셀)입니다.
         * @return 각 변이 1~16384이고 필요한 리소스가 준비되면 true입니다.
         */
        bool Resize(Core::Uint32 Width, Core::Uint32 Height);

        /**
         * @brief 현재 콘텐츠의 전체 경계가 출력 영역에 들어오도록 카메라 또는 이미지 여백을 맞춥니다.
         * @param Padding 전체 경계 주위의 여백 배율입니다.
         * @return 경계와 출력 설정이 유효하여 맞춤에 성공하면 true입니다.
         */
        bool Fit(float Padding = 1.1f);

        /**
         * @brief 지정 상태 또는 콘텐츠 기본 상태로 촬영 준비를 위임합니다.
         * @param State 준비·저장·복원할 상태 데이터입니다.
         * @param Error 실패 원인을 기록할 출력 문자열입니다.
         * @return 콘텐츠의 준비 결과이며 실패 원인은 Error에 기록합니다.
         */
        Viewer::EViewerPrepareResult PrepareCapture(const Viewer::FViewerCaptureState& State, Core::FString& Error);

        /**
         * @brief 지정 상태 또는 콘텐츠 기본 상태로 촬영 준비를 위임합니다.
         * @param Error 실패 원인을 기록할 출력 문자열입니다.
         * @return 콘텐츠의 준비 결과이며 실패 원인은 Error에 기록합니다.
         */
        Viewer::EViewerPrepareResult PrepareCapture(Core::FString& Error);

        /**
         * @brief 유한한 음수 아닌 경과 시간을 현재 콘텐츠에 전달합니다.
         * @param DeltaTime 이전 갱신 이후 경과 시간(초)입니다.
         */
        void Tick(float DeltaTime);

        /**
         * @brief 콘텐츠를 동기화하고 독립 장면 또는 직접 2D 이미지를 렌더링합니다.
         * @param EditorElements 장면 위에 표시할 외부 소유 에디터 오버레이이며 없으면 nullptr입니다.
         * @param BackgroundOverlay 배경 깊이와 함께 표시할 외부 소유 오버레이이며 없으면 nullptr입니다.
         * @return 현재 콘텐츠의 표시 결과를 준비했으면 true입니다.
         */
        bool Render(const Engine::FPrimitiveDrawInterface* EditorElements = nullptr, const Engine::FPrimitiveDrawInterface* BackgroundOverlay = nullptr);

        /**
         * @brief 현재 콘텐츠를 렌더링한 뒤 독립된 최종 컬러 이미지를 만듭니다.
         * @param Error 실패 원인을 기록할 출력 문자열입니다.
         * @return 호출자가 소유하는 캡처이며 실패하면 nullptr이고 Error에 원인을 기록합니다.
         */
        std::unique_ptr<Viewer::FViewerCapture> Capture(Core::FString& Error);

        /**
         * @brief 궤도 카메라의 상태와 조작 인터페이스를 제공합니다.
         * @return 뷰어가 소유하는 카메라 참조입니다.
         */
        Viewer::FViewerCamera& GetCamera();

        /**
         * @brief 궤도 카메라의 상태와 조작 인터페이스를 제공합니다.
         * @return 뷰어가 소유하는 카메라 참조입니다.
         */
        const Viewer::FViewerCamera& GetCamera() const;

        /**
         * @brief 뷰어별 렌더링 표시 플래그를 제공합니다.
         * @return 뷰어가 소유하는 플래그의 수정 가능한 참조입니다.
         */
        Engine::FEngineShowFlags& GetShowFlags();

        /**
         * @brief 뷰어 장면의 셰이딩 또는 버퍼 시각화 모드를 설정합니다.
         * @param Mode 설정할 동작 또는 표시 모드입니다.
         */
        void SetViewMode(Engine::EViewMode Mode);

        /**
         * @brief 유한한 음수 아닌 월드 단위 격자 간격을 설정하며 0이면 격자를 끕니다.
         * @param Spacing 월드 단위 간격이며 0은 격자를 끕니다.
         */
        void SetGridSpacing(float Spacing);

        /**
         * @brief 현재 출력 영역 크기를 반환합니다.
         * @return 너비·높이(픽셀)입니다.
         */
        Core::FVector2 GetSize() const;

        /**
         * @brief 출력 영역 안에서 종횡비와 여백을 반영한 이미지 사각형을 계산합니다.
         * @return 픽셀 단위의 X, Y, 너비, 높이입니다.
         */
        Core::FVector4 GetImageRect() const;

        /**
         * @brief UI에서 표시할 네이티브 텍스처 핸들을 반환합니다.
         * @return 소유권을 이전하지 않는 SRV 핸들이며 표시할 이미지가 없으면 nullptr입니다.
         */
        void* GetNativeTexture() const;

    private:

        /**
         * @brief 외부에서 소유하는 렌더러 서비스입니다.
         */
        RenderCore::IRendererModule* mRenderer{};

        /**
         * @brief 외부에서 소유하는 에셋 관리자입니다.
         */
        Asset::FAssetManager* mAssets{};

        /**
         * @brief 마지막 에셋 또는 콘텐츠 교체 실패의 원인입니다.
         */
        Core::FString mError{};

        /**
         * @brief 현재 에셋의 형식별 콘텐츠를 소유합니다.
         */
        std::unique_ptr<Viewer::IViewerContent> mContent{};

        /**
         * @brief 뷰어가 재사용하는 3D 장면 렌더 타깃의 소유 포인터입니다.
         */
        std::unique_ptr<Engine::FRenderTarget> mTarget{};

        /**
         * @brief Fit과 사용자 궤도 조작을 담당하는 카메라입니다.
         */
        Viewer::FViewerCamera mCamera{};

        /**
         * @brief 이 뷰어에만 적용하는 렌더링 표시 플래그입니다.
         */
        Engine::FEngineShowFlags mShowFlags{};

        /**
         * @brief 이 뷰어의 셰이딩·버퍼 시각화 모드입니다.
         */
        Engine::EViewMode mViewMode{Engine::EViewMode::Unlit};

        /**
         * @brief 뷰어 출력 너비·높이(픽셀)입니다.
         */
        Core::FVector2 mSize{256.0f, 256.0f};

        /**
         * @brief 배경 격자의 월드 단위 간격이며 0은 표시하지 않습니다.
         */
        float mGridSpacing{};

        /**
         * @brief 2D 이미지 Fit에 적용할 여백 배율입니다.
         */
        float mImagePadding{1.0f};
    };
}
