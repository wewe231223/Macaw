#include "pch.h"
#include "FViewportGeometry.h"

Int32 FRect::GetWidth() const {
    return mMax.mX - mMin.mX;
}

Int32 FRect::GetHeight() const {
    return mMax.mY - mMin.mY;
}

bool FRect::IsEmpty() const {
    return GetWidth() <= 0 || GetHeight() <= 0;
}

bool FRect::Contains(const FPoint& Point) const {
    return Point.mX >= mMin.mX && Point.mX < mMax.mX && Point.mY >= mMin.mY && Point.mY < mMax.mY;
}
