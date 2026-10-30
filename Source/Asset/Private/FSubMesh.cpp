/**
 * @file FSubMesh.cpp
 * @brief 재질별 메시 구간의 구현입니다.
 * @ingroup Asset
 */
#include "Asset/FSubMesh.h"

namespace Asset {

    Asset::FSubMesh::FSubMesh(Core::Uint32 FirstIndex, Core::Uint32 IndexCount, Core::Uint32 MaterialIndex)
		: mFirstIndex{FirstIndex},
		  mIndexCount{IndexCount},
		  mMaterialIndex{MaterialIndex} {
    }

    Core::Uint32 Asset::FSubMesh::GetFirstIndex() const {
        return mFirstIndex;
    }

    Core::Uint32 Asset::FSubMesh::GetIndexCount() const {
        return mIndexCount;
    }

    Core::Uint32 Asset::FSubMesh::GetMaterialIndex() const {
        return mMaterialIndex;
    }

}
