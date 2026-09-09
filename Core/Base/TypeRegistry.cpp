#include "PCH.h"
#include "TypeRegistry.h"

namespace {
	TMap<std::string_view, const FTypeInfo*> TypeMap{};
}

void TypeRegistry::Register(const FTypeInfo* Type) {
	if (Type == nullptr) {
		return;
	}
	TypeMap[Type->TypeName] = Type;
}

const FTypeInfo* TypeRegistry::Find(std::string_view TypeName) {
	const auto It = TypeMap.find(TypeName);
	if (It != TypeMap.end()) {
		return It->second;
	}
	return nullptr;
}
