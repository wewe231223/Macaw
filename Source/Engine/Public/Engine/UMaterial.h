/**
 * @file UMaterial.h
 * @brief 셰이더 경로·기본색·텍스처·양면 정책과 GPU 상수를 관리합니다.
 * @ingroup Engine
 */

#pragma once

#include "CoreUObject/UObject.h"
#include "Core/Math/FVector4.h"

#include <d3d11.h>
#include <wrl/client.h>

class FAssetManager;
class UTexture2D;

/**
 * @brief 셰이더 경로·기본색·텍스처·양면 정책과 GPU 상수를 관리합니다.
 */
class UMaterial final : public UObject {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(UMaterial, UObject);

public:
    /**
     * @brief 전달된 설정과 기본값으로 UMaterial 객체를 생성합니다.
     */
    UMaterial();

    /**
     * @brief 객체를 소멸하고 소유한 GPU 자원 또는 UV 제어 상태를 해제합니다.
     */
    ~UMaterial() override;

public:
    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectClass(UMaterial);

    /**
     * @brief Content 기준 머티리얼 셰이더 경로를 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FString& GetShaderPath() const;

    /**
     * @brief Content 기준 머티리얼 셰이더 경로를 설정합니다.
     * @param[in] Path Content 상대 에셋 경로 또는 작업할 파일 경로입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SetShaderPath(const FString& Path);

    /**
     * @brief 머티리얼 기본 RGBA 색상을 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FVector4& GetBaseColor() const;

    /**
     * @brief 머티리얼 기본 RGBA 색상을 설정합니다.
     * @param[in] Color 적용할 색상입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SetBaseColor(const FVector4& Color);

    /**
     * @brief Content 기준 기본색 텍스처 경로를 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FString& GetBaseColorTexturePath() const;

    /**
     * @brief Content 기준 기본색 텍스처 경로를 설정합니다.
     * @param[in] Path Content 상대 에셋 경로 또는 작업할 파일 경로입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SetBaseColorTexturePath(const FString& Path);

    /**
     * @brief Content 상대 노멀 맵 텍스처 경로를 반환합니다.
     * @return 머티리얼이 소유한 경로 문자열의 참조입니다.
     */
    const FString& GetNormalMapTexturePath() const;

    /**
     * @brief Content 상대 노멀 맵 경로를 변경하고 해석된 참조를 초기화합니다.
     * @param[in] Path Content 상대 에셋 경로 또는 읽을 파일 경로입니다.
     * @return 경로 변경을 수락하면 true입니다.
     */
    bool SetNormalMapTexturePath(const FString& Path);

    /**
     * @brief 해석된 노멀 맵 텍스처를 조회합니다.
     * @return 에셋 캐시가 소유한 텍스처의 비소유 주소이며 미해석 상태면 nullptr입니다.
     */
    UTexture2D* GetNormalMapTexture() const;

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
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SetTwoSided(bool TwoSided);

    /**
     * @brief 기본색 텍스처의 비소유 주소를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    UTexture2D* GetBaseColorTexture() const;

    /**
     * @brief 저장된 텍스처 경로를 에셋 관리자에서 해석하고 렌더 상태를 갱신합니다.
     * @param[in] Assets 텍스처를 해석할 프로젝트 에셋 관리자입니다.
     * @param[out] Error 실패 시 원인을 받을 문자열입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool ResolveTexture(FAssetManager& Assets, FString& Error);

    /**
     * @brief 필요한 GPU 자원을 생성하거나 변경된 상수를 업로드합니다.
     * @param[in] Device 자원 생성에 사용할 Direct3D 장치의 비소유 주소입니다.
     * @param[in] Context 그리기 또는 편집에 사용할 컨텍스트의 비소유 주소입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool CreateResources(ID3D11Device* Device, ID3D11DeviceContext* Context);

    /**
     * @brief 소유한 GPU 자원을 해제합니다.
     */
    void ReleaseResources();

    /**
     * @brief GPU 상수 버퍼를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ID3D11Buffer* GetConstantBuffer() const;

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief Content 기준 머티리얼 셰이더 경로입니다.
     */
    FString mShaderPath{"Shaders/Lit.hlsl"};
    ReflectAccessor(mShaderPath, GetShaderPath, SetShaderPath, "Shader", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    /**
     * @brief 표시 색상입니다.
     */
    FVector4 mBaseColor{1.0f, 1.0f, 1.0f, 1.0f};
    ReflectAccessor(mBaseColor, GetBaseColor, SetBaseColor, "BaseColor", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    /**
     * @brief Content 기준 기본색 텍스처 경로입니다.
     */
    FString mBaseColorTexturePath{};
    ReflectAccessor(mBaseColorTexturePath, GetBaseColorTexturePath, SetBaseColorTexturePath, "BaseColorTexture", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    /**
     * @brief Content 상대 노멀 맵 텍스처 경로입니다.
     */
    FString mNormalMapTexturePath{};
    ReflectAccessor(mNormalMapTexturePath, GetNormalMapTexturePath, SetNormalMapTexturePath, "NormalMap", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    /**
     * @brief 로드한 노멀 맵 텍스처의 비소유 주소입니다.
     */
    UTexture2D* mNormalMapTexture{};
    ReflectProperty(mNormalMapTexture, "ResolvedNormalMap", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 정반사 성분의 밝기 배율입니다.
     */
    float mSpecularStrength{0.35f};
    ReflectAccessor(mSpecularStrength, GetSpecularStrength, SetSpecularStrength, "SpecularStrength", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    /**
     * @brief 정반사 하이라이트의 지수입니다.
     */
    float mShininess{32.0f};
    ReflectAccessor(mShininess, GetShininess, SetShininess, "Shininess", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    /**
     * @brief 노멀 맵의 접선 공간 XY 성분 배율입니다.
     */
    float mNormalStrength{1.0f};
    ReflectAccessor(mNormalStrength, GetNormalStrength, SetNormalStrength, "NormalStrength", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    /**
     * @brief 메시의 양면 렌더링 여부입니다.
     */
    bool mTwoSided{};
    ReflectAccessor(mTwoSided, IsTwoSided, SetTwoSided, "TwoSided", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    /**
     * @brief 기본색 텍스처의 비소유 주소입니다.
     */
    UTexture2D* mBaseColorTexture{};
    ReflectProperty(mBaseColorTexture, "ResolvedBaseColorTexture", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief GPU 상수 버퍼입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11Buffer> mConstantBuffer{};
    ReflectProperty(mConstantBuffer, "ConstantBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief GPU 상수 버퍼 갱신 필요 여부입니다.
     */
    bool mConstantsDirty{true};
    ReflectProperty(mConstantsDirty, "ConstantsDirty", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
