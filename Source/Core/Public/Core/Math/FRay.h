/**
 * @file FRay.h
 * @brief 원점과 방향으로 정의하는 월드 공간 광선입니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"

namespace Core {

    /**
     * @brief 원점과 방향으로 정의하는 월드 공간 광선입니다.
     * @ingroup Core
     */
    struct alignas(16) FRay {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FRay);

    public:
        /**
         * @brief FRay의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        FRay() = default;

        /**
         * @brief FRay의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Origin 광선의 시작점입니다.
         * @param[in] Direction 광선의 진행 방향입니다.
         */
        FRay(const Core::FVector& Origin, const Core::FVector& Direction);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FRay() = default;

        /**
         * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
         * @param[in] Other 복사할 원본 값입니다.
         */
        FRay(const Core::FRay& Other) = default;

        /**
         * @brief 상대 값의 상태를 대입합니다.
         * @param[in] Other 복사할 원본 값입니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        Core::FRay& operator=(const Core::FRay& Other) = default;

        /**
         * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
         * @param[in] Other 이동할 원본 값입니다.
         */
        FRay(Core::FRay&& Other) noexcept = default;

        /**
         * @brief 상대 값의 상태를 대입합니다.
         * @param[in] Other 이동할 원본 값입니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        Core::FRay& operator=(Core::FRay&& Other) noexcept = default;

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FRay);

        /**
         * @brief 광선 원점입니다.
         */
        Core::FVector mOrigin{};
        ReflectProperty(mOrigin, "Origin", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 광선 진행 방향입니다.
         */
        Core::FVector mDirection{1.0f, 0.0f, 0.0f};
        ReflectProperty(mDirection, "Direction", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);
    };

}
