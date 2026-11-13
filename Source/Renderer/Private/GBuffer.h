/**
 * @file GBuffer.h
 * @brief 표면 속성 첨부와 스텐실 분류 비트를 정의합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/Types.h"

/**
 * @brief GBuffer 첨부 인덱스와 표면 분류 스텐실 비트입니다.
 */
namespace Renderer::GBuffer {

    /**
     * @brief GBuffer의 컬러 첨부 텍스처 개수입니다.
     */
    inline constexpr std::size_t AttachmentCount{6};

    /**
     * @brief 월드 공간 노멀을 저장하는 GBuffer 첨부 인덱스입니다.
     */
    inline constexpr std::size_t NormalAttachment{1};

    /**
     * @brief 데칼을 받을 표면을 식별하는 스텐실 비트입니다.
     */
    inline constexpr Core::Uint32 ReceivesDecals{1};

    /**
     * @brief GBuffer에 표면 정보를 기록한 픽셀의 스텐실 비트입니다.
     */
    inline constexpr Core::Uint32 Surface{2};

    /**
     * @brief 조명 합성을 생략하는 표면을 식별하는 스텐실 비트입니다.
     */
    inline constexpr Core::Uint32 Prelit{4};

}
