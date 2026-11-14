/**
 * @file FShadowAtlasView.h
 * @brief 그림자 아틀라스의 미리보기·깊이 텍스처와 타일 정보를 전달합니다.
 * @ingroup RenderCore
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include "Core/Guid.h"
#include "Core/Math/FVector4.h"

namespace RenderCore {

    /**
     * @brief 광원별 그림자 면 또는 캐스케이드의 아틀라스 배치 정보입니다.
     * @ingroup RenderCore
     */
    struct FShadowAtlasTile {
    public:
        /**
         * @brief 이 타일을 사용하는 광원의 GUID입니다.
         */
        Core::FGuid mLightId{};

        /**
         * @brief 보호 여백을 포함한 타일의 아틀라스 UV 시작점과 크기입니다.
         */
        Core::FVector4 mRectangle{};

        /**
         * @brief 점광원 큐브 면 또는 방향광 캐스케이드의 0 기반 번호입니다.
         */
        Core::Index_t mFace{};

        /**
         * @brief 보호 여백을 제외한 그림자 맵 한 변의 유효 픽셀 수입니다.
         */
        Core::Uint32 mResolution{};
    };

    /**
     * @brief 그림자 아틀라스의 미리보기·깊이 텍스처와 타일 정보를 전달합니다.
     * @ingroup RenderCore
     */
    struct FShadowAtlasView {
    public:
        /**
         * @brief 색상 미리보기 셰이더 리소스 뷰의 비소유 주소입니다.
         */
        void* mTexture{};

        /**
         * @brief 아틀라스 텍스처 한 변의 픽셀 수입니다.
         */
        Core::Uint32 mSize{};

        /**
         * @brief 현재 아틀라스에서 광원별 타일을 식별할 배치 정보 사본입니다.
         */
        Core::TArray<RenderCore::FShadowAtlasTile> mTiles{};

        /**
         * @brief 깊이 아틀라스 셰이더 리소스 뷰의 비소유 주소입니다.
         */
        void* mDepthTexture{};
    };

}
