#pragma once

#include <string_view>

using FObjectCreator = std::unique_ptr<class UObject>(*)(); 

struct FTypeInfo {
    std::string_view TypeName;
    const FTypeInfo* Parent{ nullptr };
	FObjectCreator Creator{ nullptr };

    [[nodiscard]] bool IsA(const FTypeInfo& Type) const noexcept {
        for (const FTypeInfo* Current = this; Current != nullptr; Current = Current->Parent) {
            if (Current == &Type) {
                return true;
            }
        }

        return false;
    }

	[[nodiscard]] bool isExactlyA(const FTypeInfo& Type) const noexcept {
		return this == &Type;
	}
};

#define JG_DECLARE_ROOT_TYPEINFO(Type) \
    inline static const FTypeInfo TypeInfo{ #Type, nullptr }; \
    static const FTypeInfo& StaticTypeInfo() noexcept { return TypeInfo; }


#define JG_DECLARE_DERIVED_TYPEINFO(Type, ParentType) \
    inline static const FTypeInfo TypeInfo{ #Type, &ParentType::StaticTypeInfo() }; \
    static const FTypeInfo& StaticTypeInfo() noexcept { return TypeInfo; }