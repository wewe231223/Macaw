/**
 * @file FAssetSerialization.h
 * @brief 메시 배열의 원소 개수 검증과 바이너리 직렬화에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Serialization/MemoryArchive.h"

#include <type_traits>

/**
 * @brief 메시 배열의 원소 개수 검증과 바이너리 직렬화를 담당합니다.
 */
class FAssetSerialization {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(FAssetSerialization);

public:
    /**
     * @brief 이 타입의 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FAssetSerialization);

    /**
     * @brief 배열의 개수와 원소를 직렬화하고 로드할 개수와 남은 바이트를 검증합니다.
     * @param[in,out] Archive 로드 또는 저장 모드와 오류 상태를 제공하는 아카이브입니다.
     * @param[in,out] Values 원소 개수와 함께 읽거나 기록할 배열입니다.
     */
    template <typename T>
    static void SerializeValues(FMemoryArchive& Archive, TArray<T>& Values);

private:
    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();
};

template <typename T>
void FAssetSerialization::SerializeValues(FMemoryArchive& Archive, TArray<T>& Values) {
    Uint32 Count{Archive.IsLoading() ? 0 : static_cast<Uint32>(Values.size())};
    Archive.Serialize("Count", Count);

    constexpr std::size_t ElementSize{std::is_same_v<T, FVector2> ? 8 : std::is_same_v<T, FVector> ? 12 : sizeof(T)};

    if (Archive.HasError() || Count > 16000000 || (Archive.IsLoading() && Count > Archive.GetRemainingBytes() / ElementSize)) {
        Archive.SetError("Invalid mesh element count");
        return;
    }

    if (Archive.IsLoading()) {
        Values.resize(Count);
    }

    for (T& Value : Values) {
        Archive.Serialize("Value", Value);

        if (Archive.HasError()) {
            break;
        }
    }
}
