/**
 * @file FRay.cpp
 * @brief 원점과 방향으로 정의하는 월드 공간 광선입니다.
 * @ingroup Core
 */

#include "Core/Math/FRay.h"

/**
 * @brief FRay의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FRay::FRay(const FVector& Origin, const FVector& Direction)
	: mOrigin{Origin},
	  mDirection{Direction} {
}
