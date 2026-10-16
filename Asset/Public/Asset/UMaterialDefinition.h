/**
 * @file UMaterialDefinition.h
 * @brief 표준 재질 계수와 확장 매개변수 선언을 소유하는 부모 재질 에셋입니다.
 */
#pragma once

#include "Asset/UMaterial.h"
#include "Asset/FMaterialParameter.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include <span>

/** @brief 기본값, 렌더 설정과 파이프라인을 정의하며 인스턴스가 참조하는 부모 재질입니다. @note 파일 확장자는 .material이며 GPU 업로드는 이후 렌더 자원 계층에서 수행합니다. */
class UMaterialDefinition final : public UMaterial {
public:
    /** @brief 네이티브 리플렉션 생성 타입입니다. */
    ReflectTypes(UMaterialDefinition, UMaterial);

private:
    struct FData {
    public:
        ReflectTypes(FData);

    public:
        ReflectStruct(UMaterialDefinition::FData);
        bool IsValid() const;
        void Serialize(FArchive& Archive);

    public:
        ReflectProperty(mGPUData, "Core", EPropertyFlags::Edit);
        FMaterialGPUData mGPUData{};
        ReflectProperty(mParameters, "Parameters", EPropertyFlags::Edit);
        TArray<FMaterialParameterDefinition> mParameters{};
        ReflectProperty(mPipeline, "Pipeline", EPropertyFlags::Edit);
        FMaterialAssetReference mPipeline{};
        ReflectProperty(mBlendMode, "BlendMode", EPropertyFlags::Edit);
        EMaterialBlendMode mBlendMode{EMaterialBlendMode::Opaque};
        ReflectProperty(mTwoSided, "TwoSided", EPropertyFlags::Edit);
        bool mTwoSided{};

        ReflectMembers();
    };

public:
    /** @brief 기본 표면 계수로 빈 부모 재질을 생성합니다. */
    UMaterialDefinition();
    /** @brief 부모 재질을 해제합니다. */
    ~UMaterialDefinition() override;
    /** @brief 에셋 식별 정보의 복사를 금지합니다. */
    UMaterialDefinition(const UMaterialDefinition&) = delete;
    /** @brief 에셋 식별 정보의 대입을 금지합니다. */
    UMaterialDefinition& operator=(const UMaterialDefinition&) = delete;
    /** @brief 에셋 주소의 이동을 금지합니다. */
    UMaterialDefinition(UMaterialDefinition&&) = delete;
    /** @brief 에셋 주소의 이동 대입을 금지합니다. */
    UMaterialDefinition& operator=(UMaterialDefinition&&) = delete;

public:
    /** @brief 네이티브 클래스 메타데이터를 반환합니다. */
    ReflectClass(UMaterialDefinition);
    /** @brief 버전과 전체 데이터를 검증한 뒤 파일을 로드합니다. @return 실패하면 기존 데이터는 보존됩니다. */
    bool Initialize(const std::filesystem::path& Path);
    /** @brief 검증된 재질을 임시 파일 교체 방식으로 저장합니다. */
    bool Save(const std::filesystem::path& Path) const;
    /** @brief 중복 ID, 이름과 값의 범위를 검사한 뒤 전체 정의를 갱신합니다. */
    bool Configure(const FMaterialGPUData& GPUData, std::span<const FMaterialParameterDefinition> Parameters, const FMaterialAssetReference& Pipeline = {}, EMaterialBlendMode BlendMode = EMaterialBlendMode::Opaque, bool TwoSided = false);
    /** @brief 표준 계수를 검증하여 갱신합니다. */
    bool SetGPUData(const FMaterialGPUData& Data);
    /** @brief 기존 타입을 유지하면서 확장 매개변수 기본값을 갱신합니다. */
    bool SetParameterDefault(const FGuid& Id, const FMaterialParameterValue& Value);
    /** @brief 표준 계수를 반환합니다. */
    const FMaterialGPUData& GetGPUData() const;
    /** @brief 확장 매개변수 선언을 반환합니다. */
    std::span<const FMaterialParameterDefinition> GetParameters() const;
    /** @brief 확장 매개변수 선언을 ID로 찾습니다. */
    const FMaterialParameterDefinition* FindParameter(const FGuid& Id) const;
    /** @brief 부모의 기본값으로 최종 재질 스냅샷을 만듭니다. */
    bool BuildResolvedData(const IAssetRegistry& Registry, FResolvedMaterialData& OutData) const override;

private:
    bool SetData(const FData& Data);
    void Serialize(FArchive& Archive) override;

private:
    ReflectSetter(mData, SetData, "Data", EPropertyFlags::Edit);
    FData mData{};

    ReflectMembers();
};
