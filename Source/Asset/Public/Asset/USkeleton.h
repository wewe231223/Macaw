/**
 * @file USkeleton.h
 * @brief 본 계층, 로컬 기준 포즈, 전역 바인드·역바인드 행렬을 공유하는 스켈레톤 에셋입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Import/FMeshImportBone.h"
#include "Core/Math/FTransform.h"
#include "CoreUObject/UObject.h"
#include "USkeleton.generated.h"

namespace Asset {

    /**
     * @brief 스켈레톤에서 허용하는 최대 본 수입니다.
     */
    inline constexpr Core::Uint32 MaxSkeletonBones{100000};

    /**
     * @brief 본 계층, 로컬 기준 포즈, 전역 바인드·역바인드 행렬을 공유하는 스켈레톤 에셋입니다.
     * @ingroup Asset
     */
    UCLASS()
    class USkeleton final : public CoreUObject::UObject {
        GENERATED_BODY()

    public:

        /**
         * @brief 본 계층, 로컬 기준 포즈, 전역 바인드·역바인드 행렬을 공유하는 스켈레톤 에셋을 구성합니다.
         */
        USkeleton() = default;

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~USkeleton() override = default;

    public:

        /**
         * @brief 본 계층을 검증하고 기준 포즈와 전역·역바인드 행렬을 재구성합니다.
         * @param Bones 부모 인덱스와 바인드 변환을 가진 본 배열입니다.
         * @return 유효한 본 데이터로 교체했으면 true입니다.
         */
        bool SetBones(Core::TArray<Asset::FMeshImportBone> Bones);

        /**
         * @brief 스켈레톤 본 계층을 반환합니다.
         * @return 스켈레톤이 소유하는 본 배열의 읽기 전용 참조입니다.
         */
        const Core::TArray<Asset::FMeshImportBone>& GetBones() const;

        /**
         * @brief 본별 전역 바인드 행렬을 반환합니다.
         * @return 스켈레톤이 소유하는 행렬 배열의 읽기 전용 참조입니다.
         */
        const Core::TArray<Core::FMatrix>& GetGlobalBindPoses() const;

        /**
         * @brief 본별 전역 바인드 역행렬을 반환합니다.
         * @return 스켈레톤이 소유하는 행렬 배열의 읽기 전용 참조입니다.
         */
        const Core::TArray<Core::FMatrix>& GetInverseBindPoses() const;

        /**
         * @brief 본별 바인드 포즈를 위치·회전·스케일로 분해한 레퍼런스 포즈를 반환합니다.
         * @return 본 인덱스 순서의 부모 본 기준 변환이며, 애니메이션 트랙이 없는 본은 이 값을 씁니다.
         */
        const Core::TArray<Core::FTransform>& GetReferencePose() const;

        /**
         * @brief 스켈레톤의 본과 파생 포즈 데이터가 유효한지 확인합니다.
         * @return 렌더링·애니메이션에 사용할 수 있는 스켈레톤이면 true입니다.
         */
        bool IsValid() const;

    private:

        /**
         * @brief 부모 인덱스와 로컬 바인드 변환을 가진 본 목록입니다.
         */
        Core::TArray<Asset::FMeshImportBone> mBones{};

        /**
         * @brief 본별 전역 바인드 포즈 행렬입니다.
         */
        Core::TArray<Core::FMatrix> mGlobalBindPoses{};

        /**
         * @brief 스키닝에 사용하는 본별 전역 바인드 포즈 역행렬입니다.
         */
        Core::TArray<Core::FMatrix> mInverseBindPoses{};

        /**
         * @brief 본별 로컬 기준 포즈입니다.
         */
        Core::TArray<Core::FTransform> mReferencePose{};
    };
}
