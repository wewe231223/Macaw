/**
 * @file FAssetSerialization.h
 * @brief 메시 배열의 원소 개수 검증과 바이너리 직렬화에 대한 선언입니다.
 * @ingroup Asset
 */

#pragma once

#include "Serialization/MemoryArchive.h"

#include <type_traits>

namespace Asset {

    /**
     * @brief 메시 배열의 원소 개수 검증과 바이너리 직렬화를 담당합니다.
     */
    class FAssetSerialization {
    public:
        /**
         * @brief 배열의 개수와 원소를 직렬화하고 로드할 개수와 남은 바이트를 검증합니다.
         * @param[in,out] Archive 모드에 따라 값을 읽거나 쓸 아카이브입니다.
         * @param[in,out] Values 원소 개수와 함께 읽거나 기록할 배열입니다.
         */
        template <typename T>
        static void SerializeValues(Serialization::FMemoryArchive& Archive, Core::TArray<T>& Values);
    };

    /**
     * @brief 배열의 개수와 원소를 직렬화하고 로드할 개수와 남은 바이트를 검증합니다.
     */
    template <typename T>
    void Asset::FAssetSerialization::SerializeValues(Serialization::FMemoryArchive& Archive, Core::TArray<T>& Values) {
        Core::Uint32 Count{Archive.IsLoading() ? 0 : static_cast<Core::Uint32>(Values.size())};

        Archive.Serialize("Count", Count);

        constexpr std::size_t ElementSize{std::is_same_v<T, Core::FVector2> ? 8 : std::is_same_v<T, Core::FVector> ? 12 : sizeof(T)};

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

}
