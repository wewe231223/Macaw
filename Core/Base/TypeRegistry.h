#include "TypeInfo.h"

namespace TypeRegistry {
    void Register(const FTypeInfo* Type);
    const FTypeInfo* Find(std::string_view TypeName);
}

