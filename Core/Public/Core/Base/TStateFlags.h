#pragma once
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <type_traits>

template <typename TEnum>
    requires std::is_enum_v<TEnum>
class TStateFlags final {
public:
    TStateFlags() = default;

public:
    bool IsSet(TEnum Flag) const;
    bool HasAny(std::initializer_list<TEnum> Flags) const;
    bool HasAll(std::initializer_list<TEnum> Flags) const;
    void Set(TEnum Flag, bool Value = true);
    void Clear(TEnum Flag);
    void Reset();

private:
    static std::uint64_t MakeMask(TEnum Flag);

private:
    static_assert(static_cast<std::size_t>(TEnum::Count) > 0 && static_cast<std::size_t>(TEnum::Count) <= std::numeric_limits<std::uint64_t>::digits);
    std::uint64_t mBits{};
};

template <typename TEnum>
    requires std::is_enum_v<TEnum>
class TScopedStateFlag final {
public:
    TScopedStateFlag(TStateFlags<TEnum>& Flags, TEnum Flag, bool Value = true);
    ~TScopedStateFlag();
    TScopedStateFlag(const TScopedStateFlag&) = delete;
    TScopedStateFlag& operator=(const TScopedStateFlag&) = delete;
    TScopedStateFlag(TScopedStateFlag&&) = delete;
    TScopedStateFlag& operator=(TScopedStateFlag&&) = delete;

private:
    TStateFlags<TEnum>& mFlags;
    TEnum mFlag;
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
