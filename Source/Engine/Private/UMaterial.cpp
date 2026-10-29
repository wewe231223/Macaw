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

using namespace Microsoft::WRL;

/** @brief 머티리얼 경로 검증과 GPU 상수 표현의 내부 구현입니다. */
namespace {
    /**
     * @brief 절대 경로와 상위 폴더 이동을 제외한 Content 상대 경로인지 검사합니다.
     * @param[in] Path Content 상대 에셋 경로 또는 작업할 파일 경로입니다.
     * @return 조건을 만족하면 true입니다.
     */
    bool IsContentPath(const FString& Path) {
        if (Path.empty() || Path.find('\0') != FString::npos || Path.find_first_of("\r\n#\"") != FString::npos) {
            return false;
        }
        const std::filesystem::path File{FilePath::FromUtf8(Path)};
        if (File.is_absolute() || File.has_root_path()) {
            return false;
        }
        return std::none_of(File.begin(), File.end(), [](const auto& Part) {
            return Part == L"..";
        });
    }
}

UMaterial::UMaterial() = default;

UMaterial::~UMaterial() = default;

const FString& UMaterial::GetShaderPath() const {
    return mShaderPath;
}

bool UMaterial::SetShaderPath(const FString& Path) {
    if (!IsContentPath(Path) || !Path.ends_with(".hlsl")) {
        return false;
    }
    mShaderPath = Path;
    return true;
}

const FVector4& UMaterial::GetBaseColor() const {
    return mBaseColor;
}

bool UMaterial::SetBaseColor(const FVector4& Color) {
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

const FString& UMaterial::GetBaseColorTexturePath() const {
    return mBaseColorTexturePath;
}

bool UMaterial::SetBaseColorTexturePath(const FString& Path) {
    if (!Path.empty() && !IsContentPath(Path)) {
        return false;
    }
    if (mBaseColorTexturePath != Path) {
        mBaseColorTexturePath = Path;
        mBaseColorTexture = nullptr;
    }
    return true;
}

bool UMaterial::IsTwoSided() const {
    return mTwoSided;
}

bool UMaterial::SetTwoSided(bool TwoSided) {
    mTwoSided = TwoSided;
    return true;
}

UTexture2D* UMaterial::GetBaseColorTexture() const {
    return mBaseColorTexture;
}

bool UMaterial::ResolveTexture(FAssetManager& Assets, FString& Error) {
    Error.clear();
    UTexture2D* BaseTexture{mBaseColorTexturePath.empty() ? nullptr : Assets.LoadTexture(FilePath::FromUtf8(mBaseColorTexturePath), Error)};
    if (!mBaseColorTexturePath.empty() && BaseTexture == nullptr) {
        return false;
    }
    UTexture2D* NormalTexture{mNormalMapTexturePath.empty() ? nullptr : Assets.LoadNormalMap(FilePath::FromUtf8(mNormalMapTexturePath), Error)};
    if (!mNormalMapTexturePath.empty() && NormalTexture == nullptr) {
        return false;
    }
    mBaseColorTexture = BaseTexture;
    mNormalMapTexture = NormalTexture;
    return true;
}

bool UMaterial::CreateResources(ID3D11Device* Device, ID3D11DeviceContext* Context) {
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

void UMaterial::ReleaseResources() {
    mConstantBuffer.Reset();
    mConstantsDirty = true;
}

ID3D11Buffer* UMaterial::GetConstantBuffer() const {
    return mConstantBuffer.Get();
}

const FString& UMaterial::GetNormalMapTexturePath() const {
    return mNormalMapTexturePath;
}

bool UMaterial::SetNormalMapTexturePath(const FString& Path) {
    if (!Path.empty() && !IsContentPath(Path)) {
        return false;
    }
    if (mNormalMapTexturePath != Path) {
        mNormalMapTexturePath = Path;
        mNormalMapTexture = nullptr;
    }
    return true;
}

UTexture2D* UMaterial::GetNormalMapTexture() const {
    return mNormalMapTexture;
}

float UMaterial::GetSpecularStrength() const {
    return mSpecularStrength;
}

bool UMaterial::SetSpecularStrength(float Strength) {
    if (!std::isfinite(Strength) || Strength < 0.0f || Strength > 1.0f) {
        return false;
    }
    mSpecularStrength = Strength;
    mConstantsDirty = true;
    return true;
}

float UMaterial::GetShininess() const {
    return mShininess;
}

bool UMaterial::SetShininess(float Shininess) {
    if (!std::isfinite(Shininess) || Shininess < 1.0f || Shininess > 1024.0f) {
        return false;
    }
    mShininess = Shininess;
    mConstantsDirty = true;
    return true;
}

float UMaterial::GetNormalStrength() const {
    return mNormalStrength;
}

bool UMaterial::SetNormalStrength(float Strength) {
    if (!std::isfinite(Strength) || Strength < 0.0f || Strength > 8.0f) {
        return false;
    }
    mNormalStrength = Strength;
    mConstantsDirty = true;
    return true;
}
