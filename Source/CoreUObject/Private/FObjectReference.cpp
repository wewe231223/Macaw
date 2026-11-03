#include "CoreUObject/FObjectReference.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/UObject.h"

namespace CoreUObject {

    CoreUObject::FObjectReference::FObjectReference(const Core::FGuid& Guid)
		: mGuid{Guid} {
    }

    CoreUObject::FObjectReference::FObjectReference(const CoreUObject::UObject* Object) {
        if (Object != nullptr) {
            mGuid = Object->GetGuid();
            mHandle = Object->GetHandle();
        }
    }

    const Core::FGuid& CoreUObject::FObjectReference::GetGuid() const {
        return mGuid;
    }

    CoreUObject::UObject* CoreUObject::FObjectReference::Resolve() const {
        return mHandle.IsValid() ? CoreUObject::FObjectRegistry::Get()->Resolve(mHandle) : nullptr;
    }

    void CoreUObject::FObjectReference::Bind(const CoreUObject::FObjectGuidMap& Objects) {
        const auto Position{Objects.find(mGuid)};
        mHandle = Position != Objects.end() ? Position->second->GetHandle() : CoreUObject::FObjectHandle{};
    }

}
