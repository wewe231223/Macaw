/**
 * @file FSequencePropertyTrack.h
 * @brief 바인딩 대상의 float 프로퍼티를 제어하는 트랙에 대한 선언입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Sequence/FSequenceChannel.h"
#include "Asset/Sequence/FSequenceTrack.h"

namespace Asset {

    /**
     * @brief 바인딩 대상의 float 프로퍼티를 이름으로 찾아 제어하는 트랙입니다.
     * @ingroup Asset
     */
    class FSequencePropertyTrack final : public Asset::FSequenceTrack {
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
         * @brief 제어할 프로퍼티의 리플렉션 이름을 반환합니다.
         * @return 프로퍼티 이름의 참조입니다.
         */
        const Core::FString& GetPropertyName() const;

        /**
         * @brief 제어할 프로퍼티의 리플렉션 이름을 설정합니다.
         * @param[in] PropertyName 프로퍼티 이름입니다.
         */
        void SetPropertyName(Core::FString PropertyName);

        /**
         * @brief 프로퍼티 값의 채널을 반환합니다.
         * @return 트랙이 소유한 채널의 참조입니다.
         */
        Asset::FSequenceChannel& GetChannel();

        /**
         * @brief 프로퍼티 값의 채널을 반환합니다.
         * @return 트랙이 소유한 채널의 읽기 전용 참조입니다.
         */
        const Asset::FSequenceChannel& GetChannel() const;

    private:
        /**
         * @brief 제어할 프로퍼티의 리플렉션 이름입니다.
         */
        Core::FString mPropertyName{};

        /**
         * @brief 프로퍼티 값의 채널입니다.
         */
        Asset::FSequenceChannel mChannel{};
    };

}
