#include "CoreUObject/TObjectRef.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/UObject.h"

CoreUObject::UObject* CoreUObject::ObjectRefPrivate::Resolve(CoreUObject::FObjectHandle Handle, const CoreUObject::UClass* Class) {
    CoreUObject::UObject* Object{CoreUObject::FObjectRegistry::Get()->Resolve(Handle)};

    return Object != nullptr && Object->IsA(Class) ? Object : nullptr;
}

Core::FGuid CoreUObject::ObjectRefPrivate::GetGuid(CoreUObject::FObjectHandle Handle) {
    const CoreUObject::UObject* Object{CoreUObject::FObjectRegistry::Get()->Resolve(Handle)};

    return Object != nullptr ? Object->GetGuid() : Core::FGuid::Null();
}
