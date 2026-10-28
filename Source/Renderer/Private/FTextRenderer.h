/** @file FTextRenderer.h @brief 폰트 아틀라스와 동적 정점 버퍼를 사용하는 텍스트 렌더러를 선언합니다. @ingroup Renderer */

#pragma once

#include "FFontAtlas.h"
#include "Engine/UTexture2D.h"
#include "Core/Math/FMatrix.h"
#include "RenderCore/FDynamicVertexBuffer.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <span>

struct FDrawText;
struct FSceneView;

/** @brief 문자 배치 캐시와 GPU 자원을 관리하여 화면 및 월드 공간 텍스트를 그립니다. */
class FTextRenderer {
public:
    /** @brief 리플렉션 소유 타입입니다. */
    ReflectTypes(FTextRenderer);

private:
    /** @brief 텍스트 셰이더에 전달하는 변환, 색상과 거리 필드 매개변수입니다. */
    struct FTextConstants {
    public:
        /** @brief 리플렉션 소유 타입입니다. */
        ReflectTypes(FTextConstants);

    public:
        /** @brief 상수 데이터의 메타데이터를 반환합니다. @return 등록된 구조체 메타데이터의 비소유 주소입니다. */
        ReflectStruct(FTextConstants);

    private:
        /** @brief 리플렉션 등록 함수에 접근을 허용합니다. */
        ReflectMembers();

    public:
        /** @brief 문자 평면에서 클립 공간으로 변환하는 행렬입니다. */
        FMatrix mTransform{};
        ReflectProperty(mTransform, "Transform", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
        /** @brief 텍스트에 곱할 RGBA 색상입니다. */
        FVector4 mColor{};
        ReflectProperty(mColor, "Color", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
        /** @brief 거리 필드 범위와 아틀라스의 픽셀 크기입니다. */
        FVector4 mParameters{};
        ReflectProperty(mParameters, "Parameters", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    };

public:
    /** @brief 텍스트 렌더러의 메타데이터를 반환합니다. @return 등록된 구조체 메타데이터의 비소유 주소입니다. */
    ReflectStruct(FTextRenderer);

    /** @brief 폰트 아틀라스, 텍스처, 샘플러와 상수 버퍼를 준비합니다. @param[in] Device 자원을 생성할 장치입니다. @param[in] FontPath 폰트 JSON 경로입니다. @return 초기화에 성공하면 true입니다. */
    bool Initialize(ID3D11Device* Device, const std::filesystem::path& FontPath);
    /** @brief 컴파일한 바이트코드로 셰이더와 입력 레이아웃을 교체합니다. @param[in] Device 자원을 생성할 장치입니다. @param[in] VertexCode 정점 셰이더 바이트코드입니다. @param[in] PixelCode 픽셀 셰이더 바이트코드입니다. @return 모든 자원을 생성하면 true입니다. */
    bool CreateShaders(ID3D11Device* Device, std::span<const Uint8> VertexCode, std::span<const Uint8> PixelCode);
    /** @brief 문자 정점을 공통 동적 버퍼에 업로드하고 텍스트를 그립니다. @param[in] Context 업로드와 그리기에 사용할 컨텍스트입니다. @param[in] Text 위치, 크기와 문자열을 지정하는 요청입니다. @param[in] View 변환과 화면 크기를 제공하는 뷰입니다. @return 유효한 문자 정점을 그렸으면 true입니다. */
    bool Draw(ID3D11DeviceContext* Context, const FDrawText& Text, const FSceneView& View);

    /** @brief 아틀라스와 정점 및 상수 버퍼의 사용량을 반환합니다. @return 소유한 GPU 자원의 바이트 수입니다. */
    std::size_t GetResourceBytes() const;
    /** @brief 문자 평면에서 문자열이 차지하는 경계 사각형을 계산합니다. @param[in] Text 경계를 계산할 문자열입니다. @param[out] Minimum 문자 평면의 최소 좌표입니다. @param[out] Maximum 문자 평면의 최대 좌표입니다. @return 문자가 존재하여 경계를 계산하면 true입니다. */
    bool GetBounds(const FString& Text, FVector2& Minimum, FVector2& Maximum) const;

private:
    /** @brief 리플렉션 등록 함수에 접근을 허용합니다. */
    ReflectMembers();

private:
    /** @brief 글리프 배치와 거리 필드 메타데이터입니다. */
    FFontAtlas mAtlas{};
    ReflectProperty(mAtlas, "Atlas", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 거리 필드 폰트 이미지와 GPU 텍스처입니다. */
    std::unique_ptr<UTexture2D> mTexture{};
    ReflectProperty(mTexture, "Texture", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 문자 평면을 클립 공간으로 변환하는 정점 셰이더입니다. */
    Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader{};
    ReflectProperty(mVertexShader, "VertexShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 거리 필드를 샘플링하는 픽셀 셰이더입니다. */
    Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader{};
    ReflectProperty(mPixelShader, "PixelShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 문자 정점의 위치와 UV 입력 레이아웃입니다. */
    Microsoft::WRL::ComPtr<ID3D11InputLayout> mLayout{};
    ReflectProperty(mLayout, "Layout", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 폰트 아틀라스를 샘플링하는 상태입니다. */
    Microsoft::WRL::ComPtr<ID3D11SamplerState> mSampler{};
    ReflectProperty(mSampler, "Sampler", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 현재 텍스트 요청의 셰이더 상수 버퍼입니다. */
    Microsoft::WRL::ComPtr<ID3D11Buffer> mConstants{};
    ReflectProperty(mConstants, "Constants", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 텍스트 정점의 동적 GPU 버퍼와 재사용할 용량입니다. */
    FDynamicVertexBuffer mVertices{};
    ReflectProperty(mVertices, "Vertices", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 문자열별로 재사용하는 문자 정점 배치입니다. */
    TMap<FString, TArray<FTextVertex>> mLayouts{};
    ReflectProperty(mLayouts, "Layouts", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
