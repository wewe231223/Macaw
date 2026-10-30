/**
 * @file ELightType.h
 * @brief 환경광·방향광·점광원·스포트라이트를 구분합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

/**
 * @brief 환경광·방향광·점광원·스포트라이트를 구분합니다.
 * @ingroup Engine
 */
enum class ELightType : Uint8 {
    /**
     * @brief 방향과 위치에 무관한 환경광입니다.
     */
    Ambient,
    /**
     * @brief 일정한 방향으로 비추는 무한 거리 광원입니다.
     */
    Directional,
    /**
     * @brief 지정한 반경 안을 모든 방향으로 비추는 점광원입니다.
     */
    Point,
    /**
     * @brief 원뿔 범위 안을 비추는 스포트라이트입니다.
     */
    Spot
};

/**
 * @brief 광원 종류의 정적 열거형 메타데이터를 반환합니다.
 * @return 등록된 열거형 메타데이터의 비소유 주소입니다.
 */
ReflectEnum(ELightType, Ambient, Directional, Point, Spot);
/**
 * @fn UEnum* GetNativeEnum(TNativeEnum<ELightType>)
 * @brief 타입 태그에 대응하는 광원 종류의 메타데이터를 반환합니다.
 * @return 등록된 열거형 메타데이터의 비소유 주소입니다.
 */
