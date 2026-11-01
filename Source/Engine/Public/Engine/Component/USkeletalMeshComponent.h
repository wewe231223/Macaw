/**
 * @file USkeletalMeshComponent.h
 * @brief 스켈레탈 메시의 에셋·재질과 객체별 본 포즈를 관리합니다.
 * @ingroup Engine
 */
#pragma once

#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/FSkeletalMeshPose.h"

namespace Asset {
    class USkeletalMesh;
    class UMaterial;
}

namespace Engine {

    /** @brief 공유 바인드 형상을 객체별 포즈로 변형하여 표시하는 컴포넌트입니다. */
    class USkeletalMeshComponent final : public Engine::UPrimitiveComponent {
    public:
        ReflectTypes(USkeletalMeshComponent, Engine::UPrimitiveComponent);

    public:
        /** @brief 에디터에서도 에셋 변경을 감지하는 컴포넌트를 생성합니다. */
        USkeletalMeshComponent();
        /** @brief 컴포넌트를 파괴합니다. */
        ~USkeletalMeshComponent() override;

    public:
        /** @brief 컴포넌트의 클래스 메타데이터를 반환합니다. @return 등록된 클래스입니다. */
        ReflectClass(USkeletalMeshComponent);

        /** @brief 기본 프리미티브 구분값을 반환합니다. @return 에셋이 없을 때도 대신 표시하지 않는 Cube 구분값입니다. */
        Engine::EPrimitiveType GetPrimitiveType() const override;
        /** @brief 공유 바인드 형상을 반환합니다. @return 공통 렌더 경로용 형상이며 없으면 nullptr입니다. */
        Asset::UStaticMesh* GetStaticMesh() const override;
        /** @brief 연결된 스켈레탈 메시를 반환합니다. @return 에셋 관리자가 소유하는 비소유 주소입니다. */
        Asset::USkeletalMesh* GetSkeletalMesh() const;
        /** @brief 현재 변형 위치와 경계를 가진 CPU 형상을 반환합니다. @return 피킹용 형상입니다. */
        const Asset::FMeshData& GetMeshData() const override;

        /** @brief 프로젝트 상대 에셋 경로를 반환합니다. @return 저장되는 메시 경로입니다. */
        const Core::FString& GetMeshPath() const;
        /** @brief 에셋 경로를 설정하고 포즈를 초기화합니다. @param Path Content 기준 .masset 경로입니다. @return 경로가 유효하고 가능한 경우 로드했으면 true입니다. */
        bool SetMeshPath(const Core::FString& Path);
        /** @brief 저장된 에셋 경로와 현재 에셋 개정을 해석합니다. @return 에셋과 포즈를 준비했으면 true입니다. */
        bool ResolveMesh();

        /** @brief 현재 본 포즈를 반환합니다. @return 컴포넌트가 소유한 포즈입니다. */
        const Engine::FSkeletalMeshPose& GetPose() const;
        /** @brief 본 하나의 부모 기준 변환을 변경합니다. @param Bone 본 인덱스입니다. @param Transform 부모 기준 아핀 행렬입니다. @return 유효한 포즈를 적용하면 true입니다. */
        bool SetBoneLocalTransform(Core::Uint32 Bone, const Core::FMatrix& Transform);
        /** @brief 전체 로컬 포즈를 변경합니다. @param Transforms 본 인덱스순 부모 기준 행렬입니다. @return 적용하면 true입니다. */
        bool SetLocalPose(const Core::TArray<Core::FMatrix>& Transforms);
        /** @brief 모든 본을 바인드 포즈로 복원합니다. @return 복원하면 true입니다. */
        bool ResetPose();

