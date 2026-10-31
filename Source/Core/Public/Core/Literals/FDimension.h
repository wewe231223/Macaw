/**
 * @file FDimension.h
 * @brief 물리량의 차원을 기본 차원 지수로 표현하는 타입과 자주 쓰는 차원 상수를 정의합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "Core/Reflection/EPropertyFlags.h"

namespace Core {

    /**
     * @brief 길이, 질량, 시간, 각도 기본 차원의 지수 묶음입니다.
     * @details 템플릿 비타입 인자로 쓰기 위해 공개 멤버만 가진 구조적 타입으로 유지합니다.
     * @ingroup Core
     */
    struct FDimension {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FDimension);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FDimension);

        /**
         * @brief 두 차원을 곱한 결과 차원을 반환합니다.
         * @param[in] Other 곱할 차원입니다.
         * @return 지수를 성분별로 더한 차원입니다.
         */
        constexpr Core::FDimension operator+(const Core::FDimension& Other) const {
            return {mLength + Other.mLength, mMass + Other.mMass, mTime + Other.mTime, mAngle + Other.mAngle};
        }

        /**
         * @brief 이 차원을 다른 차원으로 나눈 결과 차원을 반환합니다.
         * @param[in] Other 나눌 차원입니다.
         * @return 지수를 성분별로 뺀 차원입니다.
         */
        constexpr Core::FDimension operator-(const Core::FDimension& Other) const {
            return {mLength - Other.mLength, mMass - Other.mMass, mTime - Other.mTime, mAngle - Other.mAngle};
        }

        /**
         * @brief 두 차원의 모든 지수가 같은지 비교합니다.
         * @param[in] Other 비교할 차원입니다.
         * @return 모든 지수가 같으면 true입니다.
         */
        constexpr bool operator==(const Core::FDimension& Other) const = default;

        /**
         * @brief 모든 지수가 짝수인지 검사합니다.
         * @return 제곱근을 취해도 정수 지수가 유지되면 true입니다.
         */
        constexpr bool IsEven() const {
            return mLength % 2 == 0 && mMass % 2 == 0 && mTime % 2 == 0 && mAngle % 2 == 0;
        }

        /**
         * @brief 제곱근을 취한 결과 차원을 반환합니다.
         * @return 지수를 성분별로 절반으로 나눈 차원입니다.
         */
        constexpr Core::FDimension Half() const {
            return {mLength / 2, mMass / 2, mTime / 2, mAngle / 2};
        }

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 길이(m) 지수입니다.
         */
        Core::Int32 mLength = 0;
        ReflectProperty(mLength, "Length", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 질량(kg) 지수입니다.
         */
        Core::Int32 mMass = 0;
        ReflectProperty(mMass, "Mass", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 시간(s) 지수입니다.
         */
        Core::Int32 mTime = 0;
        ReflectProperty(mTime, "Time", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 각도(rad) 지수입니다.
         * @details SI에서는 무차원이지만 도와 라디안을 섞는 실수를 막기 위해 별도 차원으로 둡니다.
         */
        Core::Int32 mAngle = 0;
        ReflectProperty(mAngle, "Angle", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

    /**
     * @brief 자주 쓰는 물리 차원 상수입니다.
     * @ingroup Core
     */
    namespace Dim {
        /**
         * @brief 무차원입니다.
         */
        inline constexpr Core::FDimension None{};

        /**
         * @brief 길이(m)입니다.
         */
        inline constexpr Core::FDimension Length{.mLength = 1};

        /**
         * @brief 질량(kg)입니다.
         */
        inline constexpr Core::FDimension Mass{.mMass = 1};

        /**
         * @brief 시간(s)입니다.
         */
        inline constexpr Core::FDimension Time{.mTime = 1};

        /**
         * @brief 각도(rad)입니다.
         */
        inline constexpr Core::FDimension Angle{.mAngle = 1};

        /**
         * @brief 넓이(m²)입니다.
         */
        inline constexpr Core::FDimension Area{Length + Length};

        /**
         * @brief 부피(m³)입니다.
         */
        inline constexpr Core::FDimension Volume{Area + Length};

        /**
         * @brief 주파수(Hz = s⁻¹)입니다.
         */
        inline constexpr Core::FDimension Frequency{None - Time};

        /**
         * @brief 속도(m/s)입니다.
         */
        inline constexpr Core::FDimension Speed{Length - Time};

        /**
         * @brief 가속도(m/s²)입니다.
         */
        inline constexpr Core::FDimension Acceleration{Speed - Time};

        /**
         * @brief 힘(N = kg·m/s²)입니다.
         */
        inline constexpr Core::FDimension Force{Mass + Acceleration};

        /**
         * @brief 에너지(J = N·m)입니다.
         */
        inline constexpr Core::FDimension Energy{Force + Length};

        /**
         * @brief 일률(W = J/s)입니다.
         */
        inline constexpr Core::FDimension Power{Energy - Time};

        /**
         * @brief 각속도(rad/s)입니다.
         */
        inline constexpr Core::FDimension AngularSpeed{Angle - Time};

        /**
         * @brief 각가속도(rad/s²)입니다.
         */
        inline constexpr Core::FDimension AngularAcceleration{AngularSpeed - Time};
    }

}
