#include "Engine/FPrimitiveDrawInterface.h"
#include "Core/Math/FVector.h"
#include "Engine/FDrawLine.h"

void FPrimitiveDrawInterface::DrawLine(const FVector& Start, const FVector& End, const FVector& Color, bool DepthTest) {
    mLines.push_back(FDrawLine{Start, End, Color, DepthTest});
}

void FPrimitiveDrawInterface::Clear() {
    mLines.clear();
}

const TArray<FDrawLine>& FPrimitiveDrawInterface::GetLines() const {
    return mLines;
}
