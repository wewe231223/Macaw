#pragma once

#include "Core/Archive/FArchive.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include <filesystem>
#include "Core/Asset/FAssetPath.h"

void SerializeScriptStructProperties(const UScriptStruct* Struct, void* Container, FArchive& Archive, bool IncludeInherited);

template <typename T>
void SerializeNativeValue(FArchive& Archive, std::string_view Name, T& Value);

void SerializeAssetReference(FArchive& Archive, std::string_view GuidName, std::string_view PathName, FAssetHandle& Handle, FGuid& Guid, FAssetPath& Path);

void SerializeObjectReference(FArchive& Archive, std::string_view Name, const UObject* Object, FGuid& PendingGuid);

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
    } else if constexpr (std::is_same_v<T, std::filesystem::path>) {
        FString Path{Value.generic_string()};
        Archive.Serialize(Name, Path);

        if (Archive.IsLoading()) {
            Value = std::filesystem::path{Path};
        }
    } else if constexpr (requires { Value.Serialize(Archive); }) {
        Archive.SerializeStruct(Name, Value);
    } else if constexpr (requires { T::StaticStruct(); }) {
        Archive.BeginObjectScope(Name);
        SerializeScriptStructProperties(T::StaticStruct(), &Value, Archive, false);
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
        Archive.SetError();
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
