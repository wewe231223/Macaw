/**
 * @file FReflectionDefinition.cpp
 * @brief 중복 등록의 호환성을 검사하는 네이티브 타입 정의입니다.
 * @ingroup CoreUObject
 */

#include "FReflectionDefinition.h"

namespace CoreUObject {

    /**
     * @brief 종류, 네이티브 타입, 부모, 플래그, 팩터리와 빌더가 모두 같은지 검사합니다.
     */
    bool CoreUObject::FReflectionDefinition::operator==(const CoreUObject::FReflectionDefinition& Other) const {
        return mKind == Other.mKind && mNativeType == Other.mNativeType && mParent == Other.mParent && mFlags == Other.mFlags && mFactory == Other.mFactory && mClassBuilder == Other.mClassBuilder && mStructBuilder == Other.mStructBuilder && mEnumBuilder == Other.mEnumBuilder && mStructParent == Other.mStructParent;
    }

}