        /** @brief 전체 슬롯 재질 경로를 반환합니다. @return 빈 경로이면 슬롯 기본 재질을 사용합니다. */
        const Core::FString& GetMaterialPath() const;
        /** @brief 전체 슬롯 재질을 설정합니다. @param Path Content 기준 재질 경로입니다. @return 적용하면 true입니다. */
        bool SetMaterialPath(const Core::FString& Path);
        /** @brief 전체 슬롯 재질을 반환합니다. @return 없으면 nullptr입니다. */
        Asset::UMaterial* GetMaterial() const;
        /** @brief 저장된 전체 슬롯 재질을 해석합니다. @return 비어 있거나 로드했으면 true입니다. */
        bool ResolveMaterial();
        /** @brief 슬롯의 유효 재질을 조회합니다. @param Slot 재질 슬롯 번호입니다. @return 전체·슬롯·기본 순으로 선택한 재질입니다. */
        Asset::UMaterial* GetSlotMaterial(Core::Uint32 Slot) const;
        /** @brief 슬롯 오버라이드 경로를 반환합니다. @param Slot 슬롯 번호입니다. @return 설정되지 않았으면 빈 경로입니다. */
        const Core::FString& GetSlotMaterialPath(Core::Uint32 Slot) const;
        /** @brief 슬롯 재질을 교체합니다. @param Slot 슬롯 번호입니다. @param Path 재질 경로이며 비우면 기본값입니다. @return 적용하면 true입니다. */
        bool SetSlotMaterialPath(Core::Uint32 Slot, const Core::FString& Path);

        /** @brief 현재 포즈와 재질을 복사한 프록시를 생성합니다. @return GPU 스키닝에 필요한 프록시입니다. */
        Engine::FPrimitiveSceneProxy CreateSceneProxy() const override;
        /** @brief 에셋과 포즈가 준비되었는지 검사합니다. @return 표시 가능한 경우 true입니다. */
        bool ShouldCreateRenderState() const override;
        /** @brief 재등록 때 재해석하도록 에셋 참조를 해제합니다. */
        void UnregisterComponent() override;
        /** @brief 재임포트에 따른 포즈·버퍼 갱신을 예약합니다. @param DeltaTime 프레임 경과 시간입니다. */
        void Tick(float DeltaTime) override;
        /** @brief 에셋 참조·슬롯 재질·로컬 본 포즈를 직렬화합니다. @param Archive 저장 또는 복원할 아카이브입니다. */
        void Serialize(Core::FArchive& Archive) override;

    private:
        ReflectMembers();

        /** @brief 등록 시 에셋과 재질 및 저장된 포즈를 복원합니다. */
        void OnRegister() override;
        /** @brief 메시 경로를 에셋 관리자에서 해석합니다. @param Path Content 상대 경로입니다. @return 로드 실패 시 nullptr입니다. */
        Asset::USkeletalMesh* LoadMesh(const Core::FString& Path) const;
        /** @brief 재질 경로를 에셋 관리자에서 해석합니다. @param Path Content 상대 경로입니다. @return 로드 실패 시 nullptr입니다. */
        Asset::UMaterial* LoadMaterial(const Core::FString& Path) const;

    private:
        /** @brief Content 기준 스켈레탈 메시 경로입니다. */
        Core::FString mMeshPath{};
        ReflectAccessor(mMeshPath, GetMeshPath, SetMeshPath, "SkeletalMesh", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);
        /** @brief 에셋 관리자가 소유하는 메시입니다. */
        Asset::USkeletalMesh* mSkeletalMesh{};
        ReflectProperty(mSkeletalMesh, "ResolvedSkeletalMesh", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /** @brief 객체별 로컬·전역 포즈와 CPU 피킹 형상입니다. */
        Engine::FSkeletalMeshPose mPose{};
        ReflectProperty(mPose, "Pose", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /** @brief 에셋을 해석하기 전에 읽어 둔 저장 포즈입니다. */
        Core::TArray<Core::FMatrix> mPendingPose{};
        ReflectProperty(mPendingPose, "PendingPose", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /** @brief 전체 슬롯 오버라이드 경로입니다. */
        Core::FString mMaterialPath{};
        ReflectAccessor(mMaterialPath, GetMaterialPath, SetMaterialPath, "Material", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);
        /** @brief 전체 슬롯 오버라이드 재질입니다. */
        Asset::UMaterial* mMaterial{};
        ReflectProperty(mMaterial, "ResolvedMaterial", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /** @brief 슬롯별 재질 경로입니다. */
        Core::TArray<Core::FString> mMaterialOverrides{};
        ReflectProperty(mMaterialOverrides, "MaterialOverrides", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
