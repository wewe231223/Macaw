#include "Renderer/FLineVertex.h"
#include "Core/Math/FVector.h"

FLineVertex::FLineVertex(const FVector& Position, const FVector& Color)
	: mPosition{Position.ToArray()},
	  mColor{Color.ToArray()} {
}
