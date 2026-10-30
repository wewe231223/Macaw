/**
 * @file IRendererModule.h
 * @brief 엔진과 에디터가 사용하는 렌더러의 수명 및 프레임 제출 인터페이스입니다.
 * @ingroup RenderCore
 */

#pragma once

#include "RenderCore/FShadowAtlasView.h"

#include "Core/ReflectionMacros.h"
#include "RenderCore/FRenderStats.h"

namespace Engine {
    class FSceneInterface;
    class FRenderTarget;
    struct FSceneViewFamily;
    struct FMeshData;
    enum class EPrimitiveType : Core::Uint8;
}

namespace Core {
    struct FVector2;
}

namespace RenderCore {

    /**
     * @brief 엔진과 에디터가 사용하는 렌더러의 수명 및 프레임 제출 인터페이스입니다.
     * @ingroup RenderCore
     */
    class IRendererModule {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(IRendererModule);

    public:
        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        virtual ~IRendererModule();

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(IRendererModule);

        /**
         * @brief Direct3D 11 장치, 스왑 체인과 렌더 파이프라인을 준비합니다.
         * @param[in] WindowHandle 렌더링할 네이티브 Win32 창 핸들입니다.
         * @param[in] Width 창 클라이언트 영역의 가로 픽셀 수입니다.
         * @param[in] Height 창 클라이언트 영역의 세로 픽셀 수입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        virtual bool Initialize(void* WindowHandle, Core::Uint32 Width, Core::Uint32 Height) = 0;

        /**
         * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
         */
        virtual void Shutdown() = 0;

        /**
         * @brief 렌더러 장치를 참조하는 새로운 렌더 장면을 생성합니다.
         * @return 생성한 객체의 단독 소유권이며, 실패하면 nullptr입니다.
         */
        virtual std::unique_ptr<Engine::FSceneInterface> AllocateScene() = 0;

        /**
         * @brief CPU 메시를 종류별 GPU 정점·인덱스 버퍼에 업로드합니다.
         * @param[in] Type 기본 프리미티브 종류입니다.
         * @param[in] Mesh 업로드할 CPU 메시입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        virtual bool UploadMesh(Engine::EPrimitiveType Type, const Engine::FMeshData& Mesh) = 0;

        /**
         * @brief 지정한 픽셀 크기의 오프스크린 렌더 타깃을 생성합니다.
         * @param[in] Width 렌더 타깃의 가로 픽셀 수입니다.
         * @param[in] Height 렌더 타깃의 세로 픽셀 수입니다.
         * @return 생성한 객체의 단독 소유권이며, 실패하면 nullptr입니다.
         */
        virtual std::unique_ptr<Engine::FRenderTarget> CreateRenderTarget(Core::Uint32 Width, Core::Uint32 Height) = 0;

        /**
         * @brief 기존 오프스크린 렌더 타깃을 새 픽셀 크기로 변경합니다.
         * @param[in,out] Target 크기를 바꿀 렌더 타깃입니다.
         * @param[in] Width 렌더 타깃의 가로 픽셀 수입니다.
         * @param[in] Height 렌더 타깃의 세로 픽셀 수입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        virtual bool ResizeRenderTarget(Engine::FRenderTarget* Target, Core::Uint32 Width, Core::Uint32 Height) = 0;

        /**
         * @brief 백 버퍼 크기를 동기화하고 프레임 렌더링 통계를 초기화합니다.
         * @param[in] Width 창 클라이언트 영역의 가로 픽셀 수입니다.
         * @param[in] Height 창 클라이언트 영역의 세로 픽셀 수입니다.
         */
        virtual void BeginFrame(Core::Uint32 Width, Core::Uint32 Height) = 0;

        /**
         * @brief 지정한 장면 뷰를 타깃에 렌더링하고 에디터 보조 선분을 그립니다.
         * @param[in] Family 장면, 출력 타깃과 카메라를 묶은 렌더 요청입니다.
         */
        virtual void BeginRenderingViewFamily(const Engine::FSceneViewFamily& Family) = 0;

        /**
         * @brief UI 렌더링에 사용할 백 버퍼와 뷰포트를 설정합니다.
         */
        virtual void BeginUiRender() = 0;

        /**
         * @brief 스왑 체인의 백 버퍼를 화면에 표시합니다.
         */
        virtual void Present() = 0;

        /**
         * @brief 비동기 프레젠테이션의 화면 찢김 허용 설정을 변경합니다.
         * @param[in] Allow 화면 찢김을 허용하려면 true입니다.
         * @return 요청을 수락하면 true이며 지원하지 않는 허용 요청은 false입니다.
         */
        virtual bool SetAllowTearing(bool Allow) = 0;

        /**
         * @brief 화면 찢김을 허용하도록 설정했는지 반환합니다.
         * @return 사용자 설정에서 화면 찢김을 허용하면 true입니다.
         */
        virtual bool IsTearingAllowed() const = 0;

        /**
         * @brief 현재 DXGI 환경이 화면 찢김 허용을 지원하는지 반환합니다.
         * @return DXGI의 프레젠테이션 기능 검사에 성공하면 true입니다.
         */
        virtual bool IsTearingSupported() const = 0;

        /**
         * @brief Direct3D 11 장치의 비소유 주소를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        virtual void* GetNativeDevice() const = 0;

        /**
         * @brief Direct3D 11 즉시 컨텍스트의 비소유 주소를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        virtual void* GetNativeContext() const = 0;

        /**
         * @brief 프레임 렌더링 통계를 반환합니다.
         * @return 프레임 렌더링 통계입니다.
         */
        virtual RenderCore::FRenderStats GetStats() const = 0;

        /**
         * @brief 현재 그림자 아틀라스의 검사 정보를 반환합니다.
         * @return 미리보기·깊이 뷰의 비소유 주소와 타일 정보 사본입니다.
         */
        virtual RenderCore::FShadowAtlasView GetShadowAtlasView() const;

        /**
         * @brief 변경된 셰이더를 다시 컴파일하고 필요한 GPU 셰이더를 갱신합니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        virtual bool ReloadChangedShaders() = 0;

        /**
         * @brief 폰트 메트릭으로 텍스트의 로컬 평면 경계를 계산합니다.
         * @param[in] Text UTF-8 문자열입니다.
         * @param[out] Minimum 계산한 최소 경계 좌표입니다.
         * @param[out] Maximum 계산한 최대 경계 좌표입니다.
         * @return 조건을 만족하면 true입니다.
         */
        virtual bool GetTextBounds(const Core::FString& Text, Core::FVector2& Minimum, Core::FVector2& Maximum) const;

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();
    };

}
