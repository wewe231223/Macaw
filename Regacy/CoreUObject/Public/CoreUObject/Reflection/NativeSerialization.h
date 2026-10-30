/**
 * @file NativeSerialization.h
 * @brief 네이티브 값, 리플렉션 구조체와 지속 에셋·객체 참조의 직렬화를 제공합니다.
 */

#pragma once

#include "Core/Archive/FArchive.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include <filesystem>
#include "Core/Asset/FAssetReference.h"

/**
 * @brief 구조체 메타데이터의 직렬화 대상 프로퍼티를 처리합니다.
 * @param Struct 사용할 유효한 구조체 메타데이터입니다.
 * @param Container 구조체 타입의 저장 공간입니다.
 * @param Archive 사용할 아카이브입니다.
 * @param IncludeInherited 기반 구조체 프로퍼티 포함 여부입니다.
 */
void SerializeScriptStructProperties(const UScriptStruct* Struct, void* Container, FArchive& Archive, bool IncludeInherited);

/**
 * @brief 네이티브 타입에 맞는 아카이브 직렬화 경로를 선택합니다.
 * @tparam T 직렬화할 값 타입입니다.
 * @param Archive 저장 또는 로드 아카이브입니다.
 * @param Name 필드 또는 범위 이름입니다.
 * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
 * @note 열거형은 Int64, 파일 경로는 일반 형식 문자열로 처리합니다. 사용자 Serialize, 리플렉션 구조체, 크기 조절 가능한 컨테이너와 아카이브 직접 지원 타입을 순서대로 검사하며 미지원 타입은 아카이브 오류를 설정합니다.
 */
template <typename T>
void SerializeNativeValue(FArchive& Archive, std::string_view Name, T& Value);

void SerializeAssetReference(FArchive& Archive, std::string_view GuidName, std::string_view PathName, FAssetReference& Reference);

/**
 * @brief 객체 참조를 GUID 문자열로 직렬화하고 로드 GUID를 참조 재매핑합니다.
 * @param Archive 사용할 아카이브입니다.
 * @param Name GUID 문자열 필드 이름입니다.
 * @param Object 저장할 참조 객체입니다. nullptr이면 유효한 PendingGuid를 사용합니다.
 * @param[in,out] PendingGuid 로드 시 읽고 재매핑한 GUID입니다. 빈 문자열이면 비웁니다.
 * @note 객체 포인터를 즉시 해석하지 않으며 GUID 구문 오류이면 아카이브 오류를 설정합니다.
 */
void SerializeObjectReference(FArchive& Archive, std::string_view Name, const UObject* Object, FGuid& PendingGuid);

/**
 * @brief 쓰기 작업을 실행하고 bool 반환 여부에 맞게 성공값을 처리합니다.
 * @tparam TSetter 무인수 쓰기 작업의 호출 가능 타입입니다.
 * @param Setter 실행할 쓰기 작업입니다.
 * @return 정확한 bool 반환 타입이면 해당 결과이고, 그 외 반환 타입이면 실행 후 true입니다.
 */
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
