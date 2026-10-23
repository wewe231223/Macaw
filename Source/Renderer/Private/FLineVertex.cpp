/**
 * @file FLineVertex.cpp
 * @brief GPU 입력 레이아웃에 맞춘 24바이트 위치·색상 선분 정점입니다.
 * @ingroup Renderer
 */

#include "Renderer/FLineVertex.h"
#include "Core/Math/FVector.h"

/**
 * @brief FLineVertex의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FLineVertex::FLineVertex(const FVector& Position, const FVector& Color)
	: mPosition{Position.ToArray()},
	  mColor{Color.ToArray()} {
}
