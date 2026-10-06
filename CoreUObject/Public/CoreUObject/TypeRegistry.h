#include "CoreUObject/TypeInfo.h"

#include <vector>

namespace TypeRegistry {
    void Register(const FTypeInfo* Type);
    const FTypeInfo* Find(FName TypeName);
    std::vector<const FTypeInfo*> GetRegisteredTypes();
}
