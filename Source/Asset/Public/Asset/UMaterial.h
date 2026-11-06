/**
 * @file UMaterial.h
 * @brief 셰이더 경로·기본색·텍스처·양면 정책과 GPU 상수를 관리합니다.
 * @ingroup Asset
 */

#pragma once

#include "CoreUObject/UObject.h"
#include "Core/Math/FVector4.h"
#include "RenderCore/TConstantBuffer.h"

#include <d3d11.h>
#include <array>
#include <wrl/client.h>
#include "UMaterial.generated.h"

namespace Asset {

    class FAssetManager;
    class UTexture2D;

    /**
     * @brief 셰이더 경로·기본색·텍스처·양면 정책과 GPU 상수를 관리합니다.
     */
    UCLASS()
    class UMaterial final : public CoreUObject::UObject {
        GENERATED_BODY()

    public:
        /**
         * @brief 전달된 설정과 기본값으로 UMaterial 객체를 생성합니다.
         */
        UMaterial();

        /**
         * @brief 기본 소멸자이며, GPU 상수 버퍼는 ComPtr 멤버가 해제합니다.
         */
        ~UMaterial() override;

    public:
        /**
         * @brief Content 기준 머티리얼 셰이더 경로를 반환합니다.
         * @return 이 객체가 보관하는 값의 참조입니다.
         */
        const Core::FString& GetShaderPath() const;

        /**
         * @brief Content 기준 머티리얼 셰이더 경로를 설정합니다.
         * @param[in] Path Content 기준 셰이더 경로입니다.
         * @return 값이 유효해 적용했으면 true, 거부했으면 false입니다.
         */
        bool SetShaderPath(const Core::FString& Path);

        /**
         * @brief 머티리얼 기본 RGBA 색상을 반환합니다.
         * @return 이 객체가 보관하는 값의 참조입니다.
         */
        const Core::FVector4& GetBaseColor() const;

        /**
         * @brief 머티리얼 기본 RGBA 색상을 설정합니다.
         * @param[in] Color 적용할 색상입니다.
         * @return 값이 유효해 적용했으면 true, 거부했으면 false입니다.
         */
        bool SetBaseColor(const Core::FVector4& Color);

        /**
         * @brief Content 기준 기본색 텍스처 경로를 반환합니다.
         * @return 이 객체가 보관하는 값의 참조입니다.
         */
        const Core::FString& GetBaseColorTexturePath() const;

        /**
         * @brief Content 기준 기본색 텍스처 경로를 설정합니다.
         * @param[in] Path Content 기준 텍스처 경로이며 비우면 텍스처를 사용하지 않습니다.
         * @return 값이 유효해 적용했으면 true, 거부했으면 false입니다.
         */
        bool SetBaseColorTexturePath(const Core::FString& Path);

        /**
         * @brief Content 상대 노멀 맵 텍스처 경로를 반환합니다.
         * @return 머티리얼이 소유한 경로 문자열의 참조입니다.
         */
        const Core::FString& GetNormalMapTexturePath() const;

        /**
         * @brief Content 상대 노멀 맵 경로를 변경하고 해석된 참조를 초기화합니다.
         * @param[in] Path Content 기준 노멀 맵 텍스처 경로이며, 비우면 노멀 맵을 사용하지 않습니다.
         * @return 경로 변경을 수락하면 true입니다.
         */
        bool SetNormalMapTexturePath(const Core::FString& Path);

        /**
         * @brief 해석된 노멀 맵 텍스처를 조회합니다.
         * @return 에셋 캐시가 소유한 텍스처의 비소유 주소이며 미해석 상태면 nullptr입니다.
         */
        Asset::UTexture2D* GetNormalMapTexture() const;

        /**
         * @brief 정반사 성분의 밝기 배율을 반환합니다.
         * @return 정반사 성분의 밝기 배율입니다.
         */
        float GetSpecularStrength() const;

        /**
         * @brief 정반사 성분의 밝기 배율을 검증하고 변경합니다.
         * @param[in] Strength 0 이상 1 이하인 유한한 정반사 강도입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetSpecularStrength(float Strength);

        /**
         * @brief 정반사 하이라이트의 지수를 반환합니다.
         * @return 정반사 하이라이트의 지수입니다.
         */
        float GetShininess() const;

