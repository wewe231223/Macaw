/**
 * @file FObjectReference.cpp
 * @brief GUID로 저장되고 세대 핸들로 해석되는 UObject 약한 참조입니다.
 * @ingroup CoreUObject
 */

#include "CoreUObject/FObjectReference.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/UObject.h"

namespace CoreUObject {

    /**
     * @brief 아직 핸들이 연결되지 않은 참조를 구성합니다.
     */
    CoreUObject::FObjectReference::FObjectReference(const Core::FGuid& Guid)
		: mGuid{Guid} {
    }

    /**
     * @brief 살아 있는 객체를 가리키는 참조를 구성합니다.
     */
    CoreUObject::FObjectReference::FObjectReference(const CoreUObject::UObject* Object) {
        if (Object != nullptr) {
            mGuid = Object->GetGuid();
            mHandle = Object->GetHandle();
        }
    }

    /**
     * @brief 대상 객체의 영속 GUID를 반환합니다.
     */
    const Core::FGuid& CoreUObject::FObjectReference::GetGuid() const {
        return mGuid;
    }

    /**
     * @brief 연결된 핸들로 살아 있는 대상 객체를 조회합니다.
     */
    CoreUObject::UObject* CoreUObject::FObjectReference::Resolve() const {
        return mHandle.IsValid() ? CoreUObject::FObjectRegistry::Get()->Resolve(mHandle) : nullptr;
    }

    /**
     * @brief 장면 대응표에서 GUID를 찾아 핸들을 연결합니다.
     */
    void CoreUObject::FObjectReference::Bind(const CoreUObject::FObjectGuidMap& Objects) {
        const auto Position{Objects.find(mGuid)};

        mHandle = Position != Objects.end() ? Position->second->GetHandle() : CoreUObject::FObjectHandle{};
    }

}
