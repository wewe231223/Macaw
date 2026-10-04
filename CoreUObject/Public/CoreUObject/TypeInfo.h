#pragma once

#include <memory>
#include <string_view>

using FObjectCreator = std::unique_ptr<class UObject> (*)();

struct FTypeInfo {
    std::string_view mTypeName{};
    const FTypeInfo* mParent{nullptr};
    FObjectCreator mCreator{nullptr};

    [[nodiscard]] bool IsA(const FTypeInfo* Type) const noexcept;

    template <typename T> [[nodiscard]] bool IsA() const noexcept;

    [[nodiscard]] bool IsExactlyA(const FTypeInfo* Type) const noexcept;
};

#define JG_DECLARE_ROOT_TYPEINFO(Type) \
    using TypeInfoOwner = Type; \
    static const FTypeInfo* StaticTypeInfo() noexcept; \
    virtual const FTypeInfo* GetTypeInfo() const noexcept;


#define JG_DECLARE_DERIVED_TYPEINFO(Type, ParentType) \
    using TypeInfoOwner = Type; \
    static const FTypeInfo* StaticTypeInfo() noexcept; \
    virtual const FTypeInfo* GetTypeInfo() const noexcept override;


#define JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(Type, ParentType) \
    using TypeInfoOwner = Type; \
    static const FTypeInfo* StaticTypeInfo() noexcept; \
    virtual const FTypeInfo* GetTypeInfo() const noexcept override;


template <typename T> [[nodiscard]] bool FTypeInfo::IsA() const noexcept {
    const auto TypeInfo{T::StaticTypeInfo()};
    return IsA(TypeInfo);
}
