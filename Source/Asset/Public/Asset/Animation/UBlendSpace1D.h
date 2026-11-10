/**
 * @file UBlendSpace1D.h
 * @brief 입력 값 하나로 샘플을 선형 보간하는 블렌드 스페이스입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Animation/UBlendSpace.h"
#include "UBlendSpace1D.generated.h"

namespace Asset {

    /**
     * @brief X 축 하나에 샘플을 늘어놓고, 입력 양옆의 두 샘플을 거리 비율로 섞습니다.
     * @details 언리얼 UBlendSpace1D에 대응합니다. 예를 들어 Idle을 0, Walk를 1.5, Run을 4.5에 두면 Speed 3.0에서 Walk 50% + Run 50%가 됩니다.
     * 입력이 첫 샘플보다 작거나 마지막 샘플보다 크면 끝 샘플 하나만 씁니다. X가 같은 샘플이 여럿이면 먼저 추가한 샘플을 씁니다.
     */
    UCLASS()
    class UBlendSpace1D final : public Asset::UBlendSpace {
        GENERATED_BODY()

    public:
        UBlendSpace1D() = default;

        ~UBlendSpace1D() override = default;

    public:
        void GetSamplesToBlend(const Core::FVector& Input, Core::TArray<Asset::FBlendSampleWeight>& OutWeights) const override;

        Core::Uint32 GetNumAxes() const override;
    };

}
