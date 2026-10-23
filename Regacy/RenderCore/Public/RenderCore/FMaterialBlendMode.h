/**
 * @file FMaterialBlendMode.h
 * @brief 머티리얼의 불투명·반투명 렌더링 분류를 정의합니다.
 */
#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include "Core/Common.h"

/**
 * @brief 머티리얼을 불투명 또는 반투명 그리기 경로로 분류하는 값입니다.
 */
enum class EMaterialBlendMode : Uint8 {
    /**
     * @brief 불투명 머티리얼 그리기 경로입니다.
     */
    Opaque,
    /**
     * @brief 알파 혼합을 사용하는 반투명 머티리얼 그리기 경로입니다.
     */
    Translucent
};

ReflectEnum(EMaterialBlendMode, ReflectEnumValue(EMaterialBlendMode, Opaque), ReflectEnumValue(EMaterialBlendMode, Translucent));
