/**
 * @file FSequenceChannel.h
 * @brief 시간순 키에서 단일 float 값을 계산하는 채널에 대한 선언입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Sequence/FSequenceKey.h"

namespace Core {
    class FArchive;
}

namespace Asset {

    /**
     * @brief 시간순 키에서 단일 float 값을 계산합니다.
     * @ingroup Asset
     */
    class FSequenceChannel {
    public:
        /**
         * @brief 시간순으로 정렬된 키 목록을 반환합니다.
         * @return 채널이 소유한 키 목록의 참조입니다.
         */
        const Core::TArray<Asset::FSequenceKey>& GetKeys() const;

        /**
         * @brief 키를 시간순 위치에 추가하고 같은 시간의 키는 교체합니다.
         * @param[in] Key 추가할 키입니다.
         * @return 키 값이 유한하여 추가하면 true입니다.
         */
        bool AddKey(const Asset::FSequenceKey& Key);

        /**
         * @brief 지정한 시간의 키를 제거합니다.
         * @param[in] Tick 제거할 키의 시간이며 틱 단위입니다.
         * @return 키가 있어 제거하면 true입니다.
         */
        bool RemoveKey(Core::Int32 Tick);

        /**
         * @brief 키를 다른 시간으로 옮깁니다.
         * @param[in] FromTick 옮길 키의 시간이며 틱 단위입니다.
         * @param[in] ToTick 새 시간이며 틱 단위입니다.
         * @return 원본 키가 있고 새 시간에 다른 키가 없어 옮기면 true입니다.
         */
        bool MoveKey(Core::Int32 FromTick, Core::Int32 ToTick);

        /**
         * @brief 지정한 시간의 값을 계산합니다.
         * @param[in] Tick 계산할 시간이며 틱 단위입니다.
         * @param[out] Value 키 범위 밖이면 가장 가까운 키의 값을 받습니다.
         * @return 키가 있어 값을 계산하면 true입니다.
         */
        bool Evaluate(double Tick, float& Value) const;

        /**
         * @brief 아카이브 모드에 따라 키 목록을 읽거나 저장합니다.
         * @param[in,out] Archive 값을 읽거나 쓸 아카이브입니다.
         */
        void Serialize(Core::FArchive& Archive);

    private:
        /**
         * @brief 시간순으로 정렬되고 시간이 중복되지 않는 키 목록입니다.
         */
        Core::TArray<Asset::FSequenceKey> mKeys{};
    };

}
