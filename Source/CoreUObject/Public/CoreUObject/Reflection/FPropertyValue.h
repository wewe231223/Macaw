/**
 * @file FPropertyValue.h
 * @brief 프로퍼티가 지원하는 스칼라·수학 타입과 불투명 주소의 variant입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/Guid.h"
#include "Core/Math.h"

#include <variant>

/**
 * @brief 프로퍼티가 지원하는 스칼라·수학 타입과 불투명 주소의 variant입니다.
 */
using FPropertyValue = std::variant<bool, Int32, Uint32, Uint64, float, FString, FGuid, FVector2, FVector, FVector4, FQuat, FRotator, FMatrix, FTransform, FBox, FRay, const void*>;

/**
 * @brief 프로퍼티 variant에 저장된 실제 값의 주소를 반환합니다.
 * @param[in,out] Value 프로퍼티 타입에 대응하는 variant 값입니다.
 * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
 */
void* GetPropertyData(FPropertyValue& Value);
