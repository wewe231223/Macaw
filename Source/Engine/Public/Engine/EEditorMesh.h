#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

/**
 * @brief 렌더러의 고정 GPU 메시와 CPU 피킹 형상을 함께 식별하는 에디터 메시 종류입니다.
 * @ingroup Engine
 */
enum class EEditorMesh : Uint8 {
    Arrow,
    Hammer,
    Ring,
    HalfRing,
    Quad,
    Triangle,
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
