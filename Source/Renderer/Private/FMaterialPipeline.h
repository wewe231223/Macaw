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

    class FShaderInstance;

    /**
     * @brief 머티리얼 셰이더와 양면 정책별 GPU 파이프라인을 관리합니다.
     */
    class FMaterialPipeline {
    public:
        /**
         * @brief 전달된 설정과 기본값으로 FMaterialPipeline 객체를 생성합니다.
         * @param[in] Pass 입력 레이아웃·래스터 상태를 결정하는 메시 패스입니다.
         * @param[in] VertexShader 공유 정점 셰이더와 입력 레이아웃입니다.
         * @param[in] PixelShader 공유 픽셀 셰이더이며, 깊이 전용 패스는 nullptr입니다.
         * @param[in] TwoSided 양면 렌더링을 사용할지 지정합니다.
         * @param[in] Instanced 메시 인스턴스의 월드 행렬을 정점 입력으로 사용하려면 true입니다.
         * @param[in] Skinned 본 인덱스·가중치 입력을 사용하는 스키닝 파이프라인이면 true입니다.
         */
        FMaterialPipeline(Renderer::EMeshPass Pass, std::shared_ptr<Renderer::FShaderInstance> VertexShader, std::shared_ptr<Renderer::FShaderInstance> PixelShader, bool TwoSided, bool Instanced = false, bool Skinned = false, bool WritesGBuffer = false);

        /**
         * @brief 기본 소멸자이며, GPU 자원은 ComPtr 멤버가 해제합니다.
         */
        ~FMaterialPipeline();

    public:
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
         * @brief 객체별 본 행렬을 사용하는지 반환합니다.
         * @return 스키닝 입력을 사용하면 true입니다.
         */
        bool IsSkinned() const;
        bool WritesGBuffer() const;

        /**
         * @brief GPU 파이프라인을 만들 때 사용한 정점 셰이더의 리비전을 반환합니다.
         * @return 아직 만들지 않았으면 0, 그 외에는 생성에 사용한 정점 셰이더 리비전입니다.
         */
        Core::Uint64 GetVertexRevision() const;

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
         * @brief 입력 레이아웃·래스터 상태를 결정한 메시 패스입니다.
         */
        Renderer::EMeshPass mPass{};

        /**
         * @brief 스킨 정점 입력과 객체별 본 팔레트를 사용하는지 나타냅니다.
         */
        bool mSkinned{};

        /**
         * @brief 메시 양면 렌더링 여부입니다.
         */
        bool mTwoSided{};

        /**
         * @brief 인스턴스 월드 행렬을 사용하는 메시 파이프라인인지 나타냅니다.
         */
        bool mInstanced{};
        bool mWritesGBuffer{};

        /**
         * @brief 메시용 정점 셰이더입니다.
         */
        std::shared_ptr<Renderer::FShaderInstance> mVertexShader{};

        /**
         * @brief 메시용 픽셀 셰이더입니다.
         */
        std::shared_ptr<Renderer::FShaderInstance> mPixelShader{};

        /**
         * @brief 면 채우기와 뒷면 제거를 제어하는 래스터라이저입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizer{};
    };

}