        /**
         * @brief 정반사 하이라이트의 지수를 검증하고 변경합니다.
         * @param[in] Shininess 1 이상 1024 이하인 유한한 정반사 지수입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetShininess(float Shininess);

        /**
         * @brief 노멀 맵의 접선 공간 XY 성분 배율을 반환합니다.
         * @return 노멀 맵의 접선 공간 XY 성분 배율입니다.
         */
        float GetNormalStrength() const;

        /**
         * @brief 노멀 맵의 접선 공간 XY 성분 배율을 검증하고 변경합니다.
         * @param[in] Strength 0 이상 8 이하인 유한한 노멀 맵 강도입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetNormalStrength(float Strength);

        /**
         * @brief 머티리얼이 양면 렌더링을 사용하는지 반환합니다.
         * @return 양면 렌더링을 사용하면 true입니다.
         */
        bool IsTwoSided() const;

        /**
         * @brief 메시 양면 렌더링 여부를 설정합니다.
         * @param[in] TwoSided 양면 렌더링을 사용할지 지정합니다.
         * @return 값이 유효해 적용했으면 true, 거부했으면 false입니다.
         */
        bool SetTwoSided(bool TwoSided);

        /**
         * @brief 기본색 텍스처의 비소유 주소를 반환합니다.
         * @return 대상의 비소유 주소이며, 없거나 유효하지 않으면 nullptr입니다.
         */
        Asset::UTexture2D* GetBaseColorTexture() const;

        /**
         * @brief 저장된 텍스처 경로를 에셋 관리자에서 해석하고 렌더 상태를 갱신합니다.
         * @param[in] Assets 텍스처를 해석할 프로젝트 에셋 관리자입니다.
         * @param[out] Error 실패 시 원인을 받을 문자열입니다.
         * @return 경로가 비어 있거나 에셋을 찾았으면 true, 로드에 실패하면 false입니다.
         */
        bool ResolveTexture(Asset::FAssetManager& Assets, Core::FString& Error);

        /**
         * @brief 필요한 GPU 자원을 생성하거나 변경된 상수를 업로드합니다.
         * @param[in] Device 자원 생성에 사용할 Direct3D 장치의 비소유 주소입니다.
         * @param[in] Context 상수 버퍼를 갱신할 Direct3D 11 장치 컨텍스트입니다.
         * @return 성공하면 true, 실패하면 false입니다.
         */
        bool CreateResources(ID3D11Device* Device, ID3D11DeviceContext* Context);

        /**
         * @brief 소유한 GPU 자원을 해제합니다.
         */
        void ReleaseResources();

        /**
         * @brief GPU 상수 버퍼를 지정한 셰이더 단계에 연결합니다.
         * @param[in] Context 바인딩할 장치 컨텍스트입니다.
         * @param[in] Stages 연결할 셰이더 단계입니다.
         * @param[in] Slot 상수 버퍼 슬롯(b 레지스터)입니다.
         */
        void BindConstants(ID3D11DeviceContext* Context, RenderCore::EShaderStage Stages, Core::Uint32 Slot) const;

        const Core::FVector4& GetSpecularColor() const;
        bool SetSpecularColor(const Core::FVector4& Color);

        const Core::FVector4& GetAmbientColor() const;
        bool SetAmbientColor(const Core::FVector4& Color);

        const Core::FVector4& GetEmissiveColor() const;
        bool SetEmissiveColor(const Core::FVector4& Color);

        const Core::FVector4& GetReflectionColor() const;
        bool SetReflectionColor(const Core::FVector4& Color);

        float GetReflectionStrength() const;
        bool SetReflectionStrength(float Strength);

        const Core::FString& GetSpecularTexturePath() const;
        bool SetSpecularTexturePath(const Core::FString& Path);
        Asset::UTexture2D* GetSpecularTexture() const;

        const Core::FString& GetEmissiveTexturePath() const;
        bool SetEmissiveTexturePath(const Core::FString& Path);
        Asset::UTexture2D* GetEmissiveTexture() const;

