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
}

namespace Engine {

    /**
     * @brief 공유 에셋을 변경하지 않고 한 객체의 포즈와 변형 결과를 보관합니다.
     */
    class FSkeletalMeshPose {
    public:
        ReflectTypes(FSkeletalMeshPose);

    public:
        /**
         * @brief 비어 있는 포즈를 생성합니다.
         */
        FSkeletalMeshPose() = default;

    public:
        /**
         * @brief 포즈 구조체의 리플렉션 정보를 반환합니다.
         * @return 등록된 타입입니다.
         */
        ReflectStruct(FSkeletalMeshPose);

        /**
         * @brief 에셋의 바인드 포즈로 초기화합니다.
         * @param Mesh 사용할 메시입니다.
         * @return 유효한 포즈를 만들면 true입니다.
         */
        bool Reset(const Asset::USkeletalMesh& Mesh);

        bool Recalculate(const Asset::USkeletalMesh& Mesh);
        void Invalidate();

        /**
         * @brief 로컬 포즈를 검증하고 변형 형상을 계산하여 반영합니다.
         * @param Mesh 포즈와 연결된 에셋입니다.
         * @param LocalTransforms 부모 본 기준 행렬입니다.
         * @return 반영에 성공하면 true이며, 검증에 실패하면 기존 포즈를 유지합니다.
         */
        bool SetLocalTransforms(const Asset::USkeletalMesh& Mesh, const Core::TArray<Core::FMatrix>& LocalTransforms);
        bool SetLocalTransforms(Core::TArray<Core::FMatrix> LocalTransforms);
        /**
         * @brief 본 하나의 로컬 변환을 교체합니다.
         * @param Mesh 사용할 에셋입니다.
         * @param Bone 본 인덱스입니다.
         * @param Transform 부모 본 기준 행렬입니다.
         * @return 적용했으면 true입니다.
         */
        bool SetLocalTransform(const Asset::USkeletalMesh& Mesh, Core::Uint32 Bone, const Core::FMatrix& Transform);

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
        /**
         * @brief 계산에 사용한 에셋의 개정을 반환합니다.
         * @return 재임포트 감지에 사용할 개정입니다.
         */
        Core::FRevision GetMeshRevision() const;

    private:
        ReflectMembers();

    private:
        /**
         * @brief 부모 본 기준 현재 변환입니다.
         */
        Core::TArray<Core::FMatrix> mLocalTransforms{};
        ReflectProperty(mLocalTransforms, "LocalTransforms", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 메시 공간 기준 현재 변환입니다.
         */
        Core::TArray<Core::FMatrix> mGlobalTransforms{};
        ReflectProperty(mGlobalTransforms, "GlobalTransforms", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 바인드 위치를 현재 위치로 옮기는 행렬입니다.
         */
        Core::TArray<Core::FMatrix> mSkinMatrices{};
        ReflectProperty(mSkinMatrices, "SkinMatrices", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief CPU 피킹을 위해 현재 위치를 반영한 형상입니다.
         */
        Asset::FMeshData mMeshData{};
        ReflectProperty(mMeshData, "MeshData", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 포즈 변경 개정입니다.
         */
        Core::FRevision mRevision{};
        ReflectProperty(mRevision, "Revision", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 계산 원본의 데이터 개정입니다.
         */
        Core::FRevision mMeshRevision{};
        ReflectProperty(mMeshRevision, "MeshRevision", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
