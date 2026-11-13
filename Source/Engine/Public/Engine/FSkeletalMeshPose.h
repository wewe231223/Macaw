/**
 * @file FSkeletalMeshPose.h
 * @brief 객체별 본 포즈와 GPU 스키닝 행렬 및 CPU 피킹 형상을 계산합니다.
 * @ingroup Engine
 */

#pragma once

#include "Asset/FMeshData.h"
#include "Core/FRevision.h"
#include "Core/Math/FMatrix.h"

namespace Asset {
    class USkeletalMesh;
    class USkeleton;
}

namespace Engine {

    /**
     * @brief 공유 에셋을 변경하지 않고 한 객체의 포즈와 변형 결과를 보관합니다.
     */
    class FSkeletalMeshPose {
    public:
        /**
         * @brief 비어 있는 포즈를 생성합니다.
         */
        FSkeletalMeshPose() = default;

    public:
        /**
         * @brief 에셋의 바인드 포즈로 초기화합니다.
         * @param Mesh 사용할 메시입니다.
         * @param Skeleton 본 계층·기준 포즈·역바인드 행렬을 제공하는 스켈레톤입니다.
         * @return 유효한 포즈를 만들면 true입니다.
         */
        bool Reset(const Asset::USkeletalMesh& Mesh, const Asset::USkeleton& Skeleton);

        /**
         * @brief 로컬 포즈와 스켈레톤으로 스킨 행렬·변형 정점·경계를 다시 계산합니다.
         * @param[in] Mesh 조회·저장·포즈 계산에 사용할 메시입니다.
         * @param Skeleton 본 계층과 역바인드 행렬을 제공하는 스켈레톤입니다.
         * @return 현재 메시를 사용해 포즈를 재계산했으면 true입니다.
         */
        bool Recalculate(const Asset::USkeletalMesh& Mesh, const Asset::USkeleton& Skeleton);

        /**
         * @brief 스켈레탈 포즈의 계산 결과를 다시 만들도록 표시합니다.
         */
        void Invalidate();

        /**
         * @brief 로컬 포즈를 검증하고 변형 형상을 계산하여 반영합니다.
         * @param Mesh 포즈와 연결된 에셋입니다.
         * @param LocalTransforms 부모 본 기준 행렬입니다.
         * @param Skeleton 로컬 포즈를 검증하고 스키닝 행렬을 계산할 스켈레톤입니다.
         * @return 반영에 성공하면 true이며, 검증에 실패하면 기존 포즈를 유지합니다.
         */
        bool SetLocalTransforms(const Asset::USkeletalMesh& Mesh, const Asset::USkeleton& Skeleton, const Core::TArray<Core::FMatrix>& LocalTransforms);

        /**
         * @brief 본별 로컬 변환을 검증해 저장하고 이전 메시 계산 결과를 무효화합니다.
         * @param[in] LocalTransforms 본 순서의 부모 기준 로컬 변환입니다.
         * @return 변환이 유효해 배열 또는 계산 결과를 적용하면 true입니다.
         */
        bool SetLocalTransforms(Core::TArray<Core::FMatrix> LocalTransforms);

        /**
         * @brief 애니메이션이 만든 로컬 포즈로 전역 행렬과 스키닝 행렬만 다시 계산합니다.
         * @details 매 프레임 호출하므로 행렬 검증과 CPU 변형 형상·경계 갱신을 생략합니다. 피킹 형상과 경계는 마지막으로 계산한 값을 유지합니다.
         * @param[in] Mesh 스키닝 행렬 수를 정하는 메시입니다.
         * @param[in] Skeleton 부모 인덱스와 역바인드 행렬을 제공하는 스켈레톤입니다.
         * @param[in] LocalTransforms 본 인덱스 순서의 부모 본 기준 변환입니다.
         * @return 본 수가 스켈레톤과 같아 적용했으면 true입니다.
         */
        bool SetAnimatedLocalTransforms(const Asset::USkeletalMesh& Mesh, const Asset::USkeleton& Skeleton, Core::TArray<Core::FMatrix> LocalTransforms);

        /**
         * @brief 본 하나의 로컬 변환을 교체합니다.
         * @param Mesh 사용할 에셋입니다.
         * @param Bone 본 인덱스입니다.
         * @param Transform 부모 본 기준 행렬입니다.
         * @param Skeleton 대상 본 계층과 역바인드 행렬을 제공하는 스켈레톤입니다.
         * @return 적용했으면 true입니다.
         */
        bool SetLocalTransform(const Asset::USkeletalMesh& Mesh, const Asset::USkeleton& Skeleton, Core::Uint32 Bone, const Core::FMatrix& Transform);

        /**
         * @brief 로컬 포즈를 반환합니다.
         * @return 본 인덱스순 행렬입니다.
         */
        const Core::TArray<Core::FMatrix>& GetLocalTransforms() const;

        /**
         * @brief 메시 공간의 현재 본 변환을 반환합니다.
         * @return 본 인덱스순 전역 포즈입니다.
         */
        const Core::TArray<Core::FMatrix>& GetGlobalTransforms() const;

        /**
         * @brief 역바인드 행렬에 현재 전역 포즈를 곱한 결과를 반환합니다.
         * @return GPU에 전달할 스키닝 행렬입니다.
         */
        const Core::TArray<Core::FMatrix>& GetSkinMatrices() const;

        /**
         * @brief 현재 포즈로 변형된 CPU 형상을 반환합니다.
         * @return 피킹과 경계 계산용 형상입니다.
         */
        const Asset::FMeshData& GetMeshData() const;

        /**
         * @brief 포즈의 변경 식별자를 반환합니다.
         * @return 마지막 계산 시 발급한 개정입니다.
         */
        Core::FRevision GetRevision() const;

    private:
        /**
         * @brief 부모 본 기준 현재 변환입니다.
         */
        Core::TArray<Core::FMatrix> mLocalTransforms{};

        /**
         * @brief 메시 공간 기준 현재 변환입니다.
         */
        Core::TArray<Core::FMatrix> mGlobalTransforms{};

        /**
         * @brief 바인드 위치를 현재 위치로 옮기는 행렬입니다.
         */
        Core::TArray<Core::FMatrix> mSkinMatrices{};

        /**
         * @brief CPU 피킹을 위해 현재 위치를 반영한 형상입니다.
         */
        Asset::FMeshData mMeshData{};

        /**
         * @brief 포즈 변경 개정입니다.
         */
        Core::FRevision mRevision{};

    };

}
