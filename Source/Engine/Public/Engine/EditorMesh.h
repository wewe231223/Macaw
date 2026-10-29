/**
 * @file EditorMesh.h
 * @brief 기즈모에 사용하는 편집기 기본 메시 데이터에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/EEditorMesh.h"
#include "Engine/FMeshData.h"

/**
 * @brief 기즈모 도형의 공유 CPU 메시 데이터를 조회합니다.
 * @param[in] Type 생성하거나 조회할 에디터 도형입니다.
 * @return 공유 메시의 읽기 전용 참조이며 잘못된 종류이면 빈 메시입니다.
 */
const FMeshData& GetEditorMesh(EEditorMesh Type);
