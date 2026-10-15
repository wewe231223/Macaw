/**
 * @file TStateFlags.h
 * @brief 열거형 인덱스 기반 상태 비트와 범위 동안 상태를 바꾸고 복원하는 도우미를 제공합니다.
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <type_traits>

/**
 * @brief 열거값을 비트 위치로 사용하는 64비트 상태 집합입니다.
 * @tparam TEnum 0부터 시작하는 상태 인덱스와 Count 항목을 제공하는 열거형입니다.
 * @note Count는 1 이상 64 이하이어야 합니다. 열거값 자체를 비트 마스크로 사용하지 않습니다.
 */
template <typename TEnum>
    requires std::is_enum_v<TEnum>
class TStateFlags final {
public:
    /**
     * @brief 모든 상태 비트가 꺼진 집합을 생성합니다.
     */
    TStateFlags() = default;

public:
    /**
     * @brief 지정한 상태가 켜져 있는지 검사합니다.
     * @param Flag 조회할 상태 인덱스입니다.
     * @return 상태 비트가 켜져 있으면 true입니다. Count 범위 밖 값이면 false입니다.
     */
    bool IsSet(TEnum Flag) const;
    /**
     * @brief 지정한 상태 중 하나라도 켜져 있는지 검사합니다.
     * @param Flags 검사할 상태 인덱스 목록입니다.
     * @return 하나 이상의 상태가 켜져 있으면 true입니다. 빈 목록이면 false입니다.
     */
    bool HasAny(std::initializer_list<TEnum> Flags) const;
    /**
     * @brief 지정한 모든 상태가 켜져 있는지 검사합니다.
     * @param Flags 검사할 상태 인덱스 목록입니다.
     * @return 모든 상태가 켜져 있으면 true입니다. 빈 목록이면 true입니다.
     */
    bool HasAll(std::initializer_list<TEnum> Flags) const;
    /**
     * @brief 지정한 상태 비트를 설정하거나 해제합니다.
     * @param Flag 변경할 상태 인덱스입니다.
     * @param Value 적용할 상태 값입니다.
     * @note Count 범위 밖 상태는 변경하지 않습니다.
     */
    void Set(TEnum Flag, bool Value = true);
    /**
     * @brief 지정한 상태 비트를 해제합니다.
     * @param Flag 해제할 상태 인덱스입니다.
     */
    void Clear(TEnum Flag);
    /**
     * @brief 모든 상태 비트를 해제합니다.
     */
    void Reset();

private:
    /**
     * @brief 상태 인덱스를 단일 비트 마스크로 변환합니다.
     * @param Flag 변환할 상태 인덱스입니다.
     * @return 유효한 인덱스의 비트 마스크입니다. Count 범위 밖 값이면 0입니다.
     */
    static std::uint64_t MakeMask(TEnum Flag);

private:
    static_assert(static_cast<std::size_t>(TEnum::Count) > 0 && static_cast<std::size_t>(TEnum::Count) <= std::numeric_limits<std::uint64_t>::digits);
    /**
     * @brief 상태 인덱스에 대응하는 현재 64비트 저장 값입니다.
     */
    std::uint64_t mBits{};
};

/**
 * @brief 지정한 상태의 이전 값을 저장하고 소멸 시 복원하는 범위 도우미입니다.
 * @tparam TEnum TStateFlags에서 사용하는 상태 인덱스 열거형입니다.
 * @note 다른 상태 비트의 변경은 유지하며 같은 상태에 대한 중첩 범위도 각자의 이전 값으로 복원합니다.
 */
template <typename TEnum>
    requires std::is_enum_v<TEnum>
class TScopedStateFlag final {
public:
    /**
     * @brief 지정한 상태의 이전 값을 기록하고 새 값을 적용합니다.
     * @param Flags 범위보다 오래 살아 있어야 하는 상태 집합입니다.
     * @param Flag 범위 동안 변경할 상태 인덱스입니다.
     * @param Value 범위 진입 시 적용할 값입니다.
     */
    TScopedStateFlag(TStateFlags<TEnum>& Flags, TEnum Flag, bool Value = true);
    /**
     * @brief 지정한 상태를 범위 진입 전 값으로 복원합니다.
     */
    ~TScopedStateFlag();
    /**
     * @brief 상태 복원 책임의 복사 생성을 금지합니다.
     */
    TScopedStateFlag(const TScopedStateFlag&) = delete;
    /**
     * @brief 상태 복원 책임의 복사 대입을 금지합니다.
     */
    TScopedStateFlag& operator=(const TScopedStateFlag&) = delete;
    /**
     * @brief 상태 복원 책임의 이동 생성을 금지합니다.
     */
    TScopedStateFlag(TScopedStateFlag&&) = delete;
    /**
     * @brief 상태 복원 책임의 이동 대입을 금지합니다.
     */
    TScopedStateFlag& operator=(TScopedStateFlag&&) = delete;

private:
    /**
     * @brief 상태를 변경하고 복원할 외부 집합 참조입니다.
     */
    TStateFlags<TEnum>& mFlags;
    /**
     * @brief 복원할 상태 인덱스입니다.
     */
    TEnum mFlag;
    /**
     * @brief 범위 진입 직전의 상태 값입니다.
     */
    bool mPreviousValue{};
};

template <typename TEnum>
    requires std::is_enum_v<TEnum>
bool TStateFlags<TEnum>::IsSet(TEnum Flag) const {
    return (mBits & MakeMask(Flag)) != 0;
}

template <typename TEnum>
    requires std::is_enum_v<TEnum>
bool TStateFlags<TEnum>::HasAny(std::initializer_list<TEnum> Flags) const {
    for (TEnum Flag : Flags) {
        if (IsSet(Flag)) {
            return true;
        }
    }

    return false;
}

template <typename TEnum>
    requires std::is_enum_v<TEnum>
bool TStateFlags<TEnum>::HasAll(std::initializer_list<TEnum> Flags) const {
    for (TEnum Flag : Flags) {
        if (!IsSet(Flag)) {
            return false;
        }
    }

    return true;
}

template <typename TEnum>
    requires std::is_enum_v<TEnum>
void TStateFlags<TEnum>::Set(TEnum Flag, bool Value) {
    const std::uint64_t Mask{MakeMask(Flag)};

    if (Value) {
        mBits |= Mask;
    } else {
        mBits &= ~Mask;
    }
}

template <typename TEnum>
    requires std::is_enum_v<TEnum>
void TStateFlags<TEnum>::Clear(TEnum Flag) {
    Set(Flag, false);
}

template <typename TEnum>
    requires std::is_enum_v<TEnum>
void TStateFlags<TEnum>::Reset() {
    mBits = 0;
}

template <typename TEnum>
    requires std::is_enum_v<TEnum>
std::uint64_t TStateFlags<TEnum>::MakeMask(TEnum Flag) {
    const std::size_t Index{static_cast<std::size_t>(Flag)};

    return Index < static_cast<std::size_t>(TEnum::Count) ? std::uint64_t{1} << Index : 0;
}

template <typename TEnum>
    requires std::is_enum_v<TEnum>
TScopedStateFlag<TEnum>::TScopedStateFlag(TStateFlags<TEnum>& Flags, TEnum Flag, bool Value)
	: mFlags{Flags},
	  mFlag{Flag},
	  mPreviousValue{Flags.IsSet(Flag)} {
    mFlags.Set(mFlag, Value);
}

template <typename TEnum>
    requires std::is_enum_v<TEnum>
TScopedStateFlag<TEnum>::~TScopedStateFlag() {
    mFlags.Set(mFlag, mPreviousValue);
}
