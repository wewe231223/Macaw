/**
 * @file UMaterialInstance.h
 * @brief 부모 재질의 고정 필드와 확장 매개변수를 선택적으로 덮어쓰는 에셋입니다.
 */
#pragma once

#include "Asset/UMaterial.h"
#include "Asset/FMaterialParameter.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include <span>

/** @brief 부모 기본값을 상속하며 변경한 항목만 저장하는 재질입니다. @note 파일 확장자는 .materialinstance이며 부모는 UMaterialDefinition만 지원합니다. */
class UMaterialInstance final : public UMaterial {
public:
    /** @brief 네이티브 리플렉션 생성 타입입니다. */
    ReflectTypes(UMaterialInstance, UMaterial);

private:
    struct FData {
    public:
        ReflectTypes(FData);

    public:
        ReflectStruct(UMaterialInstance::FData);
        bool IsValid() const;
        void Serialize(FArchive& Archive);

    public:
        ReflectProperty(mParent, "Parent", EPropertyFlags::Edit);
        FMaterialAssetReference mParent{};
        ReflectProperty(mCoreOverrides, "CoreOverrides", EPropertyFlags::Edit);
        TArray<FMaterialCoreOverride> mCoreOverrides{};
        ReflectProperty(mParameterOverrides, "ParameterOverrides", EPropertyFlags::Edit);
        TArray<FMaterialParameterOverride> mParameterOverrides{};

        ReflectMembers();
    };

public:
    /** @brief 부모가 지정되지 않은 빈 인스턴스를 생성합니다. */
    UMaterialInstance();
    /** @brief 인스턴스를 해제합니다. */
    ~UMaterialInstance() override;
    /** @brief 에셋 식별 정보의 복사를 금지합니다. */
    UMaterialInstance(const UMaterialInstance&) = delete;
    /** @brief 에셋 식별 정보의 대입을 금지합니다. */
    UMaterialInstance& operator=(const UMaterialInstance&) = delete;
    /** @brief 에셋 주소의 이동을 금지합니다. */
    UMaterialInstance(UMaterialInstance&&) = delete;
    /** @brief 에셋 주소의 이동 대입을 금지합니다. */
    UMaterialInstance& operator=(UMaterialInstance&&) = delete;

public:
    /** @brief 네이티브 클래스 메타데이터를 반환합니다. */
    ReflectClass(UMaterialInstance);
    /** @brief 파일을 검증한 뒤 로드합니다. 부모 연결은 평가 시 수행합니다. */
    bool Initialize(const std::filesystem::path& Path);
    /** @brief 부모 참조와 덮어쓰기만 저장합니다. */
    bool Save(const std::filesystem::path& Path) const;
    /** @brief 부모 참조를 설정합니다. 기존 덮어쓰기는 평가 시 새 부모와 검증합니다. */
    bool SetParent(const FMaterialAssetReference& Parent);
    /** @brief 부모의 지속 참조를 반환합니다. */
    const FMaterialAssetReference& GetParent() const;
    /** @brief 고정 필드의 타입과 범위를 검증하여 덮어씁니다. */
    bool SetCoreOverride(EMaterialCoreParameter Parameter, const FMaterialParameterValue& Value);
    /** @brief 부모의 선언 ID와 정확한 타입을 검사한 뒤 확장 값을 덮어씁니다. */
    bool SetParameterOverride(const IAssetRegistry& Registry, const FGuid& Id, const FMaterialParameterValue& Value);
    /** @brief 고정 필드 덮어쓰기를 제거하여 상속을 복원합니다. */
    bool ClearCoreOverride(EMaterialCoreParameter Parameter);
    /** @brief 확장 매개변수 덮어쓰기를 제거하여 상속을 복원합니다. */
    bool ClearParameterOverride(const FGuid& Id);
    /** @brief 저장된 고정 필드 덮어쓰기를 반환합니다. */
    std::span<const FMaterialCoreOverride> GetCoreOverrides() const;
    /** @brief 저장된 확장 매개변수 덮어쓰기를 반환합니다. */
    std::span<const FMaterialParameterOverride> GetParameterOverrides() const;
    /** @brief 부모와 모든 덮어쓰기를 검증하여 최종 값을 만듭니다. @return 실패하면 OutData는 빈 기본 상태가 됩니다. */
    bool BuildResolvedData(const IAssetRegistry& Registry, FResolvedMaterialData& OutData) const override;

private:
    bool SetData(const FData& Data);
    void Serialize(FArchive& Archive) override;

private:
    ReflectSetter(mData, SetData, "Data", EPropertyFlags::Edit);
    FData mData{};

    ReflectMembers();
};
