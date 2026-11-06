/**
 * @file FSequenceTransformTrack.h
 * @brief 바인딩 대상의 변환을 제어하는 트랙에 대한 선언입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Sequence/FSequenceTrack.h"
#include "Asset/Sequence/FSequenceTransformSection.h"

namespace Asset {

    /**
     * @brief 바인딩 대상의 변환을 제어하는 트랙입니다.
     * @ingroup Asset
     */
    class FSequenceTransformTrack final : public Asset::FSequenceTrack {
    public:
        /**
         * @brief 트랙의 구체 타입을 반환합니다.
         * @return 트랙 타입입니다.
         */
        Asset::ESequenceTrackType GetType() const override;

        /**
         * @brief 아카이브 모드에 따라 트랙 내용을 읽거나 저장합니다.
         * @param[in,out] Archive 값을 읽거나 쓸 아카이브입니다.
         */
        void Serialize(Core::FArchive& Archive) override;

        /**
         * @brief 지정한 시간에 적용할 섹션을 찾습니다.
         * @param[in] Tick 조회할 시간이며 틱 단위입니다.
         * @return 겹치면 목록의 뒤쪽 섹션이며 해당 구간이 없으면 nullptr입니다.
         */
        const Asset::FSequenceTransformSection* FindSection(double Tick) const;

        /**
         * @brief 편집 대상인 첫 섹션을 반환하고 섹션이 없으면 새로 만듭니다.
         * @return 트랙이 소유한 섹션의 참조입니다.
         */
        Asset::FSequenceTransformSection& FindOrAddEditSection();

        /**
         * @brief 모든 섹션이 지정한 시간을 포함하도록 넓힙니다.
         * @param[in] Tick 포함할 시간이며 틱 단위입니다.
         */
        void ExpandSections(Core::Int32 Tick);

    private:
        /**
         * @brief 뒤쪽 항목이 우선하는 섹션 목록입니다.
         */
        Core::TArray<Asset::FSequenceTransformSection> mSections{};
    };

}
