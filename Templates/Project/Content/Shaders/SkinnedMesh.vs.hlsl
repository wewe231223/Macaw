// include보다 먼저 정의해야 본 팔레트와 FMeshVertexInput의 본 입력이 함께 컴파일됩니다.
#define MacawSkinned 1
#include "Skinning.hlsl"
#include "MeshVertex.hlsli"

FPixelInput VertexMain(FMeshVertexInput Input) {
    // 바인드 자세의 정점을 현재 포즈로 옮깁니다. 이후 처리는 정적 메시와 같습니다.
    SkinVertex(Input.mPosition, Input.mNormal, Input.mTangent, Input.mBoneIndices, Input.mBoneWeights);

    return BuildPixelInput(Input);
}
