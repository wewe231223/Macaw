/**
 * @file FSubMesh.cpp
 * @brief 재질별 메시 구간의 구현입니다.
 * @ingroup Engine
 */
#include "Engine/FSubMesh.h"

namespace Engine {

    Engine::FSubMesh::FSubMesh(Core::Uint32 FirstIndex, Core::Uint32 IndexCount, Core::Uint32 MaterialIndex)
		: mFirstIndex{FirstIndex},
		  mIndexCount{IndexCount},
		  mMaterialIndex{MaterialIndex} {
    }

    Core::Uint32 Engine::FSubMesh::GetFirstIndex() const {
        return mFirstIndex;
    }

    Core::Uint32 Engine::FSubMesh::GetIndexCount() const {
        return mIndexCount;
    }

    Core::Uint32 Engine::FSubMesh::GetMaterialIndex() const {
        return mMaterialIndex;
    }

}
