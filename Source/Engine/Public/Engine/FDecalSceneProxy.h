/**
 * @file FDecalSceneProxy.h
 * @brief 렌더러에 전달하는 데칼 식별자, 투영 행렬과 비소유 텍스처 참조입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Guid.h"
#include "Core/Math/FMatrix.h"

namespace Asset {
    class UTexture2D;
}

namespace Engine {

    /**
     * @brief 렌더러에 전달하는 데칼 식별자, 투영 행렬과 비소유 텍스처 참조입니다.
     * @ingroup Engine
     */
    struct FDecalSceneProxy {

        /**
         * @brief 소속 데이터 안에서 항목을 식별하는 ID입니다.
         */
        Core::FGuid mId{};

        /**
         * @brief 정규화된 데칼 박스 좌표를 월드 공간으로 변환하는 행렬입니다.
         */
        Core::FMatrix mDecalToWorld{};

        /**
         * @brief 월드 좌표를 데칼 투영 박스 좌표로 변환하는 역행렬입니다.
         */
        Core::FMatrix mWorldToDecal{};

        /**
         * @brief 에셋 관리자가 소유하는 데칼 색상 텍스처입니다.
         */
        Asset::UTexture2D* mTexture{};

        /**
         * @brief 데칼 불투명도이며 범위는 0~1입니다.
         */
        float mOpacity{1.0f};

        /**
         * @brief 겹치는 데칼을 정렬할 순서 값입니다.
         */
        Core::Int32 mSortOrder{};

        /**
         * @brief 에셋 관리자가 소유하는 데칼 노멀 텍스처입니다.
         */
        Asset::UTexture2D* mNormalMap{};

        /**
         * @brief 데칼 노멀 혼합 강도이며 허용 범위는 0~8입니다.
         */
        float mNormalStrength{1.0f};
    };

}
