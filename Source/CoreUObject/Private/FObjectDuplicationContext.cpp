/**
 * @file FObjectDuplicationContext.cpp
 * @brief 복제 원본과 복제본의 비소유 대응표를 관리합니다.
 * @ingroup CoreUObject
 */

#include "CoreUObject/FObjectDuplicationContext.h"
#include "CoreUObject/UObject.h"

/**
 * @brief 서로 다른 동일 클래스 객체의 대응 관계를 등록합니다.
 */
bool FObjectDuplicationContext::Add(const UObject* Source, UObject* Duplicate) {
    if (Source == nullptr || Duplicate == nullptr || Source == Duplicate || Source->GetClass() != Duplicate->GetClass()) {
        return false;
    }

    return mObjects.emplace(Source, Duplicate).second;
}

/**
 * @brief 원본 주소에 대응하는 복제본을 조회합니다.
 */
UObject* FObjectDuplicationContext::FindDuplicate(const UObject* Source) const {
    const auto Position{mObjects.find(Source)};
    return Position != mObjects.end() ? Position->second : nullptr;
}

/**
 * @brief 등록된 모든 대응 관계를 반환합니다.
 */
const TMap<const UObject*, UObject*>& FObjectDuplicationContext::GetObjects() const {
    return mObjects;
}

/**
 * @brief 객체를 삭제하지 않고 대응 관계만 비웁니다.
 */
void FObjectDuplicationContext::Clear() {
    mObjects.clear();
}
