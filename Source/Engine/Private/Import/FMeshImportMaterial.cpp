/**
 * @file FMeshImportMaterial.cpp
 * @brief 메시 임포트에서 읽은 재질 이름·기본색·양면 설정과 텍스처 참조입니다.
 * @ingroup Engine
 */

#include "Engine/Import/FMeshImportMaterial.h"

#include <utility>

namespace Engine {

    Engine::FMeshImportMaterial::FMeshImportMaterial(Core::FString Name, const Core::FVector4& BaseColor, bool TwoSided, Engine::FMeshImportTexture BaseColorTexture, Engine::FMeshImportTexture NormalMapTexture)
		: mName{std::move(Name)},
		  mBaseColor{BaseColor},
		  mTwoSided{TwoSided},
		  mBaseColorTexture{std::move(BaseColorTexture)},
		  mNormalMapTexture{std::move(NormalMapTexture)} {
    }

    const Core::FString& Engine::FMeshImportMaterial::GetName() const {
        return mName;
    }

    const Core::FVector4& Engine::FMeshImportMaterial::GetBaseColor() const {
        return mBaseColor;
    }

    bool Engine::FMeshImportMaterial::IsTwoSided() const {
        return mTwoSided;
    }

    const Engine::FMeshImportTexture& Engine::FMeshImportMaterial::GetBaseColorTexture() const {
        return mBaseColorTexture;
    }

    const Engine::FMeshImportTexture& Engine::FMeshImportMaterial::GetNormalMapTexture() const {
        return mNormalMapTexture;
    }

}
