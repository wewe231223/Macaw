#pragma once

#include <cstdint>

#include "Core/Base/FObjectHandle.h"
#include "FMath.h"

struct FEditorSelectionState {
	FObjectHandle TransformTargetHandle{};
	FObjectHandle PickedColliderHandle{};

	FMatrix TargetWorld{ FMatrix::Identity };
	FMatrix ColliderWorld{ FMatrix::Identity };

	FVector3 BoundsCenter{};
	FVector3 BoundsExtent{};
	FQuat BoundsOrientation{};

	std::uint64_t TransformRevision = 0;
};
