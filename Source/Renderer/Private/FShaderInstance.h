/**
 * @file FShaderInstance.h
 * @brief 공유 코드에서 패스·인스턴싱·스키닝 변형의 D3D11 셰이더 자원을 관리합니다.
 * @ingroup Renderer
 */

#pragma once

#include "EMeshPass.h"
#include "RenderCore/Shader/FShader.h"

#include <d3d11.h>
#include <wrl/client.h>

namespace Renderer {

    /**
     * @brief 공유 코드에서 패스·인스턴싱·스키닝 변형의 D3D11 셰이더 자원을 관리합니다.
     */
    class FShaderInstance {
    public:
        /**
         * @brief 공유 셰이더 코드와 패스·인스턴싱·스키닝 변형을 연결합니다.
         * @param[in] Code 컴파일된 셰이더 코드 객체입니다.
         * @param[in] Pass 변형·입력 레이아웃을 고를 메시 패스입니다.
         * @param[in] Instanced 인스턴스 정점 스트림을 사용하면 true입니다.
         * @param[in] Skinned 본 인덱스·가중치 스트림을 사용하면 true입니다.
         */
        FShaderInstance(std::shared_ptr<const RenderCore::FShader> Code, Renderer::EMeshPass Pass, bool Instanced, bool Skinned);

    public:
        /**
         * @brief GPU 자원이 존재하고 코드의 최신 리비전을 반영했는지 검사합니다.
         * @return GPU 자원이 코드 리비전과 일치하면 true입니다.
         */
        bool IsCurrent() const;

        /**
         * @brief 셰이더 단계에 필요한 GPU 셰이더와 입력 레이아웃이 있는지 검사합니다.
         * @return 필요한 GPU 자원이 모두 있으면 true입니다.
         */
        bool HasResources() const;

        /**
         * @brief GPU 자원에 반영한 셰이더 코드 리비전을 반환합니다.
         * @return GPU 자원에 반영한 셰이더 코드 리비전입니다.
         */
        Core::Uint64 GetRevision() const;

        /**
         * @brief 최신 코드로 D3D11 셰이더와 필요한 정점 입력 레이아웃을 생성합니다.
         * @param[in] Device GPU 자원을 생성할 D3D11 장치입니다.
         * @param[out] Error 실패 원인을 받을 문자열입니다.
         * @return GPU 셰이더·입력 레이아웃을 준비했으면 true입니다.
         */
        bool CreateResources(ID3D11Device* Device, Core::FString& Error);

        /**
         * @brief GPU 셰이더와 필요한 정점 입력 레이아웃을 연결합니다.
         * @param[in] Context GPU 명령을 제출할 D3D11 장치 컨텍스트입니다.
         */
        void Bind(ID3D11DeviceContext* Context) const;

    private:
        /**
         * @brief GPU 자원을 만들 공유 셰이더 코드입니다.
         */
        std::shared_ptr<const RenderCore::FShader> mCode{};

        /**
         * @brief 입력 레이아웃과 변형을 결정할 메시 패스입니다.
         */
        Renderer::EMeshPass mPass{};

        /**
         * @brief 인스턴스 정점 스트림 사용 여부입니다.
         */
        bool mInstanced{};

        /**
         * @brief 본 인덱스·가중치 스트림 사용 여부입니다.
         */
        bool mSkinned{};

        /**
         * @brief GPU 자원에 반영한 코드 리비전입니다.
         */
        Core::Uint64 mRevision{};

        /**
         * @brief 바이트코드로 생성한 정점 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader{};

        /**
         * @brief 바이트코드로 생성한 픽셀 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader{};

        /**
         * @brief 정점·인스턴스·스킨 데이터 입력 레이아웃입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout{};
    };

}
