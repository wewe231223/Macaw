/**
 * @file FMaterialParameter.h
 * @brief 지속 에셋 참조, 타입이 있는 확장 매개변수와 고정 필드 덮어쓰기를 정의합니다.
 */
#pragma once

#include "Core/Archive/FArchive.h"
#include "Core/Asset/FAssetPath.h"
#include "Core/Base/FGuid.h"
#include "Core/Base/FName.h"
#include "Core/Base/FAssetHandle.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "RenderCore/FMaterialBlendMode.h"
#include "RenderCore/FMaterialData.h"
#include <variant>

class IAssetRegistry;

/** @brief 런타임 핸들과 독립적인 GUID 및 가상 경로 기반 에셋 참조입니다. */
struct FMaterialAssetReference {
public:
    ReflectTypes(FMaterialAssetReference);

public:
    ReflectStruct(FMaterialAssetReference);
    /** @brief 식별 정보가 모두 비어 있는지 확인합니다. */
    bool IsNull() const;
    /** @brief GUID를 먼저 조회하고 경로로 대체 조회합니다. @param Registry 에셋 저장소입니다. @return 해석한 핸들이며 없으면 무효 핸들입니다. */
    FAssetHandle Resolve(const IAssetRegistry& Registry) const;
    const FString& GetPath() const;
    bool SetPath(const FString& Path);
    /** @brief 참조의 지속 식별 정보만 직렬화합니다. @param Archive 사용할 아카이브입니다. */
    void Serialize(FArchive& Archive);

public:
    ReflectReadOnlyProperty(mGuid, "Guid", EPropertyFlags::None);
    /** @brief 경로 변경에도 유지할 에셋 GUID입니다. */
    FGuid mGuid{};
    ReflectAccessor(mPath, GetPath, SetPath, "Path", EPropertyFlags::Edit);
    /** @brief GUID를 해석할 수 없을 때 사용할 가상 경로입니다. */
    FAssetPath mPath{};

    ReflectMembers();
};

/** @brief 확장 매개변수에서 지원하는 값 타입이며 열거 순서는 저장 형식의 일부입니다. */
enum class EMaterialParameterType : Uint8 {
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Uint,
    Texture2D
};

ReflectEnum(EMaterialParameterType, ReflectEnumValue(EMaterialParameterType, Float), ReflectEnumValue(EMaterialParameterType, Float2), ReflectEnumValue(EMaterialParameterType, Float3), ReflectEnumValue(EMaterialParameterType, Float4), ReflectEnumValue(EMaterialParameterType, Int), ReflectEnumValue(EMaterialParameterType, Uint), ReflectEnumValue(EMaterialParameterType, Texture2D));

/** @brief 정확한 타입과 유한 값 검사를 제공하는 재질 매개변수입니다. */
class FMaterialParameterValue {
public:
    ReflectTypes(FMaterialParameterValue);

    /** @brief 저장 가능한 네이티브 값들의 합집합입니다. */
    using FValue = std::variant<float, FVector2, FVector3, FVector4, Int32, Uint32, FMaterialAssetReference>;

public:
    /** @brief 0인 float 값으로 초기화합니다. */
    FMaterialParameterValue() = default;
    /** @brief float 값을 저장합니다. */
    explicit FMaterialParameterValue(float Value);
    /** @brief 2차원 벡터를 저장합니다. */
    explicit FMaterialParameterValue(const FVector2& Value);
    /** @brief 3차원 벡터를 저장합니다. */
    explicit FMaterialParameterValue(const FVector3& Value);
    /** @brief 4차원 벡터를 저장합니다. */
    explicit FMaterialParameterValue(const FVector4& Value);
    /** @brief 부호 있는 정수를 저장합니다. */
    explicit FMaterialParameterValue(Int32 Value);
    /** @brief 부호 없는 정수를 저장합니다. */
    explicit FMaterialParameterValue(Uint32 Value);
    /** @brief 텍스처의 지속 참조를 저장합니다. 빈 참조는 기본 텍스처를 뜻합니다. */
    explicit FMaterialParameterValue(const FMaterialAssetReference& Value);

public:
    ReflectStruct(FMaterialParameterValue);
    /** @brief 저장된 값의 정확한 타입을 반환합니다. */
    EMaterialParameterType GetType() const;
    /** @brief 읽기 전용 네이티브 값을 반환합니다. */
    const FValue& GetValue() const;
    bool SetValue(const FValue& Value);
    /** @brief 숫자의 유한 여부와 에셋 경로 형식을 검사합니다. */
    bool IsValid() const;
    /** @brief 타입과 값을 직렬화하며 실패한 로드는 기존 값을 보존합니다. */
    void Serialize(FArchive& Archive);

private:
    ReflectReadOnlyAccessor(mValue, GetType, "Type", EPropertyFlags::None);
    ReflectAccessor(mValue, GetValue, SetValue, "Value", EPropertyFlags::Edit);
    FValue mValue{0.0f};

    ReflectMembers();
};

