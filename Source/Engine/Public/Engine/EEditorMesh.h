/**
 * @file EEditorMesh.h
 * @brief 편집기 기본 메시 종류와 열거형 리플렉션에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

/**
 * @brief 렌더러의 고정 GPU 메시와 CPU 피킹 형상을 함께 식별하는 에디터 메시 종류입니다.
 * @ingroup Engine
 */
enum class EEditorMesh : Uint8 {
    /**
     * @brief 이동용 원통 축과 원뿔 화살촉 메시입니다.
     */
    Arrow,
    /**
     * @brief 스케일용 사각 축과 상자 머리 메시입니다.
     */
    Hammer,
    /**
     * @brief 화면 기준 회전 또는 활성 회전에 사용하는 전체 링 메시입니다.
     */
    Ring,
    /**
     * @brief 앞뒤 회전 반원을 나누는 절반 링 메시입니다.
     */
    HalfRing,
    /**
     * @brief 평면 이동과 화면 기준 이동에 사용하는 사각형 메시입니다.
     */
    Quad,
    /**
     * @brief 두 축 스케일 조작에 사용하는 삼각형 메시입니다.
     */
    Triangle,
    /**
     * @brief 중앙 균일 스케일 핸들의 상자 메시입니다.
     */
    Cube,
    /**
     * @brief 고정 에디터 메시 종류의 개수이며 실제 형상이나 그리기 요청이 없음을 나타냅니다.
     */
    Count
};

/**
 * @brief 고정 에디터 메시 열거형의 정적 메타데이터를 반환합니다.
 * @return 등록된 열거형 메타데이터의 비소유 주소입니다.
 */
ReflectEnum(EEditorMesh, Arrow, Hammer, Ring, HalfRing, Quad, Triangle, Cube);
/**
 * @fn UEnum* GetNativeEnum(TNativeEnum<EEditorMesh>)
 * @brief 타입 태그를 통해 에디터 메시 열거형의 메타데이터를 조회합니다.
 * @return 등록된 열거형 메타데이터의 비소유 주소입니다.
 */
