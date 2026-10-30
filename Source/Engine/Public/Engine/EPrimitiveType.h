/**
 * @file EPrimitiveType.h
 * @brief 기본 생성 및 렌더링에 지원하는 프리미티브 메시 종류입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

/**
 * @brief 기본 생성 및 렌더링에 지원하는 프리미티브 메시 종류입니다.
 * @ingroup Engine
 */
enum class EPrimitiveType : Uint8 {
    /**
     * @brief 기본 평면 메시입니다.
     */
    Plane,
    /**
     * @brief 기본 정육면체 메시입니다.
     */
    Cube,
    /**
     * @brief 기본 구 메시입니다.
     */
    Sphere,
    /**
     * @brief 지원하는 프리미티브 종류의 개수이며 실제 메시 종류가 아닙니다.
     */
    Count
};

/**
 * @brief 이 열거형의 정적 메타데이터를 반환합니다.
 * @return 등록된 타입 메타데이터의 비소유 주소입니다.
 */
ReflectEnum(EPrimitiveType, Plane, Cube, Sphere);

/**
 * @fn UEnum* GetNativeEnum(TNativeEnum<EPrimitiveType>)
 * @brief 타입 태그를 통해 이 열거형의 메타데이터를 조회합니다.
 * @return 등록된 열거형 메타데이터의 비소유 주소입니다.
 */
