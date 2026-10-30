/**
 * @file TCachedValue.h
 * @brief 스탬프로 유효성을 확인하고 필요할 때 갱신하는 값 캐시를 정의한다.
 */

#pragma once
#include "Core/Common.h"

#include <concepts>
#include <functional>
#include <memory>
#include <utility>

/**
 * @brief 값과 스탬프를 함께 보관하고 유효한 캐시의 재사용 및 갱신을 지원한다.
 * @tparam TValue 기본 생성 가능한 캐시 값 타입. 사용하는 갱신 연산에 필요한 대입을 지원해야 하며 Update()에는 동등성 비교도 필요하다.
 * @tparam TStamp 기본 생성, 동등성 비교 및 복사 대입을 지원하는 스탬프 타입. 기본 타입은 Uint64이다.
 * @details 유효 상태는 값 및 스탬프와 별도로 관리한다. 무효화해도 기존 값과 스탬프는 보관한다.
 */
template <typename TValue, typename TStamp = Uint64>
class TCachedValue {
public:
    /** @brief 값과 스탬프를 기본 초기화하고 유효한 기록이 없는 캐시를 생성한다. */
    TCachedValue() = default;

public:
    /**
     * @brief 유효한 캐시 값을 조회한다.
     * @return 캐시가 소유하는 값의 읽기 전용 포인터. 캐시가 무효하면 nullptr.
     * @details 갱신 시 같은 저장 공간의 내용이 변경될 수 있으며 포인터의 대상은 캐시 객체가 소멸할 때까지 존재한다.
     */
    const TValue* GetValue() const;

    /**
     * @brief 저장된 스탬프를 조회한다.
     * @return 캐시가 소유하는 스탬프의 읽기 전용 참조.
     * @details 캐시가 무효해도 저장된 스탬프를 반환한다. 유효 상태는 IsValid()로 별도 확인한다.
     */
    const TStamp& GetStamp() const;

    /**
     * @brief 캐시에 유효한 값이 있는지 확인한다.
     * @return 성공적으로 갱신된 뒤 무효화되지 않았으면 true, 그 외에는 false.
     */
    bool IsValid() const;

    /**
     * @brief 유효한 캐시의 스탬프가 요청한 스탬프와 일치하는지 확인한다.
     * @param Stamp 비교할 스탬프.
     * @return 캐시가 유효하고 저장된 스탬프가 Stamp와 같으면 true, 그 외에는 false.
     */
    bool IsCurrent(const TStamp& Stamp) const;

    /**
     * @brief 값과 스탬프가 이미 같은 유효한 캐시를 제외하고 새 값을 저장한다.
     * @param Value 캐시에 이동 대입할 새 값.
     * @param Stamp 값에 대응하는 새 스탬프.
     * @return 저장 내용을 갱신하면 true. 캐시가 유효하고 값과 스탬프가 모두 같아 갱신하지 않으면 false.
     * @details 스탬프만 달라져도 갱신한다. 대입 전에 캐시를 무효화하고 값과 스탬프를 모두 대입한 뒤 유효 상태로 전환한다.
     */
    bool Update(TValue Value, const TStamp& Stamp) requires std::equality_comparable<TValue>;

    /**
     * @brief 요청한 스탬프의 캐시를 반환하거나 빌더로 값을 갱신한다.
     * @tparam TBuilder TValue 참조를 인수로 호출할 수 있고 성공 여부를 불리언으로 판별할 수 있는 결과를 반환하는 타입.
     * @param Stamp 요청한 값의 스탬프.
     * @param Builder 캐시의 값 참조를 받아 내용을 갱신하고 성공 여부를 반환할 호출 대상.
     * @return 유효한 캐시 값의 읽기 전용 포인터. 빌더가 실패하면 nullptr.
     * @details 유효한 캐시의 스탬프가 요청과 일치하면 빌더를 호출하지 않는다. 캐시가 무효하거나 스탬프가 다르면 캐시를 무효화하고 빌더를 호출하며, 성공 시 새 스탬프를 기록하고 유효 상태로 전환한다.
     * @note 빌더는 저장된 값을 직접 수정한다. 실패 시 캐시는 무효 상태이고 이전 스탬프는 유지하지만 빌더가 변경한 값은 되돌리지 않는다.
     */
    template <typename TBuilder>
    const TValue* GetOrUpdate(const TStamp& Stamp, TBuilder&& Builder);

    /**
     * @brief 저장된 값과 스탬프를 유지하면서 캐시를 무효화한다.
     * @details 값이 소유한 자원을 해제하지 않는다. 다시 갱신하기 전까지 GetValue()는 nullptr을 반환하고 IsCurrent()는 false를 반환한다.
     */
    void Invalidate();

private:
    /** @brief 캐시가 소유하는 값. 무효화 후에도 보관하며 빌더가 직접 수정한다. */
    TValue mValue{};

    /** @brief 마지막으로 성공한 갱신의 스탬프. 최초 갱신 전에는 기본 초기화된 값이다. */
    TStamp mStamp{};

    /** @brief 현재 값과 스탬프를 유효한 캐시로 사용할 수 있는지 나타내는 상태. */
    bool mValid{};
};

template <typename TValue, typename TStamp>
const TValue* TCachedValue<TValue, TStamp>::GetValue() const {
    return mValid ? std::addressof(mValue) : nullptr;
}

template <typename TValue, typename TStamp>
const TStamp& TCachedValue<TValue, TStamp>::GetStamp() const {
    return mStamp;
}

template <typename TValue, typename TStamp>
bool TCachedValue<TValue, TStamp>::IsValid() const {
    return mValid;
}

template <typename TValue, typename TStamp>
bool TCachedValue<TValue, TStamp>::IsCurrent(const TStamp& Stamp) const {
    return mValid && mStamp == Stamp;
}

template <typename TValue, typename TStamp>
bool TCachedValue<TValue, TStamp>::Update(TValue Value, const TStamp& Stamp) requires std::equality_comparable<TValue> {
    if (IsCurrent(Stamp) && mValue == Value) {
        return false;
    }

    Invalidate();
    mValue = std::move(Value);
    mStamp = Stamp;
    mValid = true;

    return true;
}

template <typename TValue, typename TStamp>
template <typename TBuilder>
const TValue* TCachedValue<TValue, TStamp>::GetOrUpdate(const TStamp& Stamp, TBuilder&& Builder) {
    if (!IsCurrent(Stamp)) {
        Invalidate();

        if (!std::invoke(std::forward<TBuilder>(Builder), mValue)) {
            return nullptr;
        }

        mStamp = Stamp;
        mValid = true;
    }

    return GetValue();
}

template <typename TValue, typename TStamp>
void TCachedValue<TValue, TStamp>::Invalidate() {
    mValid = false;
}
