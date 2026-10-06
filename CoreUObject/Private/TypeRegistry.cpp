#include "pch.h"
#include "CoreUObject/TypeRegistry.h"
#include "Core/Base/ErrorHandler.h"

namespace {
    TMap<FName, const FTypeInfo*> TypeMap{};
}

void TypeRegistry::Register(const FTypeInfo* Type) {
    if (Type == nullptr) {
        return;
    }

    const auto [Iterator, Inserted]{TypeMap.emplace(Type->mTypeName, Type)};

    ErrorHandler::Report(!Inserted && Iterator->second != Type, "TypeRegistry", "A different type is already registered with this name", ErrorHandler::EErrorLevel::Critical);
}

const FTypeInfo* TypeRegistry::Find(FName TypeName) {
    const auto It{TypeMap.find(TypeName)};

    if (It != TypeMap.end()) {
        return It->second;
    }

    return nullptr;
}

std::vector<const FTypeInfo*> TypeRegistry::GetRegisteredTypes() {
    std::vector<const FTypeInfo*> Types{};

    Types.reserve(TypeMap.size());

    for (const auto& [TypeName, Type] : TypeMap) {
        (void)TypeName;
        Types.push_back(Type);
    }

    std::ranges::sort(Types, [](const FTypeInfo* Left, const FTypeInfo* Right) {
        return Left->mTypeName.ToString() < Right->mTypeName.ToString();
    });

    return Types;
}
