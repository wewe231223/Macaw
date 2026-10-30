/**
 * @file UMaterial.cpp
 * @brief 셰이더 경로·기본색·텍스처·양면 정책과 GPU 상수를 관리합니다.
 * @ingroup Engine
 */

#include "Engine/UMaterial.h"
#include "Core/FilePath.h"
#include "Engine/FAssetManager.h"
#include "Engine/UTexture2D.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

namespace Engine {

    /**
     * @brief 머티리얼 경로 검증과 GPU 상수 표현의 내부 구현입니다.
     */
    namespace {
        /**
         * @brief 절대 경로와 상위 폴더 이동을 제외한 Content 상대 경로인지 검사합니다.
         * @param[in] Path Content 상대 에셋 경로 또는 작업할 파일 경로입니다.
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

    Engine::UMaterial::UMaterial() = default;

    Engine::UMaterial::~UMaterial() = default;

    const Core::FString& Engine::UMaterial::GetShaderPath() const {
        return mShaderPath;
    }

    bool Engine::UMaterial::SetShaderPath(const Core::FString& Path) {
        if (!IsContentPath(Path) || !Path.ends_with(".hlsl")) {
            return false;
        }

        mShaderPath = Path;

        return true;
    }

    const Core::FVector4& Engine::UMaterial::GetBaseColor() const {
        return mBaseColor;
    }

    bool Engine::UMaterial::SetBaseColor(const Core::FVector4& Color) {
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

    const Core::FString& Engine::UMaterial::GetBaseColorTexturePath() const {
        return mBaseColorTexturePath;
    }

    bool Engine::UMaterial::SetBaseColorTexturePath(const Core::FString& Path) {
        if (!Path.empty() && !IsContentPath(Path)) {
            return false;
        }

        if (mBaseColorTexturePath != Path) {
            mBaseColorTexturePath = Path;
            mBaseColorTexture = nullptr;
        }

        return true;
    }

    bool Engine::UMaterial::IsTwoSided() const {
        return mTwoSided;
    }

    bool Engine::UMaterial::SetTwoSided(bool TwoSided) {
        mTwoSided = TwoSided;
        return true;
    }

    Engine::UTexture2D* Engine::UMaterial::GetBaseColorTexture() const {
        return mBaseColorTexture;
    }

    bool Engine::UMaterial::ResolveTexture(Engine::FAssetManager& Assets, Core::FString& Error) {
        Error.clear();

        Engine::UTexture2D* BaseTexture{mBaseColorTexturePath.empty() ? nullptr : Assets.LoadTexture(Core::FilePath::FromUtf8(mBaseColorTexturePath), Error)};

        if (!mBaseColorTexturePath.empty() && BaseTexture == nullptr) {
            return false;
        }

        Engine::UTexture2D* NormalTexture{mNormalMapTexturePath.empty() ? nullptr : Assets.LoadNormalMap(Core::FilePath::FromUtf8(mNormalMapTexturePath), Error)};

        if (!mNormalMapTexturePath.empty() && NormalTexture == nullptr) {
            return false;
        }

        mBaseColorTexture = BaseTexture;
        mNormalMapTexture = NormalTexture;

        return true;
    }

    bool Engine::UMaterial::CreateResources(ID3D11Device* Device, ID3D11DeviceContext* Context) {
        if (Device == nullptr || Context == nullptr || (!mBaseColorTexturePath.empty() && mBaseColorTexture == nullptr) || (mBaseColorTexture != nullptr && !mBaseColorTexture->CreateResources(Device)) || (!mNormalMapTexturePath.empty() && mNormalMapTexture == nullptr) || (mNormalMapTexture != nullptr && !mNormalMapTexture->CreateResources(Device))) {
            return false;
        }

        const std::array<float, 8> Color{mBaseColor.x, mBaseColor.y, mBaseColor.z, mBaseColor.w, mSpecularStrength, mShininess, mNormalStrength, 0.0f};

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

    void Engine::UMaterial::ReleaseResources() {
        mConstantBuffer.Reset();
        mConstantsDirty = true;
    }

    ID3D11Buffer* Engine::UMaterial::GetConstantBuffer() const {
        return mConstantBuffer.Get();
    }

    const Core::FString& Engine::UMaterial::GetNormalMapTexturePath() const {
        return mNormalMapTexturePath;
    }

    bool Engine::UMaterial::SetNormalMapTexturePath(const Core::FString& Path) {
        if (!Path.empty() && !IsContentPath(Path)) {
            return false;
        }

        if (mNormalMapTexturePath != Path) {
            mNormalMapTexturePath = Path;
            mNormalMapTexture = nullptr;
        }

        return true;
    }

    Engine::UTexture2D* Engine::UMaterial::GetNormalMapTexture() const {
        return mNormalMapTexture;
    }

    float Engine::UMaterial::GetSpecularStrength() const {
        return mSpecularStrength;
    }

    bool Engine::UMaterial::SetSpecularStrength(float Strength) {
        if (!std::isfinite(Strength) || Strength < 0.0f || Strength > 1.0f) {
            return false;
        }

        mSpecularStrength = Strength;
        mConstantsDirty = true;

        return true;
    }

    float Engine::UMaterial::GetShininess() const {
        return mShininess;
    }

    bool Engine::UMaterial::SetShininess(float Shininess) {
        if (!std::isfinite(Shininess) || Shininess < 1.0f || Shininess > 1024.0f) {
            return false;
        }

        mShininess = Shininess;
        mConstantsDirty = true;

        return true;
    }

    float Engine::UMaterial::GetNormalStrength() const {
        return mNormalStrength;
    }

    bool Engine::UMaterial::SetNormalStrength(float Strength) {
        if (!std::isfinite(Strength) || Strength < 0.0f || Strength > 8.0f) {
            return false;
        }

        mNormalStrength = Strength;
        mConstantsDirty = true;

        return true;
    }

}
