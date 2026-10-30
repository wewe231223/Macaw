/**
 * @file FMeshImportResult.cpp
 * @brief 정점·인덱스·서브메시와 원본 재질 목록을 함께 보관하는 메시 임포트 결과입니다.
 * @ingroup Engine
 */

#include "Engine/Import/FMeshImportResult.h"

namespace Engine {

    Engine::FMeshData& Engine::FMeshImportResult::GetMeshData() {
        return mMeshData;
    }

    const Engine::FMeshData& Engine::FMeshImportResult::GetMeshData() const {
        return mMeshData;
    }

    Core::TArray<Engine::FMeshImportMaterial>& Engine::FMeshImportResult::GetMaterials() {
        return mMaterials;
    }

    const Core::TArray<Engine::FMeshImportMaterial>& Engine::FMeshImportResult::GetMaterials() const {
        return mMaterials;
    }

}
