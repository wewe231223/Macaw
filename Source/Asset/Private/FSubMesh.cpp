/**
 * @file FSubMesh.cpp
 * @brief 재질별 메시 구간의 구현입니다.
 * @ingroup Asset
 */

#include "Asset/FSubMesh.h"

namespace Asset {

    /**
     * @brief 공유 인덱스 버퍼의 구간과 재질을 지정합니다.
     */
    Asset::FSubMesh::FSubMesh(Core::Uint32 FirstIndex, Core::Uint32 IndexCount, Core::Uint32 MaterialIndex)
		: mFirstIndex{FirstIndex},
		  mIndexCount{IndexCount},
		  mMaterialIndex{MaterialIndex} {
    }

    /**
     * @brief 첫 인덱스 오프셋을 반환합니다.
     */
    Core::Uint32 Asset::FSubMesh::GetFirstIndex() const {
        return mFirstIndex;
    }

    /**
     * @brief 인덱스 개수를 반환합니다.
     */
    Core::Uint32 Asset::FSubMesh::GetIndexCount() const {
        return mIndexCount;
    }

    /**
     * @brief 재질 슬롯을 반환합니다.
     */
    Core::Uint32 Asset::FSubMesh::GetMaterialIndex() const {
        return mMaterialIndex;
    }

}
