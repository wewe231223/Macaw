/**
 * @file UMaterial.cpp
 * @brief 셰이더 경로·기본색·텍스처·양면 정책과 GPU 상수를 관리합니다.
 * @ingroup Asset
 */

#include "Asset/UMaterial.h"
#include "Core/FilePath.h"
#include "Asset/FAssetManager.h"
#include "Asset/UTexture2D.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

namespace Asset {

    /**
     * @brief 머티리얼 경로 검증과 GPU 상수 표현의 내부 구현입니다.
     */
    namespace {
        /**
         * @brief 절대 경로와 상위 폴더 이동을 제외한 Content 상대 경로인지 검사합니다.
         * @param[in] Path 검사할 경로 문자열입니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool IsContentPath(const Core::FString& Path) {
            if (Path.empty() || Path.find('\0') != Core::FString::npos || Path.find_first_of("\r\n#\"") != Core::FString::npos) {
                return false;
            }

            const std::filesystem::path File{Core::FilePath::FromUtf8(Path)};

            if (File.is_absolute() || File.has_root_path()) {
                return false;
            }

            return std::none_of(File.begin(), File.end(), [](const auto& Part) {
                return Part == L"..";
            });
        }
    }

    Asset::UMaterial::UMaterial() = default;

    Asset::UMaterial::~UMaterial() = default;

    const Core::FString& Asset::UMaterial::GetShaderPath() const {
        return mShaderPath;
    }

    bool Asset::UMaterial::SetShaderPath(const Core::FString& Path) {
        if (!IsContentPath(Path) || !Path.ends_with(".hlsl")) {
            return false;
        }

        mShaderPath = Path;

        return true;
    }

    const Core::FVector4& Asset::UMaterial::GetBaseColor() const {
        return mBaseColor;
    }

    bool Asset::UMaterial::SetBaseColor(const Core::FVector4& Color) {
        const auto Values{Color.ToArray()};

        if (!std::all_of(Values.begin(), Values.end(), [](float Value) {
            return std::isfinite(Value) && Value >= 0.0f && Value <= 1.0f;
        })) {
            return false;
        }

        if (mBaseColor != Color) {
            mBaseColor = Color;
            mConstantsDirty = true;
        }

        return true;
    }

    const Core::FString& Asset::UMaterial::GetBaseColorTexturePath() const {
        return mBaseColorTexturePath;
    }

    bool Asset::UMaterial::SetBaseColorTexturePath(const Core::FString& Path) {
        if (!Path.empty() && !IsContentPath(Path)) {
            return false;
        }

        if (mBaseColorTexturePath != Path) {
            mBaseColorTexturePath = Path;
            mBaseColorTexture = nullptr;
        }

        return true;
    }

    bool Asset::UMaterial::IsTwoSided() const {
        return mTwoSided;
    }

    bool Asset::UMaterial::SetTwoSided(bool TwoSided) {
        mTwoSided = TwoSided;
        return true;
    }

    Asset::UTexture2D* Asset::UMaterial::GetBaseColorTexture() const {
        return mBaseColorTexture;
    }

    bool Asset::UMaterial::ResolveTexture(Asset::FAssetManager& Assets, Core::FString& Error) {
        Error.clear();

        Asset::UTexture2D* BaseTexture{mBaseColorTexturePath.empty() ? nullptr : Assets.LoadTexture(Core::FilePath::FromUtf8(mBaseColorTexturePath), Error)};

        if (!mBaseColorTexturePath.empty() && BaseTexture == nullptr) {
            return false;
        }

        Asset::UTexture2D* NormalTexture{mNormalMapTexturePath.empty() ? nullptr : Assets.LoadNormalMap(Core::FilePath::FromUtf8(mNormalMapTexturePath), Error)};

        if (!mNormalMapTexturePath.empty() && NormalTexture == nullptr) {
            return false;
        }

        Asset::UTexture2D* SpecularTexture{mSpecularTexturePath.empty() ? nullptr : Assets.LoadTexture(Core::FilePath::FromUtf8(mSpecularTexturePath), Error)};

        if (!mSpecularTexturePath.empty() && SpecularTexture == nullptr) {
            return false;
        }

        Asset::UTexture2D* EmissiveTexture{mEmissiveTexturePath.empty() ? nullptr : Assets.LoadTexture(Core::FilePath::FromUtf8(mEmissiveTexturePath), Error)};

        if (!mEmissiveTexturePath.empty() && EmissiveTexture == nullptr) {
            return false;
        }

        Asset::UTexture2D* ReflectionTexture{mReflectionTexturePath.empty() ? nullptr : Assets.LoadTexture(Core::FilePath::FromUtf8(mReflectionTexturePath), Error)};

        if (!mReflectionTexturePath.empty() && ReflectionTexture == nullptr) {
            return false;
        }

        mSpecularTexture = SpecularTexture;
        mEmissiveTexture = EmissiveTexture;
        mReflectionTexture = ReflectionTexture;
        mConstantsDirty = true;

        mBaseColorTexture = BaseTexture;
        mNormalMapTexture = NormalTexture;

        return true;
    }

    bool Asset::UMaterial::CreateResources(ID3D11Device* Device, ID3D11DeviceContext* Context) {
        if (Device == nullptr || Context == nullptr || (!mBaseColorTexturePath.empty() && mBaseColorTexture == nullptr) || (mBaseColorTexture != nullptr && !mBaseColorTexture->CreateResources(Device)) || (!mNormalMapTexturePath.empty() && mNormalMapTexture == nullptr) || (mNormalMapTexture != nullptr && !mNormalMapTexture->CreateResources(Device))) {
            return false;
        }

        const std::array<Asset::UTexture2D*, 3> Textures{mSpecularTexture, mEmissiveTexture, mReflectionTexture};
        const std::array<const Core::FString*, 3> Paths{&mSpecularTexturePath, &mEmissiveTexturePath, &mReflectionTexturePath};

        for (std::size_t Index{}; Index < Textures.size(); ++Index) {
            if ((!Paths[Index]->empty() && Textures[Index] == nullptr) || (Textures[Index] != nullptr && !Textures[Index]->CreateResources(Device))) {
                return false;
            }
        }

        const std::array<float, 28> Color{mBaseColor.x, mBaseColor.y, mBaseColor.z, mBaseColor.w, mSpecularStrength, mShininess, mNormalStrength, mReflectionStrength, mSpecularColor.x, mSpecularColor.y, mSpecularColor.z, mSpecularColor.w, mAmbientColor.x, mAmbientColor.y, mAmbientColor.z, mAmbientColor.w, mEmissiveColor.x, mEmissiveColor.y, mEmissiveColor.z, mEmissiveColor.w, mReflectionColor.x, mReflectionColor.y, mReflectionColor.z, mReflectionColor.w, mSpecularTexture != nullptr ? 1.0f : 0.0f, mEmissiveTexture != nullptr ? 1.0f : 0.0f, mReflectionTexture != nullptr ? 1.0f : 0.0f, 0.0f};

        if (mConstantBuffer == nullptr) {
            D3D11_BUFFER_DESC Description{};
            Description.ByteWidth = sizeof(Color);
            Description.Usage = D3D11_USAGE_DEFAULT;
            Description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

            D3D11_SUBRESOURCE_DATA Data{};
            Data.pSysMem = Color.data();

            if (FAILED(Device->CreateBuffer(&Description, &Data, mConstantBuffer.GetAddressOf()))) {
                return false;
            }
        } else if (mConstantsDirty) {
            Context->UpdateSubresource(mConstantBuffer.Get(), 0, nullptr, Color.data(), 0, 0);
        }

        mConstantsDirty = false;

        return true;
    }

    void Asset::UMaterial::ReleaseResources() {
        mConstantBuffer.Reset();
        mConstantsDirty = true;
    }

    ID3D11Buffer* Asset::UMaterial::GetConstantBuffer() const {
        return mConstantBuffer.Get();
    }

    const Core::FString& Asset::UMaterial::GetNormalMapTexturePath() const {
        return mNormalMapTexturePath;
    }

    bool Asset::UMaterial::SetNormalMapTexturePath(const Core::FString& Path) {
        if (!Path.empty() && !IsContentPath(Path)) {
            return false;
        }

        if (mNormalMapTexturePath != Path) {
            mNormalMapTexturePath = Path;
            mNormalMapTexture = nullptr;
        }

        return true;
    }

    Asset::UTexture2D* Asset::UMaterial::GetNormalMapTexture() const {
        return mNormalMapTexture;
    }

    float Asset::UMaterial::GetSpecularStrength() const {
        return mSpecularStrength;
    }

    bool Asset::UMaterial::SetSpecularStrength(float Strength) {
        if (!std::isfinite(Strength) || Strength < 0.0f || Strength > 1.0f) {
            return false;
        }

        mSpecularStrength = Strength;
        mConstantsDirty = true;

        return true;
    }

    float Asset::UMaterial::GetShininess() const {
        return mShininess;
    }

    bool Asset::UMaterial::SetShininess(float Shininess) {
        if (!std::isfinite(Shininess) || Shininess < 1.0f || Shininess > 1024.0f) {
            return false;
        }

        mShininess = Shininess;
        mConstantsDirty = true;

        return true;
    }

    float Asset::UMaterial::GetNormalStrength() const {
        return mNormalStrength;
    }

    bool Asset::UMaterial::SetNormalStrength(float Strength) {
        if (!std::isfinite(Strength) || Strength < 0.0f || Strength > 8.0f) {
            return false;
        }

        mNormalStrength = Strength;
        mConstantsDirty = true;

        return true;
    }

    const Core::FVector4& Asset::UMaterial::GetSpecularColor() const {
        return mSpecularColor;
    }

    bool Asset::UMaterial::SetSpecularColor(const Core::FVector4& Color) {
        if (!Color.IsFinite() || Color.x < 0.0f || Color.y < 0.0f || Color.z < 0.0f || Color.w < 0.0f || Color.w > 1.0f) {
            return false;
        }

        mSpecularColor = Color;
        mConstantsDirty = true;
        return true;
    }

    const Core::FVector4& Asset::UMaterial::GetAmbientColor() const {
        return mAmbientColor;
    }

    bool Asset::UMaterial::SetAmbientColor(const Core::FVector4& Color) {
        if (!Color.IsFinite() || Color.x < 0.0f || Color.y < 0.0f || Color.z < 0.0f || Color.w < 0.0f || Color.w > 1.0f) {
            return false;
        }

        mAmbientColor = Color;
        mConstantsDirty = true;
        return true;
    }

    const Core::FVector4& Asset::UMaterial::GetEmissiveColor() const {
        return mEmissiveColor;
    }

    bool Asset::UMaterial::SetEmissiveColor(const Core::FVector4& Color) {
        if (!Color.IsFinite() || Color.x < 0.0f || Color.y < 0.0f || Color.z < 0.0f || Color.w < 0.0f || Color.w > 1.0f) {
            return false;
        }

        mEmissiveColor = Color;
        mConstantsDirty = true;
        return true;
    }

    const Core::FVector4& Asset::UMaterial::GetReflectionColor() const {
        return mReflectionColor;
    }

    bool Asset::UMaterial::SetReflectionColor(const Core::FVector4& Color) {
        if (!Color.IsFinite() || Color.x < 0.0f || Color.y < 0.0f || Color.z < 0.0f || Color.w < 0.0f || Color.w > 1.0f) {
            return false;
        }

        mReflectionColor = Color;
        mConstantsDirty = true;
        return true;
    }

    float Asset::UMaterial::GetReflectionStrength() const {
        return mReflectionStrength;
    }

    bool Asset::UMaterial::SetReflectionStrength(float Strength) {
        if (!std::isfinite(Strength) || Strength < 0.0f || Strength > 1.0f) {
            return false;
        }

        mReflectionStrength = Strength;
        mConstantsDirty = true;
        return true;
    }

    const Core::FString& Asset::UMaterial::GetSpecularTexturePath() const {
        return mSpecularTexturePath;
    }

    bool Asset::UMaterial::SetSpecularTexturePath(const Core::FString& Path) {
        if (!Path.empty() && !IsContentPath(Path)) {
            return false;
        }

        if (mSpecularTexturePath != Path) {
            mSpecularTexturePath = Path;
            mSpecularTexture = nullptr;
            mConstantsDirty = true;
        }

        return true;
    }

    Asset::UTexture2D* Asset::UMaterial::GetSpecularTexture() const {
        return mSpecularTexture;
    }

    const Core::FString& Asset::UMaterial::GetEmissiveTexturePath() const {
        return mEmissiveTexturePath;
    }

    bool Asset::UMaterial::SetEmissiveTexturePath(const Core::FString& Path) {
        if (!Path.empty() && !IsContentPath(Path)) {
            return false;
        }

        if (mEmissiveTexturePath != Path) {
            mEmissiveTexturePath = Path;
            mEmissiveTexture = nullptr;
            mConstantsDirty = true;
        }

        return true;
    }

    Asset::UTexture2D* Asset::UMaterial::GetEmissiveTexture() const {
        return mEmissiveTexture;
    }

    const Core::FString& Asset::UMaterial::GetReflectionTexturePath() const {
        return mReflectionTexturePath;
    }

    bool Asset::UMaterial::SetReflectionTexturePath(const Core::FString& Path) {
        if (!Path.empty() && !IsContentPath(Path)) {
            return false;
        }

        if (mReflectionTexturePath != Path) {
            mReflectionTexturePath = Path;
            mReflectionTexture = nullptr;
            mConstantsDirty = true;
        }

        return true;
    }

    Asset::UTexture2D* Asset::UMaterial::GetReflectionTexture() const {
        return mReflectionTexture;
    }

}
