/**
 * @file FPrimitiveDrawInterface.cpp
 * @brief 에디터 보조 선분을 프레임 단위로 수집합니다.
 * @ingroup Engine
 */

#include "Engine/FPrimitiveDrawInterface.h"
#include "Core/Math/FVector.h"
#include "Engine/FDrawLine.h"

/**
 * @brief 주어진 좌표와 색상으로 보조 선분을 프레임 목록에 추가합니다.
 */
void FPrimitiveDrawInterface::DrawLine(const FVector& Start, const FVector& End, const FVector& Color, bool DepthTest) {
    mLines.push_back(FDrawLine{Start, End, Color, DepthTest});
}

/**
 * @brief 현재 프레임의 보조 선분 목록을 비웁니다.
 */
void FPrimitiveDrawInterface::Clear() {
    mLines.clear();
}

/**
 * @brief 현재 프레임에 수집한 보조 선분을 반환합니다.
 */
const TArray<FDrawLine>& FPrimitiveDrawInterface::GetLines() const {
    return mLines;
}