/** @brief 부모 재질이 선언하는 확장 매개변수입니다. @note 이름 변경 시에도 mId를 유지해야 인스턴스의 덮어쓰기가 보존됩니다. */
struct FMaterialParameterDefinition {
public:
    ReflectTypes(FMaterialParameterDefinition);

public:
    ReflectStruct(FMaterialParameterDefinition);
    /** @brief 선언 식별자, 이름과 기본값을 검사합니다. */
    bool IsValid() const;
    /** @brief 선언과 기본값을 직렬화합니다. */
    void Serialize(FArchive& Archive);

public:
    ReflectReadOnlyProperty(mId, "Id", EPropertyFlags::None);
    /** @brief 재질 안에서 고유한 지속 식별자입니다. */
    FGuid mId{};
    ReflectProperty(mName, "Name", EPropertyFlags::Edit);
    /** @brief 재질 안에서 고유한 표시 및 셰이더용 이름입니다. */
    FName mName{};
    ReflectProperty(mDefaultValue, "Default", EPropertyFlags::Edit);
    /** @brief 타입을 함께 결정하는 기본값입니다. */
    FMaterialParameterValue mDefaultValue{};
    ReflectProperty(mGroup, "Group", EPropertyFlags::Edit);
    /** @brief 편집기에서 사용할 표시 그룹입니다. */
    FString mGroup{};

    ReflectMembers();
};

/** @brief 인스턴스가 덮어쓸 확장 매개변수의 ID와 값입니다. */
struct FMaterialParameterOverride {
public:
    ReflectTypes(FMaterialParameterOverride);

public:
    ReflectStruct(FMaterialParameterOverride);
    /** @brief 식별자와 값을 직렬화합니다. */
    void Serialize(FArchive& Archive);

public:
    ReflectReadOnlyProperty(mId, "Id", EPropertyFlags::None);
    /** @brief 부모 선언의 지속 식별자입니다. */
    FGuid mId{};
    ReflectProperty(mValue, "Value", EPropertyFlags::Edit);
    /** @brief 부모 기본값과 정확히 같은 타입이어야 하는 값입니다. */
    FMaterialParameterValue mValue{};

    ReflectMembers();
};

/** @brief 인스턴스에서 개별적으로 덮어쓸 수 있는 고정 재질 필드입니다. */
enum class EMaterialCoreParameter : Uint8 {
    BaseColor,
    Opacity,
    Emissive,
    NormalScale,
    Metallic,
    Roughness,
    Specular,
    AmbientOcclusion,
    OpacityMaskClipValue,
    Count
};

ReflectEnum(EMaterialCoreParameter, ReflectEnumValue(EMaterialCoreParameter, BaseColor), ReflectEnumValue(EMaterialCoreParameter, Opacity), ReflectEnumValue(EMaterialCoreParameter, Emissive), ReflectEnumValue(EMaterialCoreParameter, NormalScale), ReflectEnumValue(EMaterialCoreParameter, Metallic), ReflectEnumValue(EMaterialCoreParameter, Roughness), ReflectEnumValue(EMaterialCoreParameter, Specular), ReflectEnumValue(EMaterialCoreParameter, AmbientOcclusion), ReflectEnumValue(EMaterialCoreParameter, OpacityMaskClipValue));

/** @brief 고정 필드 하나에 대한 타입이 있는 덮어쓰기입니다. */
struct FMaterialCoreOverride {
public:
    ReflectTypes(FMaterialCoreOverride);

public:
    ReflectStruct(FMaterialCoreOverride);
    /** @brief 필드 종류와 값을 직렬화합니다. */
    void Serialize(FArchive& Archive);

public:
    ReflectReadOnlyProperty(mParameter, "Parameter", EPropertyFlags::None);
    /** @brief 덮어쓸 고정 필드입니다. */
    EMaterialCoreParameter mParameter{EMaterialCoreParameter::BaseColor};
    ReflectProperty(mValue, "Value", EPropertyFlags::Edit);
    /** @brief 색상 필드는 Float3, 나머지는 Float 값입니다. */
    FMaterialParameterValue mValue{FVector3{1.0f, 1.0f, 1.0f}};

    ReflectMembers();
};

/** @brief 고정 필드 덮어쓰기를 검사하여 적용합니다. @return 타입이나 범위가 잘못되면 false이며 Data는 유지됩니다. */
bool ApplyMaterialCoreOverride(FMaterialGPUData& Data, const FMaterialCoreOverride& Override);

/** @brief GPU 객체를 소유하지 않는, 기본값과 덮어쓰기를 합친 재질 스냅샷입니다. */
struct FResolvedMaterialData {
public:
    ReflectTypes(FResolvedMaterialData);

public:
    ReflectStruct(FResolvedMaterialData);
    /** @brief 확장 매개변수의 최종 값을 ID로 조회합니다. @return 없으면 nullptr입니다. */
    const FMaterialParameterValue* FindParameter(const FGuid& Id) const;

public:
    ReflectReadOnlyProperty(mGPUData, "GPUData", EPropertyFlags::Transient);
    /** @brief 의미가 고정된 표준 계수입니다. */
    FMaterialGPUData mGPUData{};
    ReflectReadOnlyProperty(mParameters, "Parameters", EPropertyFlags::Transient);
    /** @brief 부모 선언 순서로 배치한 확장 매개변수의 최종 값입니다. */
    TArray<FMaterialParameterOverride> mParameters{};
    ReflectReadOnlyProperty(mPipeline, "Pipeline", EPropertyFlags::Transient);
    /** @brief 부모가 지정한 파이프라인의 지속 참조입니다. */
    FMaterialAssetReference mPipeline{};
    ReflectReadOnlyProperty(mBlendMode, "BlendMode", EPropertyFlags::Transient);
    /** @brief 명시적인 블렌드 정책입니다. */
    EMaterialBlendMode mBlendMode{EMaterialBlendMode::Opaque};
    ReflectReadOnlyProperty(mTwoSided, "TwoSided", EPropertyFlags::Transient);
    /** @brief 양면 렌더링 여부입니다. */
    bool mTwoSided{};

    ReflectMembers();
};
