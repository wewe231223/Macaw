/**
 * @file FDrawLine.h
 * @brief 시작점, 끝점, 색상과 깊이 검사 정책을 갖는 보조 선분입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"

namespace Engine {

    /**
     * @brief 시작점, 끝점, 색상과 깊이 검사 정책을 갖는 보조 선분입니다.
     * @ingroup Engine
     */
    struct FDrawLine {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FDrawLine);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FDrawLine);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 선분 시작점입니다.
         */
        Core::FVector mStart{};
        ReflectProperty(mStart, "Start", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 선분 끝점입니다.
         */
        Core::FVector mEnd{};
        ReflectProperty(mEnd, "End", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 표시 색상입니다.
         */
        Core::FVector mColor{1.0f, 1.0f, 1.0f};
        ReflectProperty(mColor, "Color", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 깊이 검사를 사용할지 여부입니다.
         */
        bool mDepthTest{};
        ReflectProperty(mDepthTest, "DepthTest", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 선분의 픽셀 두께입니다.
         */
        float mThickness{1.0f};
        ReflectProperty(mThickness, "Thickness", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
