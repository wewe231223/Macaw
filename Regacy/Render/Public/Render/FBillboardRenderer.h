/**
 * @file FBillboardRenderer.h
 * @brief 전역 에셋 조회를 사용하는 빌보드 렌더러를 정의합니다.
 */

#pragma once
#include "Render/FRenderAssetResources.h"
#include <d3d11.h>
#include <wrl/client.h>
#include "Math/FMath.h"
#include "Core/STL.h"
#include "RenderCore/FRenderData.h"

class FRenderBuffers;

enum class ERenderMode : std::size_t;
class UPipeline;
class UTexture;

struct FBillboardData {
    FMatrix mWorld{};
    FVector2 mSize{};
    FVector2 mUvMin{};
    FVector2 mUvMax{};
    FVector2 mPad{};
    FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
};

class FBillboardRenderer {
private:
    struct FBatchKey {
        FAssetHandle mPipelineHandle{};
        FAssetHandle mTextureHandle{};

        bool operator==(const FBatchKey& Other) const;
    };

    struct FBatchKeyHash {
        std::size_t operator()(const FBatchKey& Key) const noexcept;
    };

    struct FBillboardDraw {
        const UPipeline* mPipeline{};
        const UTexture* mTexture{};
        Uint32 mFirstInstance{};
        Uint32 mInstanceCount{};
    };

    struct FBillboardBatch {
        FBillboardDraw mDraw{};
        Uint32 mWriteCount{};
    };

    static_assert(sizeof(FBillboardData) == 112);

public:
    FBillboardRenderer() = default;
    ~FBillboardRenderer() = default;

public:
    bool Initialize(ID3D11Device* InDevice, std::uint32_t InitialCapacity = 256);

    /**
     * @brief 전역 텍스처와 파이프라인으로 빌보드를 렌더링합니다.
     * @param Context 그래픽 명령 컨텍스트입니다.
     * @param RenderBuffers 이 렌더러의 버퍼 저장소입니다.
     * @param BillboardDraws 렌더링할 월드별 빌보드 목록입니다.
     * @param Resources 장치에 귀속된 GPU 에셋 캐시입니다.
     * @param Mode 사용할 렌더 모드입니다.
     * @note 에셋 서비스가 없으면 아무것도 그리지 않습니다.
     */
    void Render(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers, const TArray<FBillboardDrawData>& BillboardDraws, FRenderAssetResources& Resources, ERenderMode Mode);

private:
    ID3D11Device* mDevice{nullptr};
    TArray<FBillboardData> mInstances{};
    TArray<FBillboardDraw> mDraws{};
};
