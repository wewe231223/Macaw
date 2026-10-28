/**
 * @file SpriteGeometry.cpp
 * @brief 렌더링·선택·경계 계산이 공유하는 스프라이트 정렬 행렬을 제공합니다.
 * @ingroup Engine
 */

#include "Engine/SpriteGeometry.h"
#include "Engine/FDrawSprite.h"
#include "Engine/FSceneView.h"

FMatrix MakeSpriteWorld(const FMatrix& World, const FDrawSprite& Sprite, const FSceneView& View) {
    FMatrix Result{};
    const float HorizontalScale{World.TransformVector({0.0f, 1.0f, 0.0f}).Length()};
    const float VerticalScale{World.TransformVector({0.0f, 0.0f, 1.0f}).Length()};
    for (Int32 Column{}; Column < 3; ++Column) {
        Result(0, Column) = Sprite.mSize.x * (Sprite.mScreenAligned ? View.mView(Column, 0) * HorizontalScale : World(1, Column));
        Result(1, Column) = Sprite.mSize.y * (Sprite.mScreenAligned ? View.mView(Column, 1) * VerticalScale : World(2, Column));
        Result(2, Column) = Sprite.mScreenAligned ? -View.mView(Column, 2) : -World(0, Column);
        Result(3, Column) = World(3, Column);
    }
    return Result;
}
