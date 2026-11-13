/**
 * @file RenderTarget.h
 * @brief Direct3D 11 오프스크린 렌더 타깃을 구현합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FRenderTarget.h"
#include "Asset/UTexture2D.h"
#include "GBuffer.h"

#include <array>
#include <d3d11.h>
#include <wrl/client.h>

namespace Renderer {

    /**
     * @brief 컬러 텍스처, 깊이 버퍼와 대응 뷰를 소유하는 Direct3D 11 렌더 타깃입니다.
     * @ingroup Renderer
     */
    class FDXRenderTarget final : public Engine::FRenderTarget {
    public:

        /**
         * @brief FDXRenderTarget의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        FDXRenderTarget();

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FDXRenderTarget() override;

    public:

        /**
         * @brief 새 크기에 맞추어 렌더 타깃 리소스를 생성하거나 교체합니다.
         * @param[in,out] Device 리소스 생성에 사용할 Direct3D 11 장치입니다.
         * @param[in] Width 렌더 타깃의 가로 픽셀 수입니다.
         * @param[in] Height 렌더 타깃의 세로 픽셀 수입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool Resize(ID3D11Device* Device, Core::Uint32 Width, Core::Uint32 Height);

        /**
         * @brief 가로 픽셀 수를 반환합니다.
         * @return 가로 픽셀 수입니다.
         */
        Core::Uint32 GetWidth() const override;

        /**
         * @brief 세로 픽셀 수를 반환합니다.
         * @return 세로 픽셀 수입니다.
         */
        Core::Uint32 GetHeight() const override;

        /**
         * @brief UI에서 샘플링할 셰이더 리소스 뷰의 비소유 주소를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        void* GetNativeTexture() const override;

        /**
         * @brief 컬러 출력용 Direct3D 11 렌더 타깃 뷰의 비소유 주소를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        ID3D11RenderTargetView* GetRenderTargetView() const;

        /**
         * @brief 깊이 스텐실 뷰를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        ID3D11DepthStencilView* GetDepthView() const;

        /**
         * @brief 깊이 쓰기가 비활성화된 깊이·스텐실 뷰를 반환합니다.
         * @return 타깃 소유의 비소유 D3D11 뷰 포인터입니다.
         */
        ID3D11DepthStencilView* GetReadOnlyDepthView() const;

        /**
         * @brief 표면 분류 비트를 읽는 스텐실 SRV를 반환합니다.
         * @return 타깃 소유의 비소유 D3D11 리소스 뷰입니다.
         */
        ID3D11ShaderResourceView* GetStencilResource() const;

        /**
         * @brief 표면 속성을 저장하는 GBuffer 텍스처를 반환합니다.
         * @return 타깃이 소유하는 텍스처 배열의 읽기 전용 참조입니다.
         */
        const std::array<std::unique_ptr<Asset::UTexture2D>, Renderer::GBuffer::AttachmentCount>& GetGBuffer() const;

        /**
         * @brief 데칼 노멀 혼합에 필요한 원본 또는 누적 노멀 복사본을 준비합니다.
         * @param Device 리소스를 생성할 D3D11 장치입니다.
         * @param Accumulated 이전 데칼까지 누적된 노멀을 읽을지 여부입니다.
         * @return 타깃이 소유하는 샘플링용 텍스처이며 준비 실패 시 nullptr입니다.
         */
        Asset::UTexture2D* GetDecalNormalSource(ID3D11Device* Device, bool Accumulated);

        /**
         * @brief 장면 드로와 교대 후처리 입력으로 사용하는 컬러 텍스처를 조회합니다.
         * @return 렌더 타깃이 소유하는 장면 컬러의 비소유 주소이며 할당 전에는 nullptr입니다.
         */
        Asset::UTexture2D* GetSceneColor() const;

        /**
         * @brief 다중 안개 합성에 사용하는 교대 컬러 텍스처를 조회합니다.
         * @return 렌더 타깃이 소유하는 중간 컬러의 비소유 주소이며 할당 전에는 nullptr입니다.
         */
        Asset::UTexture2D* GetPostProcessColor() const;

        /**
         * @brief 에디터와 UI에 표시하는 최종 컬러 텍스처를 조회합니다.
         * @return 렌더 타깃이 소유하는 최종 컬러의 비소유 주소이며 할당 전에는 nullptr입니다.
         */
        Asset::UTexture2D* GetOutputColor() const;

        /**
         * @brief 깊이 검사와 후처리 샘플링에 공유하는 깊이 텍스처를 조회합니다.
         * @return 렌더 타깃이 소유하는 깊이 텍스처의 비소유 주소이며 할당 전에는 nullptr입니다.
         */
        Asset::UTexture2D* GetSceneDepth() const;

        /**
         * @brief 현재 보유한 GPU 리소스의 추정 바이트 수를 반환합니다.
         * @return 현재 보유한 GPU 리소스의 추정 바이트 수를 나타내는 정수 값입니다.
         */
        std::size_t GetResourceBytes() const;

    private:

        /**
         * @brief UI 표시용 셰이더 리소스 뷰가 참조하는 컬러 텍스처입니다.
         */

        /**
         * @brief 컬러 렌더 타깃 뷰입니다.
         */

        /**
         * @brief UI 샘플링에 사용할 셰이더 리소스 뷰입니다.
         */
        std::unique_ptr<Asset::UTexture2D> mColor{};

        /**
         * @brief 후처리 입력 컬러와 다중 안개 합성의 교대 출력으로 사용하는 텍스처입니다.
         */
        std::unique_ptr<Asset::UTexture2D> mSceneColor{};

        /**
         * @brief 다중 안개를 읽기·쓰기 충돌 없이 합성하기 위한 교대 컬러 텍스처입니다.
         */
        std::unique_ptr<Asset::UTexture2D> mPostProcessColor{};

        /**
         * @brief 깊이 텍스처입니다.
         */

        /**
         * @brief 깊이 스텐실 뷰입니다.
         */
        std::unique_ptr<Asset::UTexture2D> mDepth{};

        /**
         * @brief 깊이를 읽으면서 셰이더에서도 샘플링할 수 있는 깊이 뷰입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mReadOnlyDepthView{};

        /**
         * @brief 프리미티브 분류 비트를 셰이더에서 읽는 스텐실 SRV입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mStencilResource{};

        /**
         * @brief 지연 렌더링의 표면 속성 텍스처를 소유합니다.
         */
        std::array<std::unique_ptr<Asset::UTexture2D>, Renderer::GBuffer::AttachmentCount> mGBuffer{};

        /**
         * @brief 데칼 적용 전 노멀을 읽기 위해 소유한 복사 텍스처입니다.
         */
        std::unique_ptr<Asset::UTexture2D> mDecalNormalSource{};

        /**
         * @brief 겹친 데칼의 누적 노멀을 읽기 위해 소유한 중간 텍스처입니다.
         */
        std::unique_ptr<Asset::UTexture2D> mDecalNormalHistory{};

        /**
         * @brief 가로 픽셀 수입니다.
         */
        Core::Uint32 mWidth{};

        /**
         * @brief 세로 픽셀 수입니다.
         */
        Core::Uint32 mHeight{};
    };

}
