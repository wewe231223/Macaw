/**
 * @file FMeshImportTransform.cpp
 * @brief 원본 메시 공간의 위치·방향·탄젠트 부호를 엔진 공간으로 옮기는 임포터 공용 변환입니다.
 * @ingroup Asset
 */

#include "FMeshImportTransform.h"

#include <cmath>

namespace Asset {

    /**
     * @brief 원본 메시 공간에서 엔진 공간으로 가는 행렬로 변환을 구성합니다.
     */
    Asset::FMeshImportTransform::FMeshImportTransform(const Core::FMatrix& SourceToEngine)
		: mMatrix{SourceToEngine} {
        const float Determinant{mMatrix.Determinant()};
        Core::FMatrix Inverse{};

        // 행렬식이 0이면 스케일이 0인 축이 있어 노멀 변환에 필요한 역행렬을 구할 수 없습니다.
        mValid = std::isfinite(Determinant) && Determinant != 0.0f && mMatrix.TryInverse(Inverse);
        mReversesWinding = Determinant < 0.0f;

        if (mValid) {
            // 노멀은 비균일 스케일에서도 표면에 수직을 유지하도록 역전치 행렬로 변환합니다.
            // 전치하면 이동 성분이 4번째 열로 옮겨 가므로 방향 변환에 섞이지 않도록 0으로 지웁니다.
            mNormalMatrix = Inverse.Transposed();

            for (Core::Int32 Index{}; Index < 3; ++Index) {
                mNormalMatrix(Index, 3) = 0.0f;
                mNormalMatrix(3, Index) = 0.0f;
            }

            mNormalMatrix(3, 3) = 1.0f;
        }
    }

    /**
     * @brief Z-Up 오른손 좌표계의 좌표에서 Y만 반전해 엔진의 Z-Up 왼손 좌표계로 옮기는 행렬을 만듭니다.
     */
    Core::FMatrix Asset::FMeshImportTransform::MakeFlipY() {
        return Core::FMatrix::Scale({1.0f, -1.0f, 1.0f});
    }

    /**
     * @brief Y-Up 오른손 좌표계(Front +Z) 원본을 FBX 임포터와 같은 규칙으로 엔진 좌표계로 옮기는 행렬을 만듭니다.
     */
    Core::FMatrix Asset::FMeshImportTransform::MakeYUpConversion(const Asset::FAssetImportOption& Option) {
        // 행 벡터 규약이므로 각 행은 원본 X, Y, Z 축이 엔진 공간에서 향하는 방향입니다.
        // 원본은 Y-Up, Front +Z인 오른손 좌표계(glTF, Maya 기본값)입니다.
        Core::FMatrix Result{};

        if (Option.mForceFrontXAxis) {
            // Z-Up, Front +X 오른손 좌표계로 회전한 뒤 Y를 반전합니다: (x, y, z) → (z, -x, y)
            Result(0, 0) = 0.0f;
            Result(0, 1) = -1.0f;
            Result(1, 1) = 0.0f;
            Result(1, 2) = 1.0f;
            Result(2, 0) = 1.0f;
            Result(2, 2) = 0.0f;
        } else {
            // 언리얼 기본값인 Z-Up, Front -Y 오른손 좌표계로 회전한 뒤 Y를 반전합니다: (x, y, z) → (x, z, y)
            Result(1, 1) = 0.0f;
            Result(1, 2) = 1.0f;
            Result(2, 1) = 1.0f;
            Result(2, 2) = 0.0f;
        }

        return Result;
    }

    /**
     * @brief 원본 공간 기준 변환을 엔진 공간 기준 변환으로 바꿉니다.
     */
    bool Asset::FMeshImportTransform::ConvertTransform(const Core::FMatrix& Basis, const Core::FMatrix& Source, Core::FMatrix& Result) {
        Core::FMatrix Inverse{};

        if (!Basis.TryInverse(Inverse)) {
            return false;
        }

        // 엔진 좌표를 원본 좌표로 되돌리고, 원본 변환을 적용한 뒤, 다시 엔진 좌표로 옮깁니다.
        // 축 반전이 양쪽에서 상쇄되므로 결과 회전은 다시 행렬식이 양수인 정상 회전이 됩니다.
        Result = Inverse * Source * Basis;

        return true;
    }

