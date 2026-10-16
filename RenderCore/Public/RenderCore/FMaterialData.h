/**
 * @file FMaterialData.h
 * @brief 표준 표면 재질의 고정 저장 규약을 정의합니다.
 */
#pragma once

#include "Core/CoreMinimal.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include <cstddef>
#include <limits>
#include <type_traits>

/** @brief 고정 재질 데이터의 필드 의미와 배치를 식별하는 버전입니다. */
inline constexpr Uint32 MaterialDataVersion{1};

/** @brief 선형 색상과 표준 셰이딩 계수를 담는 64바이트 GPU 저장 규약입니다. @details 확장 매개변수는 별도 저장소에 보관합니다. 기존 FMaterialGPUSlot과는 다른 규약입니다. */
struct alignas(16) FMaterialGPUData {
public:
    ReflectTypes(FMaterialGPUData);

public:
    ReflectStruct(FMaterialGPUData);

public:
    ReflectProperty(mBaseColor, "BaseColor", EPropertyFlags::Edit | EPropertyFlags::Color, ReflectRange(0.0f, std::numeric_limits<float>::max()));
    /** @brief 텍스처와 곱할 선형 RGB 기본색입니다. */
    FVector3 mBaseColor{1.0f, 1.0f, 1.0f};
    ReflectProperty(mOpacity, "Opacity", EPropertyFlags::Edit, ReflectRange(0.0f, 1.0f), ReflectSpeed(0.01f));
    /** @brief 0부터 1까지의 불투명도 계수입니다. */
    float mOpacity{1.0f};
    ReflectProperty(mEmissive, "Emissive", EPropertyFlags::Edit | EPropertyFlags::Color, ReflectRange(0.0f, std::numeric_limits<float>::max()));
    /** @brief 음수가 아닌 선형 발광 색상입니다. */
    FVector3 mEmissive{};
    ReflectProperty(mNormalScale, "NormalScale", EPropertyFlags::Edit, ReflectRange(0.0f, std::numeric_limits<float>::max()), ReflectSpeed(0.01f));
    /** @brief 음수가 아닌 노멀 맵 강도입니다. */
    float mNormalScale{1.0f};
    ReflectProperty(mMetallic, "Metallic", EPropertyFlags::Edit, ReflectRange(0.0f, 1.0f), ReflectSpeed(0.01f));
    /** @brief 0부터 1까지의 금속성입니다. */
    float mMetallic{};
    ReflectProperty(mRoughness, "Roughness", EPropertyFlags::Edit, ReflectRange(0.0f, 1.0f), ReflectSpeed(0.01f));
    /** @brief 0부터 1까지의 거칠기입니다. */
    float mRoughness{0.5f};
    ReflectProperty(mSpecular, "Specular", EPropertyFlags::Edit, ReflectRange(0.0f, 1.0f), ReflectSpeed(0.01f));
    /** @brief 0부터 1까지의 표준 반사 계수입니다. */
    float mSpecular{0.5f};
    ReflectProperty(mAmbientOcclusion, "AmbientOcclusion", EPropertyFlags::Edit, ReflectRange(0.0f, 1.0f), ReflectSpeed(0.01f));
    /** @brief 0부터 1까지의 환경 차폐 계수입니다. */
    float mAmbientOcclusion{1.0f};
    ReflectProperty(mOpacityMaskClipValue, "OpacityMaskClipValue", EPropertyFlags::Edit, ReflectRange(0.0f, 1.0f), ReflectSpeed(0.01f));
    /** @brief 0부터 1까지의 알파 마스크 기준값입니다. */
    float mOpacityMaskClipValue{0.5f};
    ReflectReadOnlyProperty(mFlags, "Flags", EPropertyFlags::Transient);
    /** @brief GPU 바인딩 단계가 설정할 플래그이며 에셋 기본값은 0입니다. */
    Uint32 mFlags{};
    ReflectReadOnlyProperty(mExtensionOffset, "ExtensionOffset", EPropertyFlags::Transient);
    /** @brief 별도 확장 저장소 안의 주소이며 에셋 기본값은 0입니다. */
    Uint32 mExtensionOffset{};
    ReflectReadOnlyProperty(mExtensionLayoutId, "ExtensionLayoutId", EPropertyFlags::Transient);
    /** @brief GPU 확장 레이아웃 식별자이며 에셋 기본값은 0입니다. */
    Uint32 mExtensionLayoutId{};

    ReflectMembers();
};

/** @brief 에셋에 저장 가능한 유한 값과 범위, 비어 있는 GPU 주소를 검사합니다. @param Data 검사할 재질 데이터입니다. @return 모든 필드가 규약을 만족하면 true입니다. */
bool IsValidMaterialData(const FMaterialGPUData& Data);

static_assert(std::is_standard_layout_v<FMaterialGPUData>);
static_assert(std::is_trivially_copyable_v<FMaterialGPUData>);
static_assert(sizeof(FMaterialGPUData) == 64);
static_assert(alignof(FMaterialGPUData) == 16);
static_assert(offsetof(FMaterialGPUData, mOpacity) == 12);
static_assert(offsetof(FMaterialGPUData, mEmissive) == 16);
static_assert(offsetof(FMaterialGPUData, mMetallic) == 32);
static_assert(offsetof(FMaterialGPUData, mOpacityMaskClipValue) == 48);
static_assert(offsetof(FMaterialGPUData, mExtensionLayoutId) == 60);
