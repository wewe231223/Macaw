#pragma once

#include <memory>
#include <string_view>

using FObjectCreator = std::unique_ptr<class UObject> (*)();

struct FTypeInfo {
    std::string_view mTypeName{};
    const FTypeInfo* mParent{nullptr};
    FObjectCreator mCreator{nullptr};

    [[nodiscard]] bool IsA(const FTypeInfo* Type) const noexcept;

    template <typename T>
    [[nodiscard]] bool IsA() const noexcept;

    [[nodiscard]] bool IsExactlyA(const FTypeInfo* Type) const noexcept;
};

#define JG_DECLARE_ROOT_TYPEINFO(Type) \
    using TypeInfoOwner = Type; \
    static const FTypeInfo* StaticTypeInfo() noexcept { \
        static const FTypeInfo Information{#Type, nullptr, +[]() -> std::unique_ptr<UObject> { \
            return std::make_unique<Type>(); \
        }}; \
        return &Information; \
    } \
    virtual const FTypeInfo* GetTypeInfo() const noexcept { \
        return StaticTypeInfo(); \
    }


#define JG_DECLARE_DERIVED_TYPEINFO(Type, ParentType) \
    using TypeInfoOwner = Type; \
    static const FTypeInfo* StaticTypeInfo() noexcept { \
        static const FTypeInfo Information{#Type, ParentType::StaticTypeInfo(), +[]() -> std::unique_ptr<UObject> { \
            return std::make_unique<Type>(); \
        }}; \
        return &Information; \
    } \
    virtual const FTypeInfo* GetTypeInfo() const noexcept override { \
        return StaticTypeInfo(); \
    }


#define JG_DECLARE_NON_CREATABLE_DERIVED_TYPEINFO(Type, ParentType) \
    using TypeInfoOwner = Type; \
    static const FTypeInfo* StaticTypeInfo() noexcept { \
        static const FTypeInfo Information{#Type, ParentType::StaticTypeInfo(), nullptr}; \
        return &Information; \
    } \
    virtual const FTypeInfo* GetTypeInfo() const noexcept override { \
        return StaticTypeInfo(); \
    }


#define JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(Type, ParentType) \
    JG_DECLARE_NON_CREATABLE_DERIVED_TYPEINFO(Type, ParentType)


template <typename T>
[[nodiscard]] bool FTypeInfo::IsA() const noexcept {
    const auto TypeInfo{T::StaticTypeInfo()};
    return IsA(TypeInfo);
}