    /**
     * @brief 변환이 역행렬을 가져 노멀을 변환할 수 있는지 확인합니다.
     */
    bool Asset::FMeshImportTransform::IsValid() const {
        return mValid;
    }

    /**
     * @brief 원본 삼각형의 꼭짓점 순서를 뒤집어야 하는지 확인합니다.
     */
    bool Asset::FMeshImportTransform::ReversesWinding() const {
        return mReversesWinding;
    }

    /**
     * @brief 이동을 포함해 위치를 변환합니다.
     */
    Core::FVector Asset::FMeshImportTransform::TransformPosition(const Core::FVector& Position) const {
        return mMatrix.TransformPosition(Position);
    }

    /**
     * @brief 이동을 제외하고 표면을 따르는 방향(탄젠트·바이탄젠트)을 변환합니다.
     */
    Core::FVector Asset::FMeshImportTransform::TransformDirection(const Core::FVector& Direction) const {
        return mMatrix.TransformVector(Direction);
    }

    /**
     * @brief 역전치 행렬로 노멀을 변환합니다.
     */
    Core::FVector Asset::FMeshImportTransform::TransformNormal(const Core::FVector& Normal) const {
        return mNormalMatrix.TransformVector(Normal);
    }

    /**
     * @brief 엔진 공간 노멀과 원본 탄젠트·바이탄젠트로 직교 탄젠트와 부호를 만듭니다.
     */
    Core::FVector4 Asset::FMeshImportTransform::MakeTangent(const Core::FVector& Normal, const Core::FVector& SourceTangent, const Core::FVector& SourceBitangent, float SourceSign) const {
        const Core::FVector Direction{TransformDirection(SourceTangent)};

        // 그람-슈미트 직교화: 탄젠트에서 노멀 방향 성분을 빼서 노멀과 정확히 수직이 되도록 만듭니다.
        Core::FVector Tangent{Direction - Normal * Normal.Dot(Direction)};

        if (!Tangent.Normalize()) {
            // 탄젠트가 없거나 노멀과 평행하면, 노멀과 거의 평행하지 않은 기준 축(기본 Z, 노멀이 Z에 가까우면 Y)과 노멀의 외적을 사용합니다.
            const Core::FVector Axis{std::abs(Normal.z) < 0.99f ? Core::FVector{0.0f, 0.0f, 1.0f} : Core::FVector{0.0f, 1.0f, 0.0f}};

            return {Axis.Cross(Normal).Normalized(), 1.0f};
        }

        const Core::FVector Bitangent{TransformDirection(SourceBitangent)};
        const float Alignment{Normal.Cross(Tangent).Dot(Bitangent)};

        if (std::isfinite(Alignment) && Alignment != 0.0f) {
            // 바이탄젠트는 원본 +V 방향이고 엔진 UV는 V가 뒤집혔으므로, (Normal × Tangent)가 +V와 같은 방향이면 부호는 -1입니다.
            return {Tangent, Alignment > 0.0f ? -1.0f : 1.0f};
        }

        // 바이탄젠트가 없으면 원본 부호를 사용합니다. (M·N) × (M·T)는 원본 외적에 det(M)의 부호를 곱한 방향이고
        // V 반전이 부호를 한 번 더 뒤집으므로, 손 방향이 바뀌는 변환(det < 0)에서는 원본 부호가 그대로 유지됩니다.
        const float Sign{SourceSign < 0.0f ? -1.0f : 1.0f};

        return {Tangent, mReversesWinding ? Sign : -Sign};
    }

}
