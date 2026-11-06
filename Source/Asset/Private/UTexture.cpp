/**
 * @file UTexture.cpp
 * @brief 텍스처 크기와 픽셀 형식의 공통 인터페이스에 대한 구현입니다.
 * @ingroup Asset
 */

#include "Asset/UTexture.h"

namespace Asset {

    /**
     * @brief UTexture의 초기 상태를 구성합니다.
     */
    Asset::UTexture::UTexture() = default;

    /**
     * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
     */
    Asset::UTexture::~UTexture() = default;

}
