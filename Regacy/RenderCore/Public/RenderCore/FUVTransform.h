/**
 * @file FUVTransform.h
 * @brief UV의 배율·오프셋과 머티리얼 그룹별 프리미티브 UV 변환을 정의합니다.
 */

#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "Core/CoreMinimal.h"
#include "Core/Base/FName.h"
#include "CoreUObject/FObjectHandle.h"

/**
 * @brief UV에 성분별 배율을 곱한 뒤 오프셋을 더하는 변환입니다.
 * @note 기본값은 배율 (1, 1), 오프셋 (0, 0)의 항등 변환입니다.
 */
struct FUVTransform {
public:
    ReflectTypes(FUVTransform);

public:
    /**
     * @brief UV 변환의 구조체 리플렉션 메타데이터를 반환합니다.
     * @return FUVTransform의 등록 구조체 정보입니다.
     */
    ReflectStruct(FUVTransform);

public:
    ReflectProperty(mScale, "UVScale", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mScale), ReflectEditor("", "UV Scale"), ReflectRange(-100.0f, 100.0f), ReflectSpeed(0.01f));

    /**
     * @brief U와 V에 각각 곱할 배율입니다.
     */
    FVector2 mScale{1.0f, 1.0f};
    ReflectProperty(mOffset, "UVOffset", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mOffset), ReflectEditor("", "UV Offset"), ReflectRange(-100.0f, 100.0f), ReflectSpeed(0.01f));

    /**
     * @brief 배율 적용 후 U와 V에 각각 더할 오프셋입니다.
     */
    FVector2 mOffset{};

    ReflectMembers();
};

/**
 * @brief 특정 머티리얼 그룹과 변환 원본을 식별하는 UV 변환 레코드입니다.
 */
struct FPrimitiveUVTransform {
    /**
     * @brief 적용할 머티리얼 그룹 이름입니다. None이면 그룹에 대응하지 않는 요청의 기본 변환으로 사용합니다.
     */
    FName mMaterialSlot{};

    /**
     * @brief 그룹에 적용할 UV 배율과 오프셋입니다.
     */
    FUVTransform mTransform{};

    /**
     * @brief 변환을 제공한 원본 객체의 비소유 핸들입니다.
     */
    FObjectHandle mSourceHandle{};
};

static_assert(sizeof(FUVTransform) == 16);
