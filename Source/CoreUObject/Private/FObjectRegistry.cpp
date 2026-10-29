/**
 * @file FObjectRegistry.cpp
 * @brief 프로세스 전역 객체 등록과 슬롯 세대 검사를 통한 핸들 해석을 담당합니다.
 * @ingroup CoreUObject
 */

#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/FObjectHandle.h"
#include "CoreUObject/UObject.h"

/**
 * @brief FObjectRegistry의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FObjectRegistry::FObjectRegistry() = default;

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
FObjectRegistry::~FObjectRegistry() = default;

/**
 * @brief 프로세스 수명 동안 유지되는 전역 객체 레지스트리를 반환합니다.
 */
FObjectRegistry* FObjectRegistry::Get() {
    static FObjectRegistry Registry{};
    return &Registry;
}

/**
 * @brief 객체를 전역 슬롯에 등록하고 핸들을 부여합니다.
 */
bool FObjectRegistry::Register(UObject* Object) {
    if (Object == nullptr || Object->mHandle.IsValid()) {
        return false;
    }

    Uint32 Index{};

    if (mFreeSlots.empty()) {
        Index = static_cast<Uint32>(mSlots.size());
        mSlots.push_back(FSlot{});
    } else {
        Index = mFreeSlots.back();
        mFreeSlots.pop_back();
    }

    FSlot& Slot{mSlots[Index]};

    Slot.mObject = Object;

    Object->mHandle = {Index, Slot.mGeneration};
    ++mObjectCount;

    return true;
}

/**
 * @brief 객체를 등록 해제하고 슬롯 세대를 올려 기존 핸들을 무효화합니다.
 */
void FObjectRegistry::Unregister(UObject* Object) {
    if (Object == nullptr || Resolve(Object->mHandle) != Object) {
        return;
    }

    FSlot& Slot{mSlots[Object->mHandle.mIndex]};

    Slot.mObject = nullptr;
    ++Slot.mGeneration;

    if (Slot.mGeneration == 0) {
        ++Slot.mGeneration;
    }

    mFreeSlots.push_back(Object->mHandle.mIndex);

    Object->mHandle = {};
    --mObjectCount;
}

/**
 * @brief 핸들의 슬롯 범위와 세대가 일치하는 객체를 조회합니다.
 */
UObject* FObjectRegistry::Resolve(FObjectHandle Handle) const {
    return Handle.mIndex < mSlots.size() && mSlots[Handle.mIndex].mGeneration == Handle.mGeneration ? mSlots[Handle.mIndex].mObject : nullptr;
}

/**
 * @brief 현재 레지스트리에 등록된 객체 개수를 반환합니다.
 */
std::size_t FObjectRegistry::GetObjectCount() const {
    return mObjectCount;
}

UObject* FObjectRegistry::FindObject(const FName& Name, const UObject* Outer) const {
    for (const FSlot& Slot : mSlots) {
        if (Slot.mObject != nullptr && Slot.mObject->GetFName() == Name && Slot.mObject->GetOuter() == Outer) {
            return Slot.mObject;
        }
    }
    return nullptr;
}

TArray<FObjectHandle> FObjectRegistry::GetObjects(const UClass* Class, const UObject* Outer, bool IncludeDerived) const {
    TArray<FObjectHandle> Objects{};
    if (Class == nullptr) {
        return Objects;
    }
    Objects.reserve(mObjectCount);
    for (const FSlot& Slot : mSlots) {
        const UObject* Object{Slot.mObject};
        if (Object == nullptr || (IncludeDerived ? !Object->IsA(Class) : Object->GetClass() != Class)) {
            continue;
        }
        const UObject* Parent{Object->GetOuter()};
        while (Parent != nullptr && Parent != Outer) {
            Parent = Parent->GetOuter();
        }
        if (Outer == nullptr || Parent == Outer) {
            Objects.push_back(Object->GetHandle());
        }
    }
    return Objects;
}
