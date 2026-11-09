/**
 * @file FAssetImportOption.cpp
 * @brief 메시 임포터가 공통으로 따르는 좌표축·변환·메시 구성·재질 임포트 옵션입니다.
 * @ingroup Asset
 */

#include "Asset/Import/FAssetImportOption.h"

#include <cmath>

namespace Asset {

    /**
     * @brief 이동·회전·균일 스케일 옵션을 엔진 공간에서 적용하는 행렬을 만듭니다.
     */
    Core::FMatrix Asset::FAssetImportOption::GetImportTransform() const {
        // 행 벡터 규약이므로 왼쪽 행렬이 먼저 적용됩니다. 언리얼과 같이 스케일 → 회전 → 이동 순서입니다.
        return Core::FMatrix::Scale({mImportUniformScale, mImportUniformScale, mImportUniformScale}) * Core::FMatrix::Rotation(mImportRotation) * Core::FMatrix::Translation(mImportTranslation);
    }

    /**
     * @brief 옵션 값이 임포트에 사용할 수 있는지 확인합니다.
     */
    bool Asset::FAssetImportOption::IsValid() const {
        const bool ValidTransform{mImportTranslation.IsFinite() && std::isfinite(mImportRotation.pitch) && std::isfinite(mImportRotation.yaw) && std::isfinite(mImportRotation.roll) && std::isfinite(mImportUniformScale) && std::abs(mImportUniformScale) > 1.0e-6f};
        const bool ValidAnimation{!mImportAnimations || (mCustomSampleRate >= 0 && mCustomSampleRate <= 1000 && (mAnimationLength != Asset::EAnimationLengthImportType::SetRange || mFrameImportRangeEnd > mFrameImportRangeStart))};

        return ValidTransform && ValidAnimation;
    }

}
