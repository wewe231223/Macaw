/**
 * @file UMeshComponent.h
 * @brief 메시의 공통 머티리얼 참조와 슬롯별 덮어쓰기를 관리합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UPrimitiveComponent.h"
#include "UMeshComponent.generated.h"

namespace Asset {
    class UMaterial;
}

namespace Engine {

    /**
     * @brief 메시의 공통 머티리얼 참조와 슬롯별 덮어쓰기를 관리합니다.
     */
    UCLASS(Abstract)
    class UMeshComponent : public Engine::UPrimitiveComponent {
        GENERATED_BODY()

    public:
        /**
         * @brief 메시와 머티리얼을 연결하는 공통 컴포넌트를 생성합니다.
         */
        UMeshComponent() = default;

        /**
         * @brief 메시 컴포넌트의 머티리얼 참조와 공통 상태를 정리합니다.
         */
        ~UMeshComponent() override = default;

    public:
        /**
         * @brief 저장된 메시 경로를 실제 메시 에셋으로 해석합니다.
         * @return 저장한 경로를 메시 에셋으로 해석했으면 true입니다.
         */
        virtual bool ResolveMesh() = 0;

        /**
         * @brief 전체 덮어쓰기 머티리얼의 프로젝트 상대 경로를 반환합니다.
         * @return 전체 덮어쓰기 머티리얼의 프로젝트 상대 경로입니다.
         */
        const Core::FString& GetMaterialPath() const;

        /**
         * @brief 전체 덮어쓰기 머티리얼 경로를 검증·해석하고 렌더 상태를 갱신합니다.
         * @param[in] Path 작업에 사용할 파일 또는 프로젝트 상대 에셋 경로입니다.
         * @return 경로가 유효하고 필요한 에셋 조회에 성공하면 true입니다.
         */
        bool SetMaterialPath(const Core::FString& Path);

        /**
         * @brief 메시 전체에 적용할 덮어쓰기 머티리얼의 비소유 주소를 반환합니다.
         * @return 메시 전체에 적용할 덮어쓰기 머티리얼의 비소유 주소입니다.
         */
        Asset::UMaterial* GetMaterial() const;

        /**
         * @brief 저장한 전체 덮어쓰기 머티리얼 경로를 에셋 참조로 해석합니다.
         * @return 경로가 비었거나 머티리얼을 해석했으면 true입니다.
         */
        bool ResolveMaterial();

        /**
         * @brief 전체 덮어쓰기·슬롯 덮어쓰기·메시 기본값 순으로 슬롯 머티리얼을 구합니다.
         * @param[in] Slot 조회하거나 바꿀 머티리얼 슬롯입니다.
         * @return 전체 덮어쓰기·슬롯 덮어쓰기·메시 기본값 순으로 선택한 머티리얼이며 로드할 수 없으면 nullptr입니다.
         */
        Asset::UMaterial* GetSlotMaterial(Core::Uint32 Slot) const;

        /**
         * @brief 지정 슬롯의 덮어쓰기 머티리얼 경로를 반환합니다.
         * @param[in] Slot 조회하거나 바꿀 머티리얼 슬롯입니다.
         * @return 지정 슬롯의 덮어쓰기 머티리얼 경로입니다.
         */
        const Core::FString& GetSlotMaterialPath(Core::Uint32 Slot) const;

        /**
         * @brief 지정 슬롯의 머티리얼 경로를 검증해 덮어쓰기에 반영합니다.
         * @param[in] Slot 조회하거나 바꿀 머티리얼 슬롯입니다.
         * @param[in] Path 작업에 사용할 파일 또는 프로젝트 상대 에셋 경로입니다.
         * @return 슬롯·경로·참조 머티리얼이 유효해 반영되면 true입니다.
         */
        bool SetSlotMaterialPath(Core::Uint32 Slot, const Core::FString& Path);

        /**
         * @brief 슬롯별 머티리얼 경로를 비우고 렌더 상태를 갱신 대상으로 표시합니다.
         */
        void ClearMaterialOverrides();

        /**
         * @brief 슬롯별 머티리얼 덮어쓰기 경로 배열을 반환합니다.
         * @return 슬롯별 머티리얼 덮어쓰기 경로 배열입니다.
         */
        const Core::TArray<Core::FString>& GetSlotMaterialPaths() const;

        /**
         * @brief 슬롯별 머티리얼 경로 배열을 검증해 덮어쓰기를 교체합니다.
         * @param[in] Paths 슬롯 순서의 머티리얼 경로 배열입니다.
         * @return 모든 경로가 유효해 배열을 교체하면 true입니다.
         */
        bool SetSlotMaterialPaths(const Core::TArray<Core::FString>& Paths);

        /**
         * @brief 컴포넌트의 현재 상태를 복사한 프리미티브 프록시를 만듭니다.
         * @return 현재 상태를 담은 프록시입니다.
         */
        Engine::FPrimitiveSceneProxy CreateSceneProxy() const override;

        /**
         * @brief 등록된 렌더 상태와 월드 연결을 해제합니다.
         */
        void UnregisterComponent() override;

    private:
        /**
         * @brief 월드에 등록된 컴포넌트가 참조하는 에셋과 렌더 상태를 준비합니다.
         */
        void OnRegister() override;

        /**
         * @brief 월드의 에셋 관리자에서 지정한 머티리얼을 조회합니다.
         * @param[in] Path 작업에 사용할 파일 또는 프로젝트 상대 에셋 경로입니다.
         * @return 로드한 머티리얼의 비소유 주소이며 실패하면 nullptr입니다.
         */
        Asset::UMaterial* LoadMaterial(const Core::FString& Path) const;

    private:
        /**
         * @brief 전체 덮어쓰기 머티리얼의 프로젝트 상대 경로입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "Material", Getter = GetMaterialPath, Setter = SetMaterialPath)
        Core::FString mMaterialPath{};

        /**
         * @brief 메시 전체 덮어쓰기 머티리얼의 비소유 주소입니다.
         */
        Asset::UMaterial* mMaterial{};

        /**
         * @brief 슬롯별 머티리얼 덮어쓰기의 프로젝트 상대 경로입니다.
         */
        UPROPERTY(Serialize, SerializeLate, Name = "SlotMaterials", Getter = GetSlotMaterialPaths, Setter = SetSlotMaterialPaths)
        Core::TArray<Core::FString> mMaterialOverrides{};

        // 메시 setter가 오버라이드를 비우므로 메시 경로를 복원한 뒤에 로드합니다.
    };

}
