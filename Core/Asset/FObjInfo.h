#pragma once
#include "PCH.h"

//OBJ의 v/vt/vn 인덱스 조합 하나 = GPU 정점 하나. 같은 조합이 또 나오면 새 정점을 만들지 않고 재사용한다.
struct FFaceVertexKey {
    Int32 mPositionIndex{};
    Int32 mUvIndex{};
    Int32 mNormalIndex{};

    bool operator==(const FFaceVertexKey& Other) const noexcept;
};

struct FFaceVertexKeyHash {
    std::size_t operator()(const FFaceVertexKey& Key) const noexcept;
};

struct FFaceVertex {
    Int32 mPositionIndex{-1};
    Int32 mUvIndex{-1};
    Int32 mNormalIndex{-1};
};

struct FObjInfo {
    FString mAssetName{};
    //이 오브젝트의 머티리얼 정보가 담긴 mtl 파일 이름
    FString mMaterialFileName{};

    TArray<FVector> mPositions{};
    TArray<FVector2> mUVs{};
    TArray<FVector> mNormals{};

    // face 하나를 구성하는 정점 순서대로 : 나중에 인덱스 버퍼의 순서가 된다.
    // v/vt/vn 조합
    //다각형에 대응하기 위해서 1개 면이 저장하는 버텍스들을 TArray로 한번 더 감쌉니다.
    TArray<TArray<FFaceVertex>> mFaceVerticesPolygon{};

    //머티리얼 이름들
    //SubMesh와 인덱스 매칭한다.
    TArray<FString> mMaterialNames{};

    //동일한 머티리얼을 쓰는 정점들의 개수
    //MaterialNames와 인덱스 매칭한다.
    //MaterialNames[0]를 쓰는 정점의 개수는 SubMesh[0]개
    TArray<Int32> mSubMesh{};

    //버텍스 컬러
    TArray<FColor4> mColors{};
};

struct FGeometry {
    TArray<FVector> mPositions{};
    TArray<FVector> mNormals{};
    TArray<FVector2> mTexCoords{};
    TArray<Uint32> mIndices{};

    FString mMaterialFileName{};
    TArray<FString> mMaterialNames{};
    TArray<Uint32> mSubMeshIndexCounts{};

    TArray<FColor4> mColors{};
};
