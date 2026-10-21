#pragma once

template <typename T>
struct TMemberPointerTraits;

template <typename TValue, typename TOwner>
struct TMemberPointerTraits<TValue TOwner::*> {
public:
    using FOwner = TOwner;
    using FValue = TValue;
};
