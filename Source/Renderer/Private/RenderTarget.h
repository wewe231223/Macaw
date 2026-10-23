/**
 * @file RenderTarget.h
 * @brief Direct3D 11 오프스크린 렌더 타깃을 구현합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FRenderTarget.h"

#include <d3d11.h>
#include <wrl/client.h>

/**
 * @brief 컬러 텍스처, 깊이 버퍼와 대응 뷰를 소유하는 Direct3D 11 렌더 타깃입니다.
 * @ingroup Renderer
 */
class FDXRenderTarget final : public FRenderTarget {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FDXRenderTarget, FRenderTarget);

public:
    /**
     * @brief FDXRenderTarget의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    FDXRenderTarget();
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~FDXRenderTarget() override;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FDXRenderTarget);

    /**
     * @brief 새 크기에 맞추어 렌더 타깃 리소스를 생성하거나 교체합니다.
     * @param[in,out] Device 리소스 생성에 사용할 Direct3D 11 장치입니다.
     * @param[in] Width 클라이언트 영역 또는 타깃의 가로 픽셀 수입니다.
     * @param[in] Height 클라이언트 영역 또는 타깃의 세로 픽셀 수입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool Resize(ID3D11Device* Device, Uint32 Width, Uint32 Height);
    /**
     * @brief 가로 픽셀 수를 반환합니다.
     * @return 가로 픽셀 수입니다.
     */
    Uint32 GetWidth() const override;
    /**
     * @brief 세로 픽셀 수를 반환합니다.
     * @return 세로 픽셀 수입니다.
     */
    Uint32 GetHeight() const override;
    /**
     * @brief UI에서 샘플링할 셰이더 리소스 뷰의 비소유 주소를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    void* GetNativeTexture() const override;
    /**
     * @brief 컬러 출력용 Direct3D 11 렌더 타깃 뷰의 비소유 주소를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    ID3D11RenderTargetView* GetRenderTargetView() const;
    /**
     * @brief 깊이 스텐실 뷰를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    ID3D11DepthStencilView* GetDepthView() const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief UI 표시용 셰이더 리소스 뷰가 참조하는 컬러 텍스처입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mColor{};
    ReflectProperty(mColor, "Color", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 깊이 텍스처입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mDepth{};
    ReflectProperty(mDepth, "Depth", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 컬러 렌더 타깃 뷰입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mRenderTarget{};
    ReflectProperty(mRenderTarget, "RenderTarget", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief UI 샘플링에 사용할 셰이더 리소스 뷰입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mResourceView{};
    ReflectProperty(mResourceView, "ResourceView", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 깊이 스텐실 뷰입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mDepthView{};
    ReflectProperty(mDepthView, "DepthView", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 가로 픽셀 수입니다.
     */
    Uint32 mWidth{};
    ReflectProperty(mWidth, "Width", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 세로 픽셀 수입니다.
     */
    Uint32 mHeight{};
    ReflectProperty(mHeight, "Height", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
