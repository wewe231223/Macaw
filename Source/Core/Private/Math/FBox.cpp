#include "Core/Math/FBox.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FRay.h"
#include "Math/SimdMath.h"

#include <algorithm>
#include <limits>

using namespace Math::Simd;

FBox::FBox(const FVector& Minimum, const FVector& Maximum)
	: mMinimum{Minimum},
	  mMaximum{Maximum} {
}

bool FBox::Intersects(const FRay& Ray, float MaximumDistance) const {
    const __m128 Origin{Ray.mOrigin.mValue};
    const __m128 Direction{Ray.mDirection.mValue};
    const __m128 Minimum{mMinimum.mValue};
    const __m128 Maximum{mMaximum.mValue};
    const __m128 Magnitude{_mm_andnot_ps(_mm_set1_ps(-0.0f), Direction)};
    const __m128 Parallel{_mm_cmple_ps(Magnitude, _mm_set1_ps(0.0000001f))};
    const __m128 Outside{_mm_or_ps(_mm_cmplt_ps(Origin, Minimum), _mm_cmpgt_ps(Origin, Maximum))};

    if ((_mm_movemask_ps(_mm_and_ps(Parallel, Outside)) & 7) != 0) {
        return false;
    }

    const __m128 SafeDirection{_mm_or_ps(_mm_andnot_ps(Parallel, Direction), _mm_and_ps(Parallel, _mm_set1_ps(1.0f)))};
    const __m128 First{_mm_div_ps(_mm_sub_ps(Minimum, Origin), SafeDirection)};
    const __m128 Second{_mm_div_ps(_mm_sub_ps(Maximum, Origin), SafeDirection)};
    const __m128 Near{_mm_or_ps(_mm_andnot_ps(Parallel, _mm_min_ps(First, Second)), _mm_and_ps(Parallel, _mm_set1_ps(-std::numeric_limits<float>::max())))};
    const __m128 Far{_mm_or_ps(_mm_andnot_ps(Parallel, _mm_max_ps(First, Second)), _mm_and_ps(Parallel, _mm_set1_ps(std::numeric_limits<float>::max())))};

    const float Entry{std::max({0.0f, Extract<0>(Near), Extract<1>(Near), Extract<2>(Near)})};
    const float Exit{std::min({MaximumDistance, Extract<0>(Far), Extract<1>(Far), Extract<2>(Far)})};

    return Entry <= Exit;
}

FBox FBox::Transformed(const FMatrix& Matrix) const {
    const __m128 Minimum{mMinimum.mValue};
    const __m128 Maximum{mMaximum.mValue};
    const __m128 Center{_mm_mul_ps(_mm_add_ps(Minimum, Maximum), _mm_set1_ps(0.5f))};
    const __m128 Extent{_mm_mul_ps(_mm_sub_ps(Maximum, Minimum), _mm_set1_ps(0.5f))};
    const __m128 Homogeneous{_mm_add_ps(Center, _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f))};
    const __m128 TransformedCenter{Transform(Homogeneous, Matrix.mRows)};
    const __m128 ExtentX{_mm_mul_ps(_mm_andnot_ps(_mm_set1_ps(-0.0f), Matrix.mRows[0]), Splat<0>(Extent))};
    const __m128 ExtentY{_mm_mul_ps(_mm_andnot_ps(_mm_set1_ps(-0.0f), Matrix.mRows[1]), Splat<1>(Extent))};
    const __m128 ExtentZ{_mm_mul_ps(_mm_andnot_ps(_mm_set1_ps(-0.0f), Matrix.mRows[2]), Splat<2>(Extent))};
    const __m128 TransformedExtent{_mm_add_ps(_mm_add_ps(ExtentX, ExtentY), ExtentZ)};

    return {FVector{_mm_sub_ps(TransformedCenter, TransformedExtent)}, FVector{_mm_add_ps(TransformedCenter, TransformedExtent)}};
}
