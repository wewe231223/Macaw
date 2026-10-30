/**
 * @file UStaticMeshComponent.h
 * @brief 기본 프리미티브 또는 정적 메시 에셋을 머티리얼과 함께 표시하는 컴포넌트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/EPrimitiveType.h"

namespace Asset {
    class UMaterial;
}

namespace Engine {

    /**
     * @brief 기본 프리미티브 또는 정적 메시 에셋을 머티리얼과 함께 표시하는 컴포넌트입니다.
     * @ingroup Engine
     */
    class UStaticMeshComponent final : public Engine::UPrimitiveComponent {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(UStaticMeshComponent, Engine::UPrimitiveComponent);

    public:
        /**
         * @brief UStaticMeshComponent의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        UStaticMeshComponent();

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~UStaticMeshComponent() override;

    public:
        /**
         * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectClass(UStaticMeshComponent);

        /**
         * @fn UClass* UStaticMeshComponent::GetClass() const
         * @brief 객체의 실제 클래스 메타데이터를 반환합니다.
         * @return 실제 클래스 메타데이터의 비소유 주소입니다.
         */

        /**
         * @brief 표시할 프리미티브 메시 종류를 반환합니다.
         * @return 표시할 프리미티브 메시 종류입니다.
         */
        Engine::EPrimitiveType GetPrimitiveType() const override;

        /**
         * @brief 지원되는 프리미티브 종류를 검증하고 렌더 상태 갱신을 예약합니다.
         * @param[in] Type 기본 프리미티브 종류입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool SetPrimitiveType(Engine::EPrimitiveType Type);

        /**
         * @brief 이 컴포넌트에 설정된 프로젝트 상대 메시 에셋 경로를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Core::FString& GetMeshPath() const;

        /**
         * @brief 프로젝트의 메시 에셋 경로를 검증하고 컴포넌트의 렌더 상태를 갱신합니다.
         * @param[in] Path Content 기준 메시 에셋 경로이며 비우면 프리미티브를 사용합니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool SetMeshPath(const Core::FString& Path);

        /**
         * @brief 프리미티브가 사용하는 정적 메시의 비소유 주소를 반환합니다.
         * @return 대상의 비소유 주소이며, 없거나 유효하지 않으면 nullptr입니다.
         */
        Asset::UStaticMesh* GetStaticMesh() const override;

        /**
         * @brief 메시의 CPU 정점 속성·인덱스 데이터 참조를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Asset::FMeshData& GetMeshData() const override;
        void UnregisterComponent() override;

        /**
         * @brief 저장된 메시 경로를 에셋 관리자에서 해석하고 렌더 상태를 갱신합니다.
         * @return 경로가 비어 있거나 에셋을 찾았으면 true, 로드에 실패하면 false입니다.
         */
        bool ResolveMesh();

        /**
         * @brief Content 기준 머티리얼 파일 경로를 반환합니다.
         * @return 이 객체가 보관하는 값의 참조입니다.
         */
        const Core::FString& GetMaterialPath() const;

        /**
         * @brief Content 기준 머티리얼 파일 경로를 설정합니다.
         * @param[in] Path Content 기준 머티리얼 경로이며 비우면 메시 머티리얼을 사용합니다.
         * @return 값이 유효해 적용했으면 true, 거부했으면 false입니다.
         */
        bool SetMaterialPath(const Core::FString& Path);

        /**
         * @brief 연결된 머티리얼의 비소유 주소를 반환합니다.
         * @return 대상의 비소유 주소이며, 없거나 유효하지 않으면 nullptr입니다.
         */
        Asset::UMaterial* GetMaterial() const;

        /**
         * @brief 슬롯별 유효 재질을 조회합니다.
         * @param Slot 재질 슬롯 번호입니다.
         * @return 오버라이드 또는 메시 기본 재질의 비소유 주소입니다.
         */
        Asset::UMaterial* GetSlotMaterial(Core::Uint32 Slot) const;

        /**
         * @brief 슬롯 오버라이드 경로를 조회합니다.
         * @param Slot 재질 슬롯 번호입니다.
         * @return 오버라이드 경로 또는 빈 문자열입니다.
         */
        const Core::FString& GetSlotMaterialPath(Core::Uint32 Slot) const;

        /**
         * @brief 슬롯 재질을 교체합니다.
         * @param Slot 재질 슬롯 번호입니다.
         * @param Path Content 기준 재질 경로이며 빈 경로는 메시 기본값입니다.
         * @return 유효한 재질을 할당하면 true입니다.
         */
        bool SetSlotMaterialPath(Core::Uint32 Slot, const Core::FString& Path);

        /**
         * @brief 리플렉션 프로퍼티와 슬롯별 재질 오버라이드를 직렬화합니다.
         * @param Archive 저장 또는 복원할 아카이브입니다.
         */
        void Serialize(Core::FArchive& Archive) override;

        /**
         * @brief 저장된 머티리얼 경로를 에셋 관리자에서 해석하고 렌더 상태를 갱신합니다.
         * @return 경로가 비어 있거나 에셋을 찾았으면 true, 로드에 실패하면 false입니다.
         */
        bool ResolveMaterial();
        Engine::FPrimitiveSceneProxy CreateSceneProxy() const override;

        /**
         * @brief 현재 표시·에셋 상태에서 렌더 프록시를 생성할지 판정합니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool ShouldCreateRenderState() const override;

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

        void OnRegister() override;

        /**
         * @brief Content 상대 경로의 메시를 에셋 관리자에서 조회합니다.
         * @param[in] Path Content 기준 메시 에셋 경로입니다.
         * @return 캐시된 에셋의 비소유 주소이며, 로드에 실패하면 nullptr입니다.
         */
        Asset::UStaticMesh* LoadMesh(const Core::FString& Path) const;

        /**
         * @brief Content 상대 경로의 머티리얼을 에셋 관리자에서 조회합니다.
         * @param[in] Path Content 기준 머티리얼 에셋 경로입니다.
         * @return 캐시된 에셋의 비소유 주소이며, 로드에 실패하면 nullptr입니다.
         */
        Asset::UMaterial* LoadMaterial(const Core::FString& Path) const;

    private:
        /**
         * @brief 프리미티브 메시 종류입니다.
         */
        Engine::EPrimitiveType mPrimitive{Engine::EPrimitiveType::Cube};
        ReflectAccessor(mPrimitive, GetPrimitiveType, SetPrimitiveType, "Primitive", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 프로젝트 콘텐츠 폴더 기준 메시 에셋의 상대 경로입니다.
         */
        Core::FString mMeshPath{};
        ReflectAccessor(mMeshPath, GetMeshPath, SetMeshPath, "Mesh", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 에셋 관리자가 보유한 정적 메시의 비소유 주소입니다.
         */
        Asset::UStaticMesh* mStaticMesh{};
        ReflectProperty(mStaticMesh, "StaticMesh", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief Content 기준 머티리얼 파일 경로입니다.
         */
        Core::FString mMaterialPath{};
        ReflectAccessor(mMaterialPath, GetMaterialPath, SetMaterialPath, "Material", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 연결된 머티리얼의 비소유 주소입니다.
         */
        Asset::UMaterial* mMaterial{};
        ReflectProperty(mMaterial, "ResolvedMaterial", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 재질 슬롯별 Content 기준 오버라이드 경로입니다.
         */
        Core::TArray<Core::FString> mMaterialOverrides{};
        ReflectProperty(mMaterialOverrides, "MaterialOverrides", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
