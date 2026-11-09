/**
 * @file UTexture.h
 * @brief 텍스처 크기와 픽셀 형식의 공통 인터페이스에 대한 선언입니다.
 * @ingroup Asset
 */

#pragma once

#include "CoreUObject/UObject.h"
#include "Asset/ETextureFormat.h"
#include "UTexture.generated.h"

namespace Asset {

    /**
     * @brief 텍스처 크기와 픽셀 형식의 공통 인터페이스를 담당합니다.
     */
    UCLASS(Abstract)
    class UTexture : public CoreUObject::UObject {
        GENERATED_BODY()

    public:
        /**
         * @brief UTexture의 초기 상태를 구성합니다.
         */
        UTexture();

        /**
         * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
         */
        ~UTexture() override;

    public:
        /**
         * @brief 텍스처의 가로 픽셀 수를 반환합니다.
         * @return 텍스처의 가로 픽셀 수를 나타내는 정수 값입니다.
         */
        virtual Core::Uint32 GetSizeX() const = 0;

        /**
         * @brief 텍스처의 세로 픽셀 수를 반환합니다.
         * @return 텍스처의 세로 픽셀 수를 나타내는 정수 값입니다.
         */
        virtual Core::Uint32 GetSizeY() const = 0;

        /**
         * @brief 텍스처의 픽셀 또는 깊이 형식을 반환합니다.
         * @return 텍스처 형식입니다.
         */
        virtual Asset::ETextureFormat GetFormat() const = 0;

        /**
         * @brief 텍스처를 sRGB 색 공간으로 해석하는지 반환합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        virtual bool IsSRGB() const = 0;

        /**
         * @brief 구현 텍스처가 사용할 수 있는 크기와 이미지 구성을 갖추었는지 검사합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        virtual bool IsValid() const = 0;

    };

}
