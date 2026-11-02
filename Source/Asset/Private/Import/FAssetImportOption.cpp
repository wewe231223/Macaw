#include "Asset/Import/FAssetImportOption.h"

#include <cmath>

namespace Asset {

    Core::FMatrix Asset::FAssetImportOption::GetImportTransform() const {
        // 행 벡터 규약이므로 왼쪽 행렬이 먼저 적용됩니다. 언리얼과 같이 스케일 → 회전 → 이동 순서입니다.
        return Core::FMatrix::Scale({mImportUniformScale, mImportUniformScale, mImportUniformScale}) * Core::FMatrix::Rotation(mImportRotation) * Core::FMatrix::Translation(mImportTranslation);
    }

    bool Asset::FAssetImportOption::IsValid() const {
        return mImportTranslation.IsFinite() && std::isfinite(mImportRotation.pitch) && std::isfinite(mImportRotation.yaw) && std::isfinite(mImportRotation.roll) && std::isfinite(mImportUniformScale) && std::abs(mImportUniformScale) > 1.0e-6f;
    }

}
