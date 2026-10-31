/**
 * @file TQuantity.h
 * @brief 컴파일 타임 차원을 가진 물리량 값 타입과 차원 연산을 정의합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/Literals/FDimension.h"
#include "Core/ReflectionMacros.h"
#include "Core/Reflection/EPropertyFlags.h"

#include <cmath>
#include <compare>
#include <type_traits>

namespace Core {

    /**
     * @brief 차원이 타입에 새겨진 물리량입니다.
     * @details 값은 항상 SI 기본 단위(m, kg, s, rad)로 저장하며 단위 변환은 리터럴로 만들 때와
     *          같은 차원의 단위로 나눠 꺼낼 때만 일어납니다.
     * @tparam D 물리량의 차원입니다.
     * @tparam T 값을 저장할 스칼라 타입입니다. 리플렉션 생성기가 기본 템플릿 인자를 지원하지 않으므로
     *           float 기반 별칭은 QuantityTypes.h를 사용합니다.
     * @ingroup Core
     */
    template <Core::FDimension D, typename T>
    struct TQuantity {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(TQuantity);

        /**
         * @brief 값을 저장하는 스칼라 타입입니다.
         */
        using ValueType = T;

        /**
         * @brief 이 물리량의 차원입니다.
         */
        static constexpr Core::FDimension Dimension = D;

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectTemplate(TQuantity);

        /**
         * @brief 값이 0인 물리량을 구성합니다.
         */
        constexpr TQuantity() = default;

        /**
         * @brief SI 기본 단위 값으로 물리량을 만듭니다.
         * @param[in] Value SI 기본 단위로 표현한 값입니다.
         * @return 생성한 물리량입니다.
         */
        static constexpr Core::TQuantity<D, T> FromBase(T Value) {
            Core::TQuantity<D, T> Result;
            Result.mValue = Value;
            return Result;
        }

        /**
         * @brief SI 기본 단위로 표현한 값을 반환합니다.
         * @return SI 기본 단위 값입니다.
         */
        constexpr T GetBase() const {
            return mValue;
        }

        /**
         * @brief 무차원 물리량을 스칼라로 변환합니다.
         * @return SI 기본 단위 값입니다.
         */
        constexpr operator T() const requires (D == Core::Dim::None) {
            return mValue;
        }

        /**
         * @brief 같은 차원의 두 물리량을 비교합니다.
         * @param[in] Other 비교할 물리량입니다.
         * @return 두 값의 순서입니다.
         */
        constexpr auto operator<=>(const Core::TQuantity<D, T>& Other) const = default;

        /**
         * @brief 같은 차원의 두 물리량이 같은지 비교합니다.
         * @param[in] Other 비교할 물리량입니다.
         * @return 두 값이 같으면 true입니다.
         */
        constexpr bool operator==(const Core::TQuantity<D, T>& Other) const = default;

        /**
         * @brief 값을 그대로 반환합니다.
         * @return 자기 자신의 복사본입니다.
         */
        constexpr Core::TQuantity<D, T> operator+() const {
            return *this;
        }

        /**
         * @brief 부호를 반전한 값을 반환합니다.
         * @return 연산을 적용한 새 값입니다.
         */
        constexpr Core::TQuantity<D, T> operator-() const {
            return FromBase(-mValue);
        }

        /**
         * @brief 같은 차원의 물리량을 더합니다.
         * @param[in] Other 더할 물리량입니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        constexpr Core::TQuantity<D, T>& operator+=(const Core::TQuantity<D, T>& Other) {
            mValue += Other.mValue;
            return *this;
        }

        /**
         * @brief 같은 차원의 물리량을 뺍니다.
         * @param[in] Other 뺄 물리량입니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        constexpr Core::TQuantity<D, T>& operator-=(const Core::TQuantity<D, T>& Other) {
            mValue -= Other.mValue;
            return *this;
        }

        /**
         * @brief 스칼라 값을 곱합니다.
         * @param[in] Scale 곱할 스칼라 값입니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        constexpr Core::TQuantity<D, T>& operator*=(T Scale) {
            mValue *= Scale;
            return *this;
        }

        /**
         * @brief 스칼라 값으로 나눕니다.
         * @param[in] Scale 나눌 스칼라 값입니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        constexpr Core::TQuantity<D, T>& operator/=(T Scale) {
            mValue /= Scale;
            return *this;
        }

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

        /**
         * @brief SI 기본 단위로 저장한 값입니다.
         */
        T mValue{};
        ReflectProperty(mValue, "Value", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);
    };

    /**
     * @brief 같은 차원의 두 물리량을 더합니다.
     * @param[in] Left 왼쪽 피연산자입니다.
     * @param[in] Right 오른쪽 피연산자입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    template <Core::FDimension D, typename T>
    constexpr Core::TQuantity<D, T> operator+(Core::TQuantity<D, T> Left, Core::TQuantity<D, T> Right) {
        return Left += Right;
    }

    /**
     * @brief 같은 차원의 두 물리량을 뺍니다.
     * @param[in] Left 왼쪽 피연산자입니다.
     * @param[in] Right 오른쪽 피연산자입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    template <Core::FDimension D, typename T>
    constexpr Core::TQuantity<D, T> operator-(Core::TQuantity<D, T> Left, Core::TQuantity<D, T> Right) {
        return Left -= Right;
    }

    /**
     * @brief 물리량에 스칼라 값을 곱합니다.
     * @details 스칼라를 산술 타입 템플릿으로 받아 정수와 무차원 물리량을 섞어도 내장 연산자와 모호해지지 않습니다.
     * @param[in] Value 물리량입니다.
     * @param[in] Scale 곱할 스칼라 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    template <Core::FDimension D, typename T, typename S>
        requires std::is_arithmetic_v<S>
    constexpr Core::TQuantity<D, T> operator*(Core::TQuantity<D, T> Value, S Scale) {
        return Value *= static_cast<T>(Scale);
    }

    /**
     * @brief 스칼라 값에 물리량을 곱합니다.
     * @param[in] Scale 곱할 스칼라 값입니다.
     * @param[in] Value 물리량입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    template <Core::FDimension D, typename T, typename S>
        requires std::is_arithmetic_v<S>
    constexpr Core::TQuantity<D, T> operator*(S Scale, Core::TQuantity<D, T> Value) {
        return Value *= static_cast<T>(Scale);
    }

    /**
     * @brief 물리량을 스칼라 값으로 나눕니다.
     * @param[in] Value 물리량입니다.
     * @param[in] Scale 나눌 스칼라 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    template <Core::FDimension D, typename T, typename S>
        requires std::is_arithmetic_v<S>
    constexpr Core::TQuantity<D, T> operator/(Core::TQuantity<D, T> Value, S Scale) {
        return Value /= static_cast<T>(Scale);
    }

    /**
     * @brief 스칼라 값을 물리량으로 나눠 역차원 물리량을 만듭니다.
     * @param[in] Scale 나눠질 스칼라 값입니다.
     * @param[in] Value 나눌 물리량입니다.
     * @return 차원 지수가 반전된 물리량입니다.
     */
    template <Core::FDimension D, typename T, typename S>
        requires std::is_arithmetic_v<S>
    constexpr Core::TQuantity<Core::Dim::None - D, T> operator/(S Scale, Core::TQuantity<D, T> Value) {
        return Core::TQuantity<Core::Dim::None - D, T>::FromBase(static_cast<T>(Scale) / Value.GetBase());
    }

    /**
     * @brief 두 물리량을 곱해 차원을 합성합니다.
     * @param[in] Left 왼쪽 피연산자입니다.
     * @param[in] Right 오른쪽 피연산자입니다.
     * @return 두 차원 지수를 더한 물리량입니다.
     */
    template <Core::FDimension A, Core::FDimension B, typename T>
    constexpr Core::TQuantity<A + B, T> operator*(Core::TQuantity<A, T> Left, Core::TQuantity<B, T> Right) {
        return Core::TQuantity<A + B, T>::FromBase(Left.GetBase() * Right.GetBase());
    }

    /**
     * @brief 두 물리량을 나눠 차원을 합성합니다.
     * @details 같은 차원끼리 나누면 무차원이 되어 스칼라로 변환할 수 있으므로 원하는 단위로 값을 꺼낼 때 사용합니다.
     * @param[in] Left 왼쪽 피연산자입니다.
     * @param[in] Right 오른쪽 피연산자입니다.
     * @return 왼쪽 차원 지수에서 오른쪽 차원 지수를 뺀 물리량입니다.
     */
    template <Core::FDimension A, Core::FDimension B, typename T>
    constexpr Core::TQuantity<A - B, T> operator/(Core::TQuantity<A, T> Left, Core::TQuantity<B, T> Right) {
        return Core::TQuantity<A - B, T>::FromBase(Left.GetBase() / Right.GetBase());
    }

    namespace Math {
        /**
         * @brief 물리량의 절댓값을 반환합니다.
         * @param[in] Value 물리량입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        template <Core::FDimension D, typename T>
        constexpr Core::TQuantity<D, T> Abs(Core::TQuantity<D, T> Value) {
            return Value.GetBase() < T{} ? -Value : Value;
        }

        /**
         * @brief 두 물리량 중 작은 값을 반환합니다.
         * @param[in] Left 왼쪽 피연산자입니다.
         * @param[in] Right 오른쪽 피연산자입니다.
         * @return 더 작은 물리량입니다.
         */
        template <Core::FDimension D, typename T>
        constexpr Core::TQuantity<D, T> Min(Core::TQuantity<D, T> Left, Core::TQuantity<D, T> Right) {
            return Right < Left ? Right : Left;
        }

        /**
         * @brief 두 물리량 중 큰 값을 반환합니다.
         * @param[in] Left 왼쪽 피연산자입니다.
         * @param[in] Right 오른쪽 피연산자입니다.
         * @return 더 큰 물리량입니다.
         */
        template <Core::FDimension D, typename T>
        constexpr Core::TQuantity<D, T> Max(Core::TQuantity<D, T> Left, Core::TQuantity<D, T> Right) {
            return Left < Right ? Right : Left;
        }

        /**
         * @brief 물리량을 지정한 범위로 제한합니다.
         * @param[in] Value 제한할 물리량입니다.
         * @param[in] Low 하한입니다.
         * @param[in] High 상한입니다.
         * @return 범위 안으로 제한한 물리량입니다.
         */
        template <Core::FDimension D, typename T>
        constexpr Core::TQuantity<D, T> Clamp(Core::TQuantity<D, T> Value, Core::TQuantity<D, T> Low, Core::TQuantity<D, T> High) {
            return Value < Low ? Low : (High < Value ? High : Value);
        }

        /**
         * @brief 두 물리량을 선형 보간합니다.
         * @param[in] From 시작 값입니다.
         * @param[in] To 끝 값입니다.
         * @param[in] Alpha 0이면 From, 1이면 To가 되는 보간 계수입니다.
         * @return 보간한 물리량입니다.
         */
        template <Core::FDimension D, typename T>
        constexpr Core::TQuantity<D, T> Lerp(Core::TQuantity<D, T> From, Core::TQuantity<D, T> To, std::type_identity_t<T> Alpha) {
            return From + (To - From) * Alpha;
        }

        /**
         * @brief 모든 차원 지수가 짝수인 물리량의 제곱근을 반환합니다.
         * @param[in] Value 물리량입니다.
         * @return 차원 지수가 절반이 된 물리량입니다.
         */
        template <Core::FDimension D, typename T>
            requires (D.IsEven())
        Core::TQuantity<D.Half(), T> Sqrt(Core::TQuantity<D, T> Value) {
            return Core::TQuantity<D.Half(), T>::FromBase(std::sqrt(Value.GetBase()));
        }
    }

}

#include "TQuantityReflection.generated.h"
