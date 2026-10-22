#include "FGpuMeshVertex.h"
#include "Engine/FMeshVertex.h"

FGpuMeshVertex::FGpuMeshVertex(const FMeshVertex& Vertex)
	: mPosition{Vertex.mPosition.ToArray()},
	  mNormal{Vertex.mNormal.ToArray()} {
}
