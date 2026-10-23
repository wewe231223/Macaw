#include "pch.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include <utility>
#include "CoreUObject/Reflection/UClass.h"

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

const void* FPropertyValue::GetData() const {
    return mData;
}

void* FPropertyValue::GetMutableData() {
    return mData;
}

bool IsNativeObjectCompatible(const UObject* Object, const UClass* Class) {
    return Object == nullptr || Object->IsA(Class);
}

template const FNativeType* GetNativeType<UClass*>();
template void VisitNativeReferences<UClass*>(const void* Value, const std::function<void(UObject*)>& Visitor);
