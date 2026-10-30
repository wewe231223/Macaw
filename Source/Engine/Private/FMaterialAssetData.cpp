/**
 * @file FMaterialAssetData.cpp
 * @brief 재질의 저장 가능한 설정을 보관하고 직렬화·참조 검증·런타임 재질 적용을 처리합니다.
 * @ingroup Engine
 */

#include "FMaterialAssetData.h"
#include "Core/FilePath.h"
#include "Engine/FAssetManager.h"
#include "Engine/UMaterial.h"
#include "Serialization/MemoryArchive.h"

namespace Engine {

    Engine::FMaterialAssetData::FMaterialAssetData() = default;

    Engine::FMaterialAssetData::FMaterialAssetData(const Engine::UMaterial& Material)
		: mName{Material.GetName()},
		  mShaderPath{Material.GetShaderPath()},
		  mBaseColor{Material.GetBaseColor()},
		  mBaseColorTexturePath{Material.GetBaseColorTexturePath()},
		  mNormalMapTexturePath{Material.GetNormalMapTexturePath()},
		  mSpecularStrength{Material.GetSpecularStrength()},
		  mShininess{Material.GetShininess()},
		  mNormalStrength{Material.GetNormalStrength()},
		  mTwoSided{Material.IsTwoSided()} {
    }

    void Engine::FMaterialAssetData::Serialize(Serialization::FMemoryArchive& Archive) {
        Core::Uint32 Version{1};
        Archive.Serialize("Version", Version);

        if (Archive.HasError() || Version != 1) {
            Archive.SetError("Unsupported material asset version");
            return;
        }

        Archive.Serialize("Name", mName);
        Archive.Serialize("Shader", mShaderPath);
        Archive.Serialize("BaseColor", mBaseColor);
        Archive.Serialize("BaseColorTexture", mBaseColorTexturePath);
        Archive.Serialize("NormalMap", mNormalMapTexturePath);
        Archive.Serialize("SpecularStrength", mSpecularStrength);
        Archive.Serialize("Shininess", mShininess);
        Archive.Serialize("NormalStrength", mNormalStrength);
        Archive.Serialize("TwoSided", mTwoSided);
    }

    bool Engine::FMaterialAssetData::ApplyTo(Engine::UMaterial& Material, Core::FString& Error) const {
        if (!Material.SetName(mName) || !Material.SetShaderPath(mShaderPath) || !Material.SetBaseColor(mBaseColor) || !Material.SetBaseColorTexturePath(mBaseColorTexturePath) || !Material.SetNormalMapTexturePath(mNormalMapTexturePath) || !Material.SetSpecularStrength(mSpecularStrength) || !Material.SetShininess(mShininess) || !Material.SetNormalStrength(mNormalStrength) || !Material.SetTwoSided(mTwoSided)) {
            Error = "Invalid material asset settings";
            return false;
        }

        return true;
    }

    bool Engine::FMaterialAssetData::ValidateReferences(const Engine::FAssetManager& Assets, Core::FString& Error) const {
        if (mName.empty() || mName.size() > 255 || mName.find('\0') != Core::FString::npos) {
            Error = "Invalid material asset name";
            return false;
        }

        for (const Core::FString* Reference : {&mShaderPath, &mBaseColorTexturePath, &mNormalMapTexturePath}) {
            if (!Reference->empty() && Assets.ResolvePath(Core::FilePath::FromUtf8(*Reference), Error).empty()) {
                return false;
            }
        }

        return true;
    }

}
