#pragma once

#include "Core/Archive/FArchive.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include "CoreUObject/Reflection/UStruct.h"
#include <stdexcept>

template <typename T>
void SerializeNativeValue(FArchive& Archive, std::string_view Name, T& Value);

template <typename TSetter>
bool ApplyPropertySetter(TSetter&& Setter);

template <typename T>
void SerializeNativeValue(FArchive& Archive, std::string_view Name, T& Value) {
    if constexpr (std::is_enum_v<T>) {
        Int64 Number{static_cast<Int64>(Value)};
        Archive.Serialize(Name, Number);

        if (Archive.IsLoading()) {
            Value = static_cast<T>(Number);
        }
    } else if constexpr (requires { Value.Serialize(Archive); }) {
        Archive.SerializeStruct(Name, Value);
    } else if constexpr (requires { T::StaticStruct(); }) {
        Archive.BeginObjectScope(Name);
        T::StaticStruct()->SerializeDeclaredProperties(&Value, Archive);
        Archive.EndObjectScope();
    } else if constexpr (!std::is_same_v<T, FString> && requires { typename T::value_type; Value.resize(std::size_t{}); Value.size(); Value.begin(); }) {
        std::size_t Size{Value.size()};
        Archive.BeginArrayScope(Name, Size);

        if (Archive.IsLoading()) {
            Value.resize(Size);
        }

        for (std::size_t Index{}; Index < Size; ++Index) {
            typename T::value_type Element{Value[Index]};
            SerializeNativeValue(Archive, std::to_string(Index), Element);

            if (Archive.IsLoading()) {
                Value[Index] = std::move(Element);
            }
        }

        Archive.EndArrayScope();
    } else if constexpr (requires { Archive.Serialize(Name, Value); }) {
        Archive.Serialize(Name, Value);
    } else {
        throw std::logic_error{"This reflected value requires a custom serializer"};
    }
}

template <typename TSetter>
bool ApplyPropertySetter(TSetter&& Setter) {
    if constexpr (std::is_same_v<std::invoke_result_t<TSetter>, bool>) {
        return Setter();
    } else {
        Setter();
        return true;
    }
}
