/**
 * @file FAnimDataModel.h
 * @brief 애니메이션 시퀀스가 소유하는 원본 본 트랙과 시간 정보입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Animation/FBoneAnimationTrack.h"
#include "Asset/Animation/FFrameRate.h"

namespace Asset {

    /**
     * @brief 프레임 레이트와 프레임 수, 본별 트랙을 함께 검증하고 보관합니다.
     * @details 언리얼 UAnimDataModel에 대응합니다. 프레임 수는 구간 수이므로 키 수는 프레임 수 + 1입니다.
     */
    class FAnimDataModel {
    public:
        /**
         * @brief 한 클립이 가질 수 있는 최대 프레임 수입니다.
         */
        static constexpr Core::Uint32 MaxFrames{1000000};

        /**
         * @brief 한 클립이 가질 수 있는 최대 트랙 수입니다.
         */
        static constexpr Core::Uint32 MaxTracks{100000};

    public:
        /**
         * @brief 레이트·프레임 수·트랙이 서로 맞는지 검사합니다.
         * @details 프레임 수가 1 이상이고, 채널마다 키 수가 1 또는 키 수이며, 모든 값이 유한하고 회전이 정규화되어 있고, 본 이름이 겹치지 않아야 합니다.
         * @param[out] Error 실패 이유입니다.
         * @return 유효하면 true입니다.
         */
        bool Validate(Core::FString& Error) const;

        /**
         * @brief 프레임 레이트를 반환합니다.
         * @return 샘플 간격을 정하는 레이트입니다.
         */
        const Asset::FFrameRate& GetFrameRate() const;

        /**
         * @brief 프레임 레이트를 설정합니다.
         * @param[in] FrameRate 설정할 레이트입니다.
         */
        void SetFrameRate(const Asset::FFrameRate& FrameRate);

        /**
         * @brief 프레임(구간) 수를 반환합니다.
         * @return 마지막 키의 프레임 번호입니다.
         */
        Core::Uint32 GetNumberOfFrames() const;

        /**
         * @brief 프레임(구간) 수를 설정합니다.
         * @param[in] NumberOfFrames 마지막 키의 프레임 번호입니다.
         */
        void SetNumberOfFrames(Core::Uint32 NumberOfFrames);

        /**
         * @brief 채널이 변할 때 가지는 키 수를 반환합니다.
         * @return 프레임 수 + 1입니다.
         */
        Core::Uint32 GetNumberOfKeys() const;

        /**
         * @brief 재생 길이를 초 단위로 반환합니다.
         * @return 프레임 수 / 프레임 레이트입니다.
         */
        double GetPlayLength() const;

        /**
         * @brief 본 트랙을 반환합니다.
         * @return 트랙 목록입니다.
         */
        const Core::TArray<Asset::FBoneAnimationTrack>& GetBoneAnimationTracks() const;

        /**
         * @brief 본 트랙을 반환합니다.
         * @return 수정 가능한 트랙 목록입니다.
         */
        Core::TArray<Asset::FBoneAnimationTrack>& GetBoneAnimationTracks();

    private:
        /**
         * @brief 샘플 레이트입니다.
         */
        Asset::FFrameRate mFrameRate{};

        /**
         * @brief 프레임(구간) 수입니다.
         */
        Core::Uint32 mNumberOfFrames{};

        /**
         * @brief 본별 트랙입니다.
         */
        Core::TArray<Asset::FBoneAnimationTrack> mBoneAnimationTracks{};
    };

}
