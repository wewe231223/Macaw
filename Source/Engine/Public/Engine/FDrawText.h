/**
 * @file FDrawText.h
 * @brief 월드 또는 화면 크기 기준으로 그릴 UTF-8 텍스트 요청입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FVector4.h"

namespace Engine {

    /**
     * @brief 월드 또는 화면 크기 기준으로 그릴 UTF-8 텍스트 요청입니다.
     */
    struct FDrawText {
    public:
        /**
         * @brief 텍스트를 그릴 월드 위치입니다.
         */
        Core::FVector mPosition{};

        /**
         * @brief UTF-8 문자열입니다.
         */
        Core::FString mText{};

        /**
         * @brief 표시할 RGBA 색상입니다.
         */
        Core::FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};

        /**
         * @brief 텍스트의 글자 크기입니다.
         */
        float mSize{0.5f};

        /**
         * @brief 가로·세로 크기 배율입니다.
         */
        Core::FVector2 mScale{1.0f, 1.0f};

        /**
         * @brief 텍스트 크기를 화면 픽셀 단위로 유지할지 여부입니다.
         */
        bool mScreenSpace{};

        /**
         * @brief 장면의 깊이 검사를 적용할지 여부입니다.
         */
        bool mDepthTest{true};
    };

}
