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
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FDrawText);

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FDrawText);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 텍스트를 그릴 월드 위치입니다.
         */
        Core::FVector mPosition{};
        ReflectProperty(mPosition, "Position", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief UTF-8 문자열입니다.
         */
        Core::FString mText{};
        ReflectProperty(mText, "Text", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 표시할 RGBA 색상입니다.
         */
        Core::FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
        ReflectProperty(mColor, "Color", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 텍스트의 글자 크기입니다.
         */
        float mSize{0.5f};
        ReflectProperty(mSize, "Size", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 가로·세로 크기 배율입니다.
         */
        Core::FVector2 mScale{1.0f, 1.0f};
        ReflectProperty(mScale, "Scale", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 텍스트 크기를 화면 픽셀 단위로 유지할지 여부입니다.
         */
        bool mScreenSpace{};
        ReflectProperty(mScreenSpace, "ScreenSpace", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 장면의 깊이 검사를 적용할지 여부입니다.
         */
        bool mDepthTest{true};
        ReflectProperty(mDepthTest, "DepthTest", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