        const Core::FString& GetReflectionTexturePath() const;
        bool SetReflectionTexturePath(const Core::FString& Path);
        Asset::UTexture2D* GetReflectionTexture() const;

    private:
        /**
         * @brief Content 기준 머티리얼 셰이더 경로입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "Shader", Getter = GetShaderPath, Setter = SetShaderPath)
        Core::FString mShaderPath{"Shaders/Lit.hlsl"};

        /**
         * @brief 머티리얼의 기본 RGBA 색상입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetBaseColor, Setter = SetBaseColor)
        Core::FVector4 mBaseColor{1.0f, 1.0f, 1.0f, 1.0f};

        /**
         * @brief Content 기준 기본색 텍스처 경로입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "BaseColorTexture", Getter = GetBaseColorTexturePath, Setter = SetBaseColorTexturePath)
        Core::FString mBaseColorTexturePath{};

        /**
         * @brief Content 상대 노멀 맵 텍스처 경로입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "NormalMap", Getter = GetNormalMapTexturePath, Setter = SetNormalMapTexturePath)
        Core::FString mNormalMapTexturePath{};

        /**
         * @brief 로드한 노멀 맵 텍스처의 비소유 주소입니다.
         */
        Asset::UTexture2D* mNormalMapTexture{};

        /**
         * @brief 정반사 성분의 밝기 배율입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetSpecularStrength, Setter = SetSpecularStrength)
        float mSpecularStrength{0.35f};

        /**
         * @brief 정반사 하이라이트의 지수입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetShininess, Setter = SetShininess)
        float mShininess{32.0f};

        /**
         * @brief 노멀 맵의 접선 공간 XY 성분 배율입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetNormalStrength, Setter = SetNormalStrength)
        float mNormalStrength{1.0f};

        /**
         * @brief 메시의 양면 렌더링 여부입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = IsTwoSided, Setter = SetTwoSided)
        bool mTwoSided{};

        /**
         * @brief 기본색 텍스처의 비소유 주소입니다.
         */
        Asset::UTexture2D* mBaseColorTexture{};

        UPROPERTY(Edit, Serialize, Getter = GetSpecularColor, Setter = SetSpecularColor)
        Core::FVector4 mSpecularColor{1.0f, 1.0f, 1.0f, 1.0f};

        UPROPERTY(Edit, Serialize, Getter = GetAmbientColor, Setter = SetAmbientColor)
        Core::FVector4 mAmbientColor{1.0f, 1.0f, 1.0f, 1.0f};

        UPROPERTY(Edit, Serialize, Getter = GetEmissiveColor, Setter = SetEmissiveColor)
        Core::FVector4 mEmissiveColor{0.0f, 0.0f, 0.0f, 1.0f};

        UPROPERTY(Edit, Serialize, Getter = GetReflectionColor, Setter = SetReflectionColor)
        Core::FVector4 mReflectionColor{1.0f, 1.0f, 1.0f, 1.0f};

        UPROPERTY(Edit, Serialize, Getter = GetReflectionStrength, Setter = SetReflectionStrength)
        float mReflectionStrength{};

        UPROPERTY(Edit, Serialize, Name = "SpecularTexture", Getter = GetSpecularTexturePath, Setter = SetSpecularTexturePath)
        Core::FString mSpecularTexturePath{};

        Asset::UTexture2D* mSpecularTexture{};

        UPROPERTY(Edit, Serialize, Name = "EmissiveTexture", Getter = GetEmissiveTexturePath, Setter = SetEmissiveTexturePath)
        Core::FString mEmissiveTexturePath{};

        Asset::UTexture2D* mEmissiveTexture{};

        UPROPERTY(Edit, Serialize, Name = "ReflectionTexture", Getter = GetReflectionTexturePath, Setter = SetReflectionTexturePath)
        Core::FString mReflectionTexturePath{};

        Asset::UTexture2D* mReflectionTexture{};

        /**
         * @brief GPU 상수 버퍼입니다.
         */
        RenderCore::TConstantBuffer<std::array<float, 28>> mConstantBuffer{};

        /**
         * @brief GPU 상수 버퍼 갱신 필요 여부입니다.
         */
        bool mConstantsDirty{true};
    };

}
