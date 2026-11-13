/**
 * @file FViewerCapture.h
 * @brief 뷰어 렌더 타깃과 독립된 최종 컬러 텍스처의 수명을 소유합니다.
 * @ingroup Viewer
 */

#pragma once

#include "Core/Types.h"

#include <memory>

namespace Asset {
    class UTexture2D;
}

namespace Viewer {

    /**
     * @brief 뷰어 렌더 타깃과 독립된 최종 컬러 텍스처의 수명을 소유합니다.
     * @ingroup Viewer
     */
    class FViewerCapture {
    public:

        /**
         * @brief 독립 촬영 텍스처의 소유권을 받아 캡처 객체를 구성합니다.
         * @param Texture 캡처로 소유권을 이전할 최종 컬러 텍스처입니다.
         */
        explicit FViewerCapture(std::unique_ptr<Asset::UTexture2D> Texture);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FViewerCapture();

    public:

        /**
         * @brief UI에서 표시할 네이티브 텍스처 핸들을 반환합니다.
         * @return 소유권을 이전하지 않는 SRV 핸들이며 표시할 이미지가 없으면 nullptr입니다.
         */
        void* GetNativeTexture() const;

        /**
         * @brief 캡처 이미지의 너비를 반환합니다.
         * @return 너비(픽셀)이며 이미지가 없으면 0입니다.
         */
        Core::Uint32 GetWidth() const;

        /**
         * @brief 캡처 이미지의 높이를 반환합니다.
         * @return 높이(픽셀)이며 이미지가 없으면 0입니다.
         */
        Core::Uint32 GetHeight() const;

        /**
         * @brief 소유한 캡처 텍스처의 GPU 리소스 크기를 반환합니다.
         * @return 추정 GPU 메모리 사용량(바이트)입니다.
         */
        std::size_t GetResourceBytes() const;

        /**
         * @brief 캡처가 소유하는 최종 컬러 텍스처를 조회합니다.
         * @return 캡처 수명에 종속되는 비소유 읽기 전용 포인터입니다.
         */
        const Asset::UTexture2D* GetTexture() const;

    private:

        /**
         * @brief 렌더 타깃에서 분리한 최종 컬러 텍스처의 소유 포인터입니다.
         */
        std::unique_ptr<Asset::UTexture2D> mTexture{};
    };
}
