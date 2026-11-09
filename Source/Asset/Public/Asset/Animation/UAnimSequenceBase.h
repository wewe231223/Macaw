/**
 * @file UAnimSequenceBase.h
 * @brief 시간축을 가진 애니메이션 에셋의 기반입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Animation/UAnimationAsset.h"
#include "UAnimSequenceBase.generated.h"

namespace Asset {

    /**
     * @brief 하나의 시간축을 따라 재생하는 애니메이션 에셋의 기반입니다.
     * @details 언리얼에서는 노티파이와 커브가 이 계층에 있으며, 이후 노티파이를 이곳에 추가합니다.
     */
    UCLASS(Abstract)
    class UAnimSequenceBase : public Asset::UAnimationAsset {
        GENERATED_BODY()

    public:
        UAnimSequenceBase() = default;

        ~UAnimSequenceBase() override = default;
    };

}
