#define MacawInstanced 1
#include "MeshVertex.hlsli"

FPixelInput VertexMain(FMeshVertexInput Input) {
    return BuildPixelInput(Input);
}
