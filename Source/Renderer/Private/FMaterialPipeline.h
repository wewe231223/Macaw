/**
 * @file FMaterialPipeline.h
 * @brief 머티리얼 셰이더와 양면 정책별 GPU 파이프라인을 관리합니다.
 * @ingroup Renderer
 */

#pragma once

#include "EMeshPass.h"
#include "RenderCore/Shader/FShader.h"

#include <d3d11.h>
#include <wrl/client.h>

namespace Renderer {

    /**
     * @brief 머티리얼 셰이더와 양면 정책별 GPU 파이프라인을 관리합니다.
     */
    class FMaterialPipeline {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FMaterialPipeline);

    public:
        /**
         * @brief 전달된 설정과 기본값으로 FMaterialPipeline 객체를 생성합니다.
         * @param[in] Pass 입력 레이아웃·래스터 상태를 결정하는 메시 패스입니다.
         * @param[in] VertexCode 컴파일된 정점 셰이더 바이트코드입니다.
         * @param[in] PixelCode 컴파일된 픽셀 셰이더 바이트코드이며, 깊이 전용 패스는 nullptr입니다.
         * @param[in] TwoSided 양면 렌더링을 사용할지 지정합니다.
         * @param[in] Instanced 메시 인스턴스의 월드 행렬을 정점 입력으로 사용하려면 true입니다.
         */
        FMaterialPipeline(Renderer::EMeshPass Pass, std::shared_ptr<const RenderCore::FShader> VertexCode, std::shared_ptr<const RenderCore::FShader> PixelCode, bool TwoSided, bool Instanced = false);

        /**
         * @brief 기본 소멸자이며, GPU 자원은 ComPtr 멤버가 해제합니다.
         */
        ~FMaterialPipeline();

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FMaterialPipeline);

        /**
         * @brief GPU 파이프라인이 현재 셰이더 개정과 일치하는지 검사합니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool IsCurrent() const;

        /**
         * @brief 렌더링에 필요한 GPU 자원이 준비되었는지 검사합니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool HasResources() const;

        /**
         * @brief 메시 인스턴스 데이터를 사용하는 파이프라인인지 반환합니다.
         * @return 인스턴스 월드 행렬을 입력으로 사용하면 true입니다.
         */
        bool IsInstanced() const;

        /**
         * @brief 필요한 GPU 자원을 생성하거나 변경된 상수를 업로드합니다.
         * @param[in] Device 자원 생성에 사용할 Direct3D 장치의 비소유 주소입니다.
         * @param[out] Error 실패 시 원인을 받을 문자열입니다.
         * @return 성공하면 true, 실패하면 false입니다.
         */
        bool CreateResources(ID3D11Device* Device, Core::FString& Error);

        /**
         * @brief 드로에 사용할 셰이더·입력 레이아웃·래스터 상태를 바인딩합니다.
         * @param[in] Context 파이프라인을 바인딩할 Direct3D 11 장치 컨텍스트입니다.
         */
        void Bind(ID3D11DeviceContext* Context) const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 입력 레이아웃·래스터 상태를 결정한 메시 패스입니다.
         */
        Renderer::EMeshPass mPass{};
        ReflectProperty(mPass, "Pass", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 컴파일된 정점 셰이더입니다.
         */
        std::shared_ptr<const RenderCore::FShader> mVertexCode{};
        ReflectProperty(mVertexCode, "VertexCode", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 컴파일된 픽셀 셰이더이며, 깊이 전용 패스는 비어 있습니다.
         */
        std::shared_ptr<const RenderCore::FShader> mPixelCode{};
        ReflectProperty(mPixelCode, "PixelCode", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 메시 양면 렌더링 여부입니다.
         */
        bool mTwoSided{};
        ReflectProperty(mTwoSided, "TwoSided", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 인스턴스 월드 행렬을 사용하는 메시 파이프라인인지 나타냅니다.
         */
        bool mInstanced{};
        ReflectProperty(mInstanced, "Instanced", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief GPU 생성에 사용한 정점 셰이더 개정 번호입니다.
         */
        Core::Uint64 mVertexRevision{};
        ReflectProperty(mVertexRevision, "VertexRevision", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief GPU 생성에 사용한 픽셀 셰이더 개정 번호입니다.
         */
        Core::Uint64 mPixelRevision{};
        ReflectProperty(mPixelRevision, "PixelRevision", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 메시용 정점 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader{};
        ReflectProperty(mVertexShader, "VertexShader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 메시용 픽셀 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader{};
        ReflectProperty(mPixelShader, "PixelShader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 메시 정점 입력 레이아웃입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout{};
        ReflectProperty(mInputLayout, "InputLayout", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 면 채우기와 뒷면 제거를 제어하는 래스터라이저입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizer{};
        ReflectProperty(mRasterizer, "Rasterizer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
