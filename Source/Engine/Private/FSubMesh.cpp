/**
 * @file FSubMesh.cpp
 * @brief 재질별 메시 구간의 구현입니다.
 * @ingroup Engine
 */
#include "Engine/FSubMesh.h"

FSubMesh::FSubMesh(Uint32 FirstIndex, Uint32 IndexCount, Uint32 MaterialIndex)
	: mFirstIndex{FirstIndex},
	  mIndexCount{IndexCount},
	  mMaterialIndex{MaterialIndex} {
}

Uint32 FSubMesh::GetFirstIndex() const {
    return mFirstIndex;
}

Uint32 FSubMesh::GetIndexCount() const {
    return mIndexCount;
}

Uint32 FSubMesh::GetMaterialIndex() const {
    return mMaterialIndex;
}
