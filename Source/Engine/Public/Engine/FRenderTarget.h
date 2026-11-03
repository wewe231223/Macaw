/**
 * @file FRenderTarget.h
 * @brief 렌더 타깃의 크기와 UI 표시용 네이티브 리소스를 노출합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

namespace Engine {

    /**
     * @brief 렌더 타깃의 크기와 UI 표시용 네이티브 리소스를 노출합니다.
     * @ingroup Engine
     */
    class FRenderTarget {
    public:
        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        virtual ~FRenderTarget();

    public:
        /**
         * @brief 가로 픽셀 수를 반환합니다.
         * @return 가로 픽셀 수입니다.
         */
        virtual Core::Uint32 GetWidth() const = 0;

        /**
         * @brief 세로 픽셀 수를 반환합니다.
         * @return 세로 픽셀 수입니다.
         */
        virtual Core::Uint32 GetHeight() const = 0;

        /**
         * @brief UI에서 샘플링할 셰이더 리소스 뷰의 비소유 주소를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        virtual void* GetNativeTexture() const = 0;
    };

}
