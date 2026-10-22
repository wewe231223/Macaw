#include "Core/Math/FRay.h"

FRay::FRay(const FVector& Origin, const FVector& Direction)
	: mOrigin{Origin},
	  mDirection{Direction} {
}
