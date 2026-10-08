#pragma once
#include "Core/Common.h"

#include <concepts>
#include <functional>
#include <memory>
#include <utility>

template <typename TValue, typename TStamp = Uint64>
class TCachedValue {
public:
    TCachedValue() = default;

public:
    const TValue* GetValue() const;
    const TStamp& GetStamp() const;
    bool IsValid() const;
    bool IsCurrent(const TStamp& Stamp) const;
    bool Update(TValue Value, const TStamp& Stamp) requires std::equality_comparable<TValue>;

    template <typename TBuilder>
    const TValue* GetOrUpdate(const TStamp& Stamp, TBuilder&& Builder);

    void Invalidate();

private:
    TValue mValue{};
    TStamp mStamp{};
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
