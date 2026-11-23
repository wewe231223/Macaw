/**
 * @file FRenderTargetPool.h
 * @brief 뷰 렌더링 동안만 쓰는 렌더 타깃을 재사용하는 풀을 정의합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Asset/ETextureFormat.h"
#include "Asset/ETextureUsage.h"
#include "Asset/UTexture2D.h"
#include "Core/Types.h"

#include <memory>

#include <d3d11.h>
#include <wrl/client.h>

namespace Renderer {

    class FRenderTargetPool;

    /**
     * @brief 풀에서 같은 텍스처를 찾을 때 비교하는 크기·형식·용도입니다.
     * @ingroup Renderer
     */
    struct FRenderTargetDesc {
    public:

        /**
         * @brief 두 설명이 같은 텍스처로 대체 가능한지 비교합니다.
         * @param[in] Other 비교할 설명입니다.
         * @return 크기·형식·용도가 모두 같으면 true입니다.
         */
        bool operator==(const Renderer::FRenderTargetDesc& Other) const = default;

    public:

        /**
         * @brief 가로 픽셀 수입니다.
         */
        Uint32 mWidth{};

        /**
         * @brief 세로 픽셀 수입니다.
         */
        Uint32 mHeight{};

        /**
         * @brief 텍스처 형식입니다.
         */
        Asset::ETextureFormat mFormat{Asset::ETextureFormat::RGBA8};

        /**
         * @brief 셰이더 리소스·렌더 타깃·깊이 스텐실 용도의 조합입니다.
         */
        Asset::ETextureUsage mUsage{Asset::ETextureUsage::ShaderResource};
    };

    /**
     * @brief 풀이 소유하는 렌더 타깃 하나와 깊이 형식의 보조 뷰입니다.
     * @ingroup Renderer
     */
    class FPooledRenderTarget {
    public:

        /**
         * @brief 풀이 소유하는 텍스처를 반환합니다.
         * @return 텍스처의 비소유 주소입니다.
         */
        Asset::UTexture2D* GetTexture() const;

        /**
         * @brief 깊이 쓰기가 비활성화된 깊이·스텐실 뷰를 반환합니다.
         * @return 깊이·스텐실 형식이 아니면 nullptr입니다.
         */
        ID3D11DepthStencilView* GetReadOnlyDepthView() const;

        /**
         * @brief 스텐실 비트를 읽는 셰이더 리소스 뷰를 반환합니다.
         * @return 깊이·스텐실 형식이 아니면 nullptr입니다.
         */
        ID3D11ShaderResourceView* GetStencilResource() const;

    private:

        /**
         * @brief 생성·대여 상태는 풀만 바꿉니다.
         */
        friend class Renderer::FRenderTargetPool;

        /**
         * @brief 생성할 때 사용한 설명입니다.
         */
        Renderer::FRenderTargetDesc mDesc{};

        /**
         * @brief 소유한 텍스처입니다.
         */
        std::unique_ptr<Asset::UTexture2D> mTexture{};

        /**
         * @brief D24S8 깊이 텍스처의 읽기 전용 깊이·스텐실 뷰입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mReadOnlyDepthView{};

        /**
         * @brief D24S8 깊이 텍스처의 스텐실 SRV입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mStencilResource{};

        /**
         * @brief 지금 대여 중이면 true입니다.
         */
        bool mInUse{};

        /**
         * @brief 마지막으로 대여된 풀 프레임 번호입니다.
         */
        Uint64 mLastUsedFrame{};
    };

    /**
     * @brief 대여한 렌더 타깃을 소멸할 때 풀에 돌려주는 이동 전용 핸들입니다.
     * @ingroup Renderer
     */
    class FRenderTargetLease {
    public:

        /**
         * @brief 아무것도 빌리지 않은 빈 핸들을 만듭니다.
         */
        FRenderTargetLease() = default;

        /**
         * @brief 대여한 렌더 타깃을 풀에 돌려줍니다.
         */
        ~FRenderTargetLease();

        /**
         * @brief 대여는 하나의 핸들만 소유하므로 복사를 금지합니다.
         */
        FRenderTargetLease(const Renderer::FRenderTargetLease&) = delete;

        /**
         * @brief 대여는 하나의 핸들만 소유하므로 복사 대입을 금지합니다.
         * @return 자기 자신의 참조입니다.
         */
        Renderer::FRenderTargetLease& operator=(const Renderer::FRenderTargetLease&) = delete;

        /**
         * @brief 다른 핸들의 대여를 넘겨받고 원본을 비웁니다.
         * @param[in,out] Other 넘겨줄 핸들입니다.
         */
        FRenderTargetLease(Renderer::FRenderTargetLease&& Other) noexcept;

        /**
         * @brief 현재 대여를 돌려준 뒤 다른 핸들의 대여를 넘겨받습니다.
         * @param[in,out] Other 넘겨줄 핸들입니다.
         * @return 자기 자신의 참조입니다.
         */
        Renderer::FRenderTargetLease& operator=(Renderer::FRenderTargetLease&& Other) noexcept;

    public:

        /**
         * @brief 대여한 렌더 타깃을 반환합니다.
         * @return 빈 핸들이면 nullptr입니다.
         */
        Renderer::FPooledRenderTarget* Get() const;

        /**
         * @brief 대여한 렌더 타깃의 텍스처를 반환합니다.
         * @return 빈 핸들이면 nullptr입니다.
         */
        Asset::UTexture2D* GetTexture() const;

        /**
         * @brief 대여 중인지 확인합니다.
         * @return 렌더 타깃을 빌렸으면 true입니다.
         */
        explicit operator bool() const;

        /**
         * @brief 대여를 즉시 풀에 돌려주고 핸들을 비웁니다.
         */
        void Reset();

    private:

        /**
         * @brief 풀만 대여 핸들을 만듭니다.
         */
        friend class Renderer::FRenderTargetPool;

        /**
         * @brief 풀과 대여한 렌더 타깃으로 핸들을 만듭니다.
         * @param[in] Pool 돌려줄 풀입니다.
         * @param[in] Target 대여한 렌더 타깃입니다.
         */
        FRenderTargetLease(Renderer::FRenderTargetPool* Pool, Renderer::FPooledRenderTarget* Target);

        /**
         * @brief 돌려줄 풀의 비소유 주소입니다.
         */
        Renderer::FRenderTargetPool* mPool{};

        /**
         * @brief 대여한 렌더 타깃의 비소유 주소입니다.
         */
        Renderer::FPooledRenderTarget* mTarget{};
    };

    /**
     * @brief 뷰 렌더링 동안만 쓰는 렌더 타깃을 설명별로 재사용하고 오래 쓰지 않은 것을 해제합니다.
     * @ingroup Renderer
     */
    class FRenderTargetPool {
    public:

        /**
         * @brief 대여되지 않은 채 이 프레임 수를 넘긴 렌더 타깃을 해제합니다.
         */
        static constexpr Uint64 MaxIdleFrames{30};

        /**
         * @brief 빈 풀을 만듭니다.
         */
        FRenderTargetPool() = default;

        /**
         * @brief 모든 렌더 타깃을 해제합니다.
         */
        ~FRenderTargetPool();

        /**
         * @brief 풀이 대여 핸들의 대상이므로 복사를 금지합니다.
         */
        FRenderTargetPool(const Renderer::FRenderTargetPool&) = delete;

        /**
         * @brief 풀이 대여 핸들의 대상이므로 대입을 금지합니다.
         * @return 자기 자신의 참조입니다.
         */
        Renderer::FRenderTargetPool& operator=(const Renderer::FRenderTargetPool&) = delete;

    public:

        /**
         * @brief 설명과 같고 대여되지 않은 렌더 타깃을 빌리며, 없으면 새로 만듭니다.
         * @param[in] Device 새 텍스처를 만들 장치입니다.
         * @param[in] Desc 필요한 크기·형식·용도입니다.
         * @return 대여 핸들이며 생성에 실패하면 빈 핸들입니다.
         * @note D24S8 깊이 텍스처는 읽기 전용 깊이 뷰와 스텐실 SRV를 함께 만듭니다.
         */
        Renderer::FRenderTargetLease Acquire(ID3D11Device* Device, const Renderer::FRenderTargetDesc& Desc);

        /**
         * @brief 풀 프레임을 하나 진행하고 오래 대여되지 않은 렌더 타깃을 해제합니다.
         */
        void Tick();

        /**
         * @brief 대여되지 않은 렌더 타깃을 모두 해제합니다.
         */
        void Clear();

        /**
         * @brief 풀이 보유한 GPU 텍스처의 추정 바이트 수를 반환합니다.
         * @return 대여 여부와 무관한 전체 바이트 수입니다.
         */
        std::size_t GetResourceBytes() const;

    private:

        /**
         * @brief 대여 핸들이 소멸할 때 렌더 타깃을 돌려받습니다.
         */
        friend class Renderer::FRenderTargetLease;

        /**
         * @brief 렌더 타깃을 다시 대여 가능 상태로 돌립니다.
         * @param[in] Target 돌려받을 렌더 타깃입니다.
         */
        void Release(Renderer::FPooledRenderTarget* Target);

        /**
         * @brief 대여 핸들이 가리키는 주소가 바뀌지 않도록 개별 할당한 렌더 타깃 목록입니다.
         */
        Core::TArray<std::unique_ptr<Renderer::FPooledRenderTarget>> mTargets{};

        /**
         * @brief Tick마다 증가하는 풀 프레임 번호입니다.
         */
        Uint64 mFrame{};
    };

}
