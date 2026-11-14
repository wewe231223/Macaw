/**
 * @file UMaterial.cpp
 * @brief 셰이더 경로·기본색·텍스처·양면 정책과 GPU 상수를 관리합니다.
 * @ingroup Asset
 */

#include "Core/Types.h"
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

    /**
     * @brief 전달된 설정과 기본값으로 UMaterial 객체를 생성합니다.
     */
    Asset::UMaterial::UMaterial() = default;

    /**
     * @brief 기본 소멸자이며, GPU 상수 버퍼는 ComPtr 멤버가 해제합니다.
     */
    Asset::UMaterial::~UMaterial() = default;

    /**
     * @brief Content 기준 머티리얼 셰이더 경로를 반환합니다.
     */
    const Core::FString& Asset::UMaterial::GetShaderPath() const {
        return mShaderPath;
    }

    /**
     * @brief Content 기준 머티리얼 셰이더 경로를 설정합니다.
     */
    bool Asset::UMaterial::SetShaderPath(const Core::FString& Path) {
        if (!IsContentPath(Path) || !Path.ends_with(".hlsl")) {
            return false;
        }

        mShaderPath = Path;

        return true;
    }

    /**
     * @brief 머티리얼 기본 RGBA 색상을 반환합니다.
     */
    const Core::FVector4& Asset::UMaterial::GetBaseColor() const {
        return mBaseColor;
    }

    /**
     * @brief 머티리얼 기본 RGBA 색상을 설정합니다.
     */
    bool Asset::UMaterial::SetBaseColor(const Core::FVector4& Color) {
        const auto Values{Color.ToArray()};

        if (!std::all_of(Values.begin(), Values.end(), [](float Value) {
            return std::isfinite(Value) && Value >= 0.0f && Value <= 1.0f;
        })) {
            return false;
        }

        const Asset::EBlendMode PreviousBlendMode{GetBlendMode()};

        if (mBaseColor != Color) {
            if (mBaseColor.w != Color.w) {
                mBlendMode = Color.w == 1.0f ? Asset::EBlendMode::Opaque : Asset::EBlendMode::Translucent;
            }

            mBaseColor = Color;
            mConstantsDirty = true;
        }

        if (PreviousBlendMode != GetBlendMode()) {
            mBlendModeRevision = Core::FRevision::Next();
        }

        return true;
    }

    Asset::EBlendMode Asset::UMaterial::GetBlendMode() const {
        return mBaseColor.w == 1.0f ? mBlendMode : Asset::EBlendMode::Translucent;
    }

    bool Asset::UMaterial::SetBlendMode(Asset::EBlendMode BlendMode) {
        if (BlendMode != Asset::EBlendMode::Opaque && BlendMode != Asset::EBlendMode::Translucent) {
            return false;
        }

        const Asset::EBlendMode PreviousBlendMode{GetBlendMode()};
        mBlendMode = mBaseColor.w == 1.0f ? BlendMode : Asset::EBlendMode::Translucent;

        if (PreviousBlendMode != GetBlendMode()) {
            mBlendModeRevision = Core::FRevision::Next();
        }

        return true;
    }

    Core::FRevision Asset::UMaterial::GetBlendModeRevision() const {
        return mBlendModeRevision;
    }

    /**
     * @brief Content 기준 기본색 텍스처 경로를 반환합니다.
     */
    const Core::FString& Asset::UMaterial::GetBaseColorTexturePath() const {
        return mBaseColorTexturePath;
    }

    /**
     * @brief Content 기준 기본색 텍스처 경로를 설정합니다.
     */
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

    /**
     * @brief 머티리얼이 양면 렌더링을 사용하는지 반환합니다.
     */
    bool Asset::UMaterial::IsTwoSided() const {
        return mTwoSided;
    }

    /**
     * @brief 메시 양면 렌더링 여부를 설정합니다.
     */
    bool Asset::UMaterial::SetTwoSided(bool TwoSided) {
        mTwoSided = TwoSided;

        return true;
    }

    /**
     * @brief 기본색 텍스처의 비소유 주소를 반환합니다.
     */
    Asset::UTexture2D* Asset::UMaterial::GetBaseColorTexture() const {
        return mBaseColorTexture;
    }

    /**
     * @brief 저장된 텍스처 경로를 에셋 관리자에서 해석하고 렌더 상태를 갱신합니다.
     */
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

    /**
     * @brief 필요한 GPU 자원을 생성하거나 변경된 상수를 업로드합니다.
     */
    bool Asset::UMaterial::CreateResources(ID3D11Device* Device, ID3D11DeviceContext* Context) {
        if (Device == nullptr || Context == nullptr || (!mBaseColorTexturePath.empty() && mBaseColorTexture == nullptr) || (mBaseColorTexture != nullptr && !mBaseColorTexture->CreateResources(Device)) || (!mNormalMapTexturePath.empty() && mNormalMapTexture == nullptr) || (mNormalMapTexture != nullptr && !mNormalMapTexture->CreateResources(Device))) {
            return false;
        }

        const std::array<Asset::UTexture2D*, 3> Textures{mSpecularTexture, mEmissiveTexture, mReflectionTexture};
        const std::array<const Core::FString*, 3> Paths{&mSpecularTexturePath, &mEmissiveTexturePath, &mReflectionTexturePath};

        for (Core::Index_t Index{}; Index < Textures.size(); ++Index) {
            if ((!Paths[Index]->empty() && Textures[Index] == nullptr) || (Textures[Index] != nullptr && !Textures[Index]->CreateResources(Device))) {
                return false;
            }
        }

        const std::array<float, 28> Color{mBaseColor.x, mBaseColor.y, mBaseColor.z, mBaseColor.w, mSpecularStrength, mShininess, mNormalStrength, mReflectionStrength, mSpecularColor.x, mSpecularColor.y, mSpecularColor.z, mSpecularColor.w, mAmbientColor.x, mAmbientColor.y, mAmbientColor.z, mAmbientColor.w, mEmissiveColor.x, mEmissiveColor.y, mEmissiveColor.z, mEmissiveColor.w, mReflectionColor.x, mReflectionColor.y, mReflectionColor.z, mReflectionColor.w, mSpecularTexture != nullptr ? 1.0f : 0.0f, mEmissiveTexture != nullptr ? 1.0f : 0.0f, mReflectionTexture != nullptr ? 1.0f : 0.0f, 0.0f};

        const bool Created{mConstantBuffer.IsCreated()};
        Core::FString Error{};

        if (!mConstantBuffer.Create(Device, Error)) {
            return false;
        }

        if (!Created || mConstantsDirty) {
            mConstantBuffer.Update(Context, Color);
        }

        mConstantsDirty = false;

        return true;
    }

    /**
     * @brief 소유한 GPU 자원을 해제합니다.
     */
    void Asset::UMaterial::ReleaseResources() {
        mConstantBuffer.Reset();
        mConstantsDirty = true;
    }

    /**
     * @brief GPU 상수 버퍼를 지정한 셰이더 단계에 연결합니다.
     */
    void Asset::UMaterial::BindConstants(ID3D11DeviceContext* Context, RenderCore::EShaderStage Stages, Core::Index_t Slot) const {
        mConstantBuffer.Bind(Context, Stages, Slot);
    }

    /**
     * @brief Content 상대 노멀 맵 텍스처 경로를 반환합니다.
     */
    const Core::FString& Asset::UMaterial::GetNormalMapTexturePath() const {
        return mNormalMapTexturePath;
    }

    /**
     * @brief Content 상대 노멀 맵 경로를 변경하고 해석된 참조를 초기화합니다.
     */
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

    /**
     * @brief 해석된 노멀 맵 텍스처를 조회합니다.
     */
    Asset::UTexture2D* Asset::UMaterial::GetNormalMapTexture() const {
        return mNormalMapTexture;
    }

    /**
     * @brief 정반사 성분의 밝기 배율을 반환합니다.
     */
    float Asset::UMaterial::GetSpecularStrength() const {
        return mSpecularStrength;
    }

    /**
     * @brief 정반사 성분의 밝기 배율을 검증하고 변경합니다.
     */
    bool Asset::UMaterial::SetSpecularStrength(float Strength) {
        if (!std::isfinite(Strength) || Strength < 0.0f || Strength > 1.0f) {
            return false;
        }

        mSpecularStrength = Strength;
        mConstantsDirty = true;

        return true;
    }

    /**
     * @brief 정반사 하이라이트의 지수를 반환합니다.
     */
    float Asset::UMaterial::GetShininess() const {
        return mShininess;
    }

    /**
     * @brief 정반사 하이라이트의 지수를 검증하고 변경합니다.
     */
    bool Asset::UMaterial::SetShininess(float Shininess) {
        if (!std::isfinite(Shininess) || Shininess < 1.0f || Shininess > 1024.0f) {
            return false;
        }

        mShininess = Shininess;
        mConstantsDirty = true;

        return true;
    }

    /**
     * @brief 노멀 맵의 접선 공간 XY 성분 배율을 반환합니다.
     */
    float Asset::UMaterial::GetNormalStrength() const {
        return mNormalStrength;
    }

    /**
     * @brief 노멀 맵의 접선 공간 XY 성분 배율을 검증하고 변경합니다.
     */
    bool Asset::UMaterial::SetNormalStrength(float Strength) {
        if (!std::isfinite(Strength) || Strength < 0.0f || Strength > 8.0f) {
            return false;
        }

        mNormalStrength = Strength;
        mConstantsDirty = true;

        return true;
    }

    /**
     * @brief 정반사 색상 값을 반환합니다.
     */
    const Core::FVector4& Asset::UMaterial::GetSpecularColor() const {
        return mSpecularColor;
    }

    /**
     * @brief 정반사 색상 값을 검증해 적용하고 렌더 상태를 갱신합니다.
     */
    bool Asset::UMaterial::SetSpecularColor(const Core::FVector4& Color) {
        if (!Color.IsFinite() || Color.x < 0.0f || Color.y < 0.0f || Color.z < 0.0f || Color.w < 0.0f || Color.w > 1.0f) {
            return false;
        }

        mSpecularColor = Color;
        mConstantsDirty = true;

        return true;
    }

    /**
     * @brief 주변광 색상 값을 반환합니다.
     */
    const Core::FVector4& Asset::UMaterial::GetAmbientColor() const {
        return mAmbientColor;
    }

    /**
     * @brief 주변광 색상 값을 검증해 적용하고 렌더 상태를 갱신합니다.
     */
    bool Asset::UMaterial::SetAmbientColor(const Core::FVector4& Color) {
        if (!Color.IsFinite() || Color.x < 0.0f || Color.y < 0.0f || Color.z < 0.0f || Color.w < 0.0f || Color.w > 1.0f) {
            return false;
        }

        mAmbientColor = Color;
        mConstantsDirty = true;

        return true;
    }

    /**
     * @brief 자체 발광 색상 값을 반환합니다.
     */
    const Core::FVector4& Asset::UMaterial::GetEmissiveColor() const {
        return mEmissiveColor;
    }

    /**
     * @brief 자체 발광 색상 값을 검증해 적용하고 렌더 상태를 갱신합니다.
     */
    bool Asset::UMaterial::SetEmissiveColor(const Core::FVector4& Color) {
        if (!Color.IsFinite() || Color.x < 0.0f || Color.y < 0.0f || Color.z < 0.0f || Color.w < 0.0f || Color.w > 1.0f) {
            return false;
        }

        mEmissiveColor = Color;
        mConstantsDirty = true;

        return true;
    }

    /**
     * @brief 환경 반사 색상 값을 반환합니다.
     */
    const Core::FVector4& Asset::UMaterial::GetReflectionColor() const {
        return mReflectionColor;
    }

    /**
     * @brief 환경 반사 색상 값을 검증해 적용하고 렌더 상태를 갱신합니다.
     */
    bool Asset::UMaterial::SetReflectionColor(const Core::FVector4& Color) {
        if (!Color.IsFinite() || Color.x < 0.0f || Color.y < 0.0f || Color.z < 0.0f || Color.w < 0.0f || Color.w > 1.0f) {
            return false;
        }

        mReflectionColor = Color;
        mConstantsDirty = true;

        return true;
    }

    /**
     * @brief 환경 반사 세기 값을 반환합니다.
     */
    float Asset::UMaterial::GetReflectionStrength() const {
        return mReflectionStrength;
    }

    /**
     * @brief 환경 반사 세기 값을 검증해 적용하고 렌더 상태를 갱신합니다.
     */
    bool Asset::UMaterial::SetReflectionStrength(float Strength) {
        if (!std::isfinite(Strength) || Strength < 0.0f || Strength > 1.0f) {
            return false;
        }

        mReflectionStrength = Strength;
        mConstantsDirty = true;

        return true;
    }

    /**
     * @brief 정반사 텍스처의 프로젝트 상대 경로 값을 반환합니다.
     */
    const Core::FString& Asset::UMaterial::GetSpecularTexturePath() const {
        return mSpecularTexturePath;
    }

    /**
     * @brief 정반사 텍스처의 프로젝트 상대 경로 값을 검증해 적용하고 렌더 상태를 갱신합니다.
     */
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

    /**
     * @brief 정반사 텍스처 값을 반환합니다.
     */
    Asset::UTexture2D* Asset::UMaterial::GetSpecularTexture() const {
        return mSpecularTexture;
    }

    /**
     * @brief 자체 발광 텍스처의 프로젝트 상대 경로 값을 반환합니다.
     */
    const Core::FString& Asset::UMaterial::GetEmissiveTexturePath() const {
        return mEmissiveTexturePath;
    }

    /**
     * @brief 자체 발광 텍스처의 프로젝트 상대 경로 값을 검증해 적용하고 렌더 상태를 갱신합니다.
     */
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

    /**
     * @brief 자체 발광 텍스처 값을 반환합니다.
     */
    Asset::UTexture2D* Asset::UMaterial::GetEmissiveTexture() const {
        return mEmissiveTexture;
    }

    /**
     * @brief 환경 반사 텍스처의 프로젝트 상대 경로 값을 반환합니다.
     */
    const Core::FString& Asset::UMaterial::GetReflectionTexturePath() const {
        return mReflectionTexturePath;
    }

    /**
     * @brief 환경 반사 텍스처의 프로젝트 상대 경로 값을 검증해 적용하고 렌더 상태를 갱신합니다.
     */
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

    /**
     * @brief 환경 반사 텍스처 값을 반환합니다.
     */
    Asset::UTexture2D* Asset::UMaterial::GetReflectionTexture() const {
        return mReflectionTexture;
    }

}
