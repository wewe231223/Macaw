/**
 * @file FBoneAnimationTrack.h
 * @brief 본 하나의 위치·회전·스케일 키 배열입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Types.h"
#include "Core/Math/FQuat.h"
#include "Core/Math/FVector.h"

namespace Asset {

    /**
     * @brief 부모 본 기준 로컬 변환을 일정 간격으로 샘플링한 키입니다.
     * @details 언리얼 FRawAnimSequenceTrack과 같이 채널마다 키 수는 데이터 모델의 키 수이거나, 값이 변하지 않으면 1입니다.
     */
    struct FRawAnimSequenceTrack {
    public:
        /**
         * @brief 위치 키입니다.
         */
        Core::TArray<Core::FVector> mPosKeys{};

        /**
         * @brief 정규화한 회전 키입니다. 인접 키는 같은 반구에 있도록 부호를 맞춥니다.
         */
        Core::TArray<Core::FQuat> mRotKeys{};

        /**
         * @brief 스케일 키입니다.
         */
        Core::TArray<Core::FVector> mScaleKeys{};
    };

    /**
     * @brief 본 이름과 그 본의 키 트랙입니다.
     * @details 트랙은 스켈레톤 인덱스가 아니라 이름으로 본을 가리키므로, 사용하는 쪽이 자신의 스켈레톤에 이름으로 매핑합니다.
     */
    struct FBoneAnimationTrack {
    public:
        /**
         * @brief 원본 계층의 본 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 키 데이터입니다.
         */
        Asset::FRawAnimSequenceTrack mInternalTrack{};
    };

}
