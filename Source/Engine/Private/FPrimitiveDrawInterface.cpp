/**
 * @file FPrimitiveDrawInterface.cpp
 * @brief 에디터 보조 선분을 프레임 단위로 수집합니다.
 * @ingroup Engine
 */

#include "Engine/FPrimitiveDrawInterface.h"
#include "Core/Math/FVector.h"
#include "Engine/FDrawLine.h"
#include "Core/Math/FBox.h"

#include <cmath>

/**
 * @brief 주어진 좌표와 색상으로 보조 선분을 프레임 목록에 추가합니다.
 */
void FPrimitiveDrawInterface::DrawLine(const FVector& Start, const FVector& End, const FVector& Color, bool DepthTest, float Thickness) {
    if (!Start.IsFinite() || !End.IsFinite() || !Color.IsFinite() || !std::isfinite(Thickness) || Thickness <= 0.0f || (End - Start).LengthSquared() <= 0.0f) {
        return;
    }

    mLines.push_back(FDrawLine{Start, End, Color, DepthTest, Thickness});
}

void FPrimitiveDrawInterface::DrawBox(const FBox& Bounds, const FVector& Color, bool DepthTest, float Thickness) {
    for (Int32 Corner{}; Corner < 8; ++Corner) {
        const FVector Start{(Corner & 1) != 0 ? Bounds.mMaximum.x : Bounds.mMinimum.x, (Corner & 2) != 0 ? Bounds.mMaximum.y : Bounds.mMinimum.y, (Corner & 4) != 0 ? Bounds.mMaximum.z : Bounds.mMinimum.z};

        for (Int32 Axis{}; Axis < 3; ++Axis) {
            if ((Corner & (1 << Axis)) != 0) {
                continue;
            }

            const Int32 Other{Corner | (1 << Axis)};
            const FVector End{(Other & 1) != 0 ? Bounds.mMaximum.x : Bounds.mMinimum.x, (Other & 2) != 0 ? Bounds.mMaximum.y : Bounds.mMinimum.y, (Other & 4) != 0 ? Bounds.mMaximum.z : Bounds.mMinimum.z};
            DrawLine(Start, End, Color, DepthTest, Thickness);
        }
    }
}

/**
 * @brief 현재 프레임의 보조 선분 목록을 비웁니다.
 */
void FPrimitiveDrawInterface::Clear() {
    mLines.clear();
    mMeshes.clear();
    mTexts.clear();
}

/**
 * @brief 현재 프레임에 수집한 보조 선분을 반환합니다.
 */
const TArray<FDrawLine>& FPrimitiveDrawInterface::GetLines() const {
    return mLines;
}

void FPrimitiveDrawInterface::DrawMesh(const FDrawMesh& Mesh) {
    mMeshes.push_back(Mesh);
}

const TArray<FDrawMesh>& FPrimitiveDrawInterface::GetMeshes() const {
    return mMeshes;
}

void FPrimitiveDrawInterface::DrawString(const FDrawText& Text) {
    mTexts.push_back(Text);
}

const TArray<FDrawText>& FPrimitiveDrawInterface::GetTexts() const {
    return mTexts;
}
