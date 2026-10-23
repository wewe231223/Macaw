/**
 * @file FObjectRegistry.cpp
 * @brief 객체의 GUID 검색과 슬롯 세대 검사를 통한 핸들 해석을 담당합니다.
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
 * @brief 객체를 빈 슬롯과 GUID 인덱스에 등록하고 핸들을 부여합니다.
 */
bool FObjectRegistry::Register(UObject& Object) {
    if (Object.mRegistry != nullptr || mObjectsByGuid.contains(Object.mGuid)) {
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

    mObjectsByGuid.emplace(Object.mGuid, &Object);
    Slot.mObject = &Object;

    Object.mHandle = {Index, Slot.mGeneration};
    Object.mRegistry = this;

    return true;
}

/**
 * @brief 객체를 등록 해제하고 슬롯 세대를 올려 기존 핸들을 무효화합니다.
 */
void FObjectRegistry::Unregister(UObject& Object) {
    if (Object.mRegistry != this || Resolve(Object.mHandle) != &Object) {
        return;
    }

    FSlot& Slot{mSlots[Object.mHandle.mIndex]};

    mObjectsByGuid.erase(Object.mGuid);
    Slot.mObject = nullptr;
    ++Slot.mGeneration;

    if (Slot.mGeneration == 0) {
        ++Slot.mGeneration;
    }

    mFreeSlots.push_back(Object.mHandle.mIndex);

    Object.mHandle = {};
    Object.mRegistry = nullptr;
}

/**
 * @brief 중복되지 않는 유효 GUID로 객체의 식별자를 복원합니다.
 */
bool FObjectRegistry::RestoreGuid(UObject& Object, const FGuid& Guid) {
    if (!Guid.IsValid() || Object.mRegistry != this) {
        return false;
    }

    if (const auto Position{mObjectsByGuid.find(Guid)}; Position != mObjectsByGuid.end() && Position->second != &Object) {
        return false;
    }

    mObjectsByGuid.erase(Object.mGuid);
    Object.mGuid = Guid;
    mObjectsByGuid.emplace(Guid, &Object);

    return true;
}

/**
 * @brief 핸들의 슬롯 범위와 세대가 일치하는 객체를 조회합니다.
 */
UObject* FObjectRegistry::Resolve(FObjectHandle Handle) const {
    return Handle.mIndex < mSlots.size() && mSlots[Handle.mIndex].mGeneration == Handle.mGeneration ? mSlots[Handle.mIndex].mObject : nullptr;
}

/**
 * @brief GUID에 등록된 객체를 조회합니다.
 */
UObject* FObjectRegistry::Find(const FGuid& Guid) const {
    const auto Position{mObjectsByGuid.find(Guid)};
    return Position != mObjectsByGuid.end() ? Position->second : nullptr;
}

/**
 * @brief 현재 레지스트리에 등록된 객체 개수를 반환합니다.
 */
std::size_t FObjectRegistry::GetObjectCount() const {
    return mObjectsByGuid.size();
}
