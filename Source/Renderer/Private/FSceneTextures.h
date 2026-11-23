/**
 * @file FSceneTextures.h
 * @brief 뷰 하나를 그리는 동안 풀에서 빌린 장면 텍스처 묶음을 정의합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Asset/UTexture2D.h"
#include "FRenderTargetPool.h"
#include "GBuffer.h"
#include "Core/Types.h"

#include <d3d11.h>

namespace Renderer {

    /**
     * @brief 뷰 하나를 그리는 동안 풀에서 빌린 장면 컬러·깊이·GBuffer와 뷰의 최종 출력 컬러입니다.
     * @details 소멸하면 빌린 텍스처를 모두 풀에 돌려주므로 뷰 렌더링 함수의 지역 변수로만 사용합니다.
     * @ingroup Renderer
     */
    class FSceneTextures {
    public:

        /**
         * @brief 뷰 크기의 장면 텍스처를 풀에서 빌립니다.
         * @param[in,out] Pool 텍스처를 빌릴 풀입니다.
         * @param[in] Device 풀에 없는 텍스처를 만들 장치입니다.
         * @param[in] OutputColor 후처리 결과와 에디터 요소를 그릴 뷰 소유 컬러 텍스처입니다.
         * @param[in] Deferred GBuffer를 빌리려면 true입니다.
         */
        FSceneTextures(Renderer::FRenderTargetPool& Pool, ID3D11Device* Device, Asset::UTexture2D& OutputColor, bool Deferred);

        /**
         * @brief 대여는 이 객체만 소유하므로 복사를 금지합니다.
         */
        FSceneTextures(const Renderer::FSceneTextures&) = delete;

        /**
         * @brief 대여는 이 객체만 소유하므로 대입을 금지합니다.
         * @return 자기 자신의 참조입니다.
         */
        Renderer::FSceneTextures& operator=(const Renderer::FSceneTextures&) = delete;

    public:

        /**
         * @brief 필요한 텍스처를 모두 빌렸는지 확인합니다.
         * @return 하나라도 만들지 못했으면 false입니다.
         */
        bool IsValid() const;

        /**
         * @brief 가로 픽셀 수를 반환합니다.
         * @return 출력 컬러의 가로 픽셀 수입니다.
         */
        Uint32 GetWidth() const;

        /**
         * @brief 세로 픽셀 수를 반환합니다.
         * @return 출력 컬러의 세로 픽셀 수입니다.
         */
        Uint32 GetHeight() const;

        /**
         * @brief 장면 드로와 교대 후처리 입력으로 사용하는 컬러 텍스처를 반환합니다.
         * @return 풀에서 빌린 텍스처의 비소유 주소입니다.
         */
        Asset::UTexture2D* GetSceneColor() const;

        /**
         * @brief 다중 안개 합성에 사용하는 교대 컬러 텍스처를 반환합니다.
         * @return 풀에서 빌린 텍스처의 비소유 주소입니다.
         */
        Asset::UTexture2D* GetPostProcessColor() const;

        /**
         * @brief 에디터와 UI에 표시하는 뷰 소유 최종 컬러 텍스처를 반환합니다.
         * @return 렌더 타깃이 소유하는 텍스처의 비소유 주소입니다.
         */
        Asset::UTexture2D* GetOutputColor() const;

        /**
         * @brief 깊이 검사와 후처리 샘플링에 공유하는 깊이 텍스처를 반환합니다.
         * @return 풀에서 빌린 텍스처의 비소유 주소입니다.
         */
        Asset::UTexture2D* GetSceneDepth() const;

        /**
         * @brief 깊이·스텐실 쓰기 뷰를 반환합니다.
         * @return 비소유 D3D11 뷰입니다.
         */
        ID3D11DepthStencilView* GetDepthView() const;

        /**
         * @brief 깊이 쓰기가 비활성화된 깊이·스텐실 뷰를 반환합니다.
         * @return 비소유 D3D11 뷰입니다.
         */
        ID3D11DepthStencilView* GetReadOnlyDepthView() const;

        /**
         * @brief 표면 분류 비트를 읽는 스텐실 SRV를 반환합니다.
         * @return 비소유 D3D11 리소스 뷰입니다.
         */
        ID3D11ShaderResourceView* GetStencilResource() const;

        /**
         * @brief GBuffer를 빌렸는지 확인합니다.
         * @return 디퍼드 셰이딩으로 만들었으면 true입니다.
         */
        bool HasGBuffer() const;

        /**
         * @brief GBuffer 첨부 텍스처를 반환합니다.
         * @param[in] Index GBuffer 첨부 번호입니다.
         * @return 풀에서 빌린 텍스처의 비소유 주소이며 GBuffer가 없으면 nullptr입니다.
         */
        Asset::UTexture2D* GetGBuffer(Index_t Index) const;

        /**
         * @brief 데칼 노멀 혼합에 필요한 원본 또는 누적 노멀 복사본을 처음 요청할 때 풀에서 빌립니다.
         * @param[in] Accumulated 이전 데칼까지 누적된 노멀 복사본을 요청하면 true입니다.
         * @return 풀에서 빌린 텍스처의 비소유 주소이며 GBuffer가 없거나 생성에 실패하면 nullptr입니다.
         */
        Asset::UTexture2D* GetDecalNormalSource(bool Accumulated);

    private:

        /**
         * @brief 추가 대여에 사용할 풀의 비소유 주소입니다.
         */
        Renderer::FRenderTargetPool* mPool{};

        /**
         * @brief 추가 대여에서 새 텍스처를 만들 장치입니다.
         */
        ID3D11Device* mDevice{};

        /**
         * @brief 렌더 타깃이 소유하는 최종 출력 컬러입니다.
         */
        Asset::UTexture2D* mOutputColor{};

        /**
         * @brief 장면 컬러 대여입니다.
         */
        Renderer::FRenderTargetLease mSceneColor{};

        /**
         * @brief 안개 교대 컬러 대여입니다.
         */
        Renderer::FRenderTargetLease mPostProcessColor{};

        /**
         * @brief 깊이·스텐실 대여입니다.
         */
        Renderer::FRenderTargetLease mDepth{};

        /**
         * @brief GBuffer 첨부 대여입니다. 포워드 셰이딩이면 비어 있습니다.
         */
        Core::TStaticArray<Renderer::FRenderTargetLease, Renderer::GBuffer::AttachmentCount> mGBuffer{};

        /**
         * @brief 데칼 적용 전 노멀 복사본 대여입니다.
         */
        Renderer::FRenderTargetLease mDecalNormalSource{};

        /**
         * @brief 겹친 데칼의 누적 노멀 복사본 대여입니다.
         */
        Renderer::FRenderTargetLease mDecalNormalHistory{};

        /**
         * @brief GBuffer를 빌렸으면 true입니다.
         */
        bool mDeferred{};
    };

}
