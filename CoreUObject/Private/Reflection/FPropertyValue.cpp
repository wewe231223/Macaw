#include "pch.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include <utility>

FPropertyValue::~FPropertyValue() {
    Reset();
}

FPropertyValue::FPropertyValue(const FPropertyValue& Other)
	: mType(Other.mType),
	  mData(mType != nullptr ? mType->mCopy(Other.mData) : nullptr) {
}

FPropertyValue& FPropertyValue::operator=(const FPropertyValue& Other) {
    if (this != &Other) {
        FPropertyValue Copy{Other};
        *this = std::move(Copy);
    }

    return *this;
}

FPropertyValue::FPropertyValue(FPropertyValue&& Other) noexcept
	: mType(std::exchange(Other.mType, nullptr)),
	  mData(std::exchange(Other.mData, nullptr)) {
}

FPropertyValue& FPropertyValue::operator=(FPropertyValue&& Other) noexcept {
    if (this != &Other) {
        Reset();
        mType = std::exchange(Other.mType, nullptr);
        mData = std::exchange(Other.mData, nullptr);
    }

    return *this;
}

const FNativeType* FPropertyValue::GetType() const {
    return mType;
}

bool FPropertyValue::IsEmpty() const {
    return mData == nullptr;
}

void FPropertyValue::Reset() {
    if (mData != nullptr) {
        mType->mDestroy(mData);
    }

    mType = nullptr;
    mData = nullptr;
}
