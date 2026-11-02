/**
 * @file UTexture2D.h
 * @brief 2차원 텍스처의 CPU 픽셀과 Direct3D 리소스 관리에 대한 선언입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/UTexture.h"
#include "Asset/ETextureUsage.h"

#include <d3d11.h>
#include <filesystem>
#include <span>
#include <wrl/client.h>

/**
 * @brief 텍스처 변환에 사용하는 DirectXTex 이미지 타입을 참조합니다.
 * @namespace DirectX
 */
namespace DirectX {
    class ScratchImage;
}

namespace Asset {

    /**
     * @brief 2차원 텍스처의 CPU 픽셀과 Direct3D 리소스 관리를 담당합니다.
     */
    class UTexture2D final : public Asset::UTexture {
    public:
        /**
         * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
         */
        ReflectTypes(UTexture2D, Asset::UTexture);

    public:
        /**
         * @brief UTexture2D의 초기 상태를 구성합니다.
         */
        UTexture2D();

        /**
         * @brief 전달받은 설정과 의존성으로 UTexture2D의 초기 상태를 구성합니다.
         * @param[in] ImagePath 읽을 원본 이미지 파일의 경로입니다.
         * @param[in] SRGB 이미지를 sRGB 색 공간으로 해석할지 나타냅니다.
         */
        explicit UTexture2D(const std::filesystem::path& ImagePath, bool SRGB = true);

        /**
         * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
         */
        ~UTexture2D() override;

    public:
        /**
         * @brief 이 타입의 클래스 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectClass(UTexture2D);

        /**
         * @brief 이미지 파일을 디코딩하여 텍스처 크기·형식·픽셀 데이터를 설정합니다.
         * @param[in] ImagePath 읽을 원본 이미지 파일의 경로입니다.
         * @param[in] SRGB 이미지를 sRGB 색 공간으로 해석할지 나타냅니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool LoadFromFile(const std::filesystem::path& ImagePath, bool SRGB = true);

        /**
         * @brief 메모리의 이미지 바이트를 디코딩하여 텍스처 크기·형식·픽셀 데이터를 설정합니다.
         * @param[in] Data 원본 이미지 파일 전체 바이트입니다.
         * @param[in] ImagePath 디코더를 고를 확장자를 가진 원본 이미지 이름 또는 경로입니다.
         * @param[in] SRGB 이미지를 sRGB 색 공간으로 해석할지 나타냅니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool LoadFromMemory(std::span<const Core::Uint8> Data, const std::filesystem::path& ImagePath, bool SRGB = true);

        /**
         * @brief DDS 데이터를 읽어 이미지 형식과 크기 및 CPU 이미지 데이터를 설정합니다.
         * @param[in] Data DDS 파일 전체 바이트입니다.
         * @param[in] SRGB 이미지를 sRGB 색 공간으로 해석할지 나타냅니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool SetDDSData(std::span<const Core::Uint8> Data, bool SRGB);

        /**
         * @brief 보존 중인 DDS 형식의 이미지 바이트 배열을 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Core::TArray<Core::Uint8>& GetDDSData() const;

        /**
         * @brief 크기·형식·용도·색 공간을 검증하여 텍스처 생성 구성을 설정합니다.
         * @param[in] SizeX 텍스처의 가로 픽셀 수입니다.
         * @param[in] SizeY 텍스처의 세로 픽셀 수입니다.
         * @param[in] Format 텍스처 형식입니다.
         * @param[in] Usage 셰이더 읽기·색상 출력·깊이 출력의 용도 플래그입니다.
         * @param[in] SRGB 이미지를 sRGB 색 공간으로 해석할지 나타냅니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool Configure(Core::Uint32 SizeX, Core::Uint32 SizeY, Asset::ETextureFormat Format, Asset::ETextureUsage Usage, bool SRGB = false);

        /**
         * @brief 크기와 형식에 맞는 CPU 픽셀 데이터를 검증하여 교체합니다.
         * @param[in] SizeX 텍스처의 가로 픽셀 수입니다.
         * @param[in] SizeY 텍스처의 세로 픽셀 수입니다.
         * @param[in] Format 픽셀 데이터의 형식입니다.
         * @param[in] Pixels 크기와 픽셀 형식에 맞는 새 CPU 이미지 데이터입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool SetPixels(Core::Uint32 SizeX, Core::Uint32 SizeY, Asset::ETextureFormat Format, Core::TArray<Core::Uint8> Pixels);

        /**
         * @brief 텍스처의 가로 픽셀 수를 반환합니다.
         * @return 텍스처의 가로 픽셀 수를 나타내는 정수 값입니다.
         */
        Core::Uint32 GetSizeX() const override;

        /**
         * @brief 텍스처의 세로 픽셀 수를 반환합니다.
         * @return 텍스처의 세로 픽셀 수를 나타내는 정수 값입니다.
         */
        Core::Uint32 GetSizeY() const override;

        /**
         * @brief 텍스처의 밉 레벨 수를 반환합니다.
         * @return 텍스처의 밉 레벨 수를 나타내는 정수 값입니다.
         */
        Core::Uint32 GetMipLevels() const;

        /**
         * @brief 텍스처의 픽셀 또는 깊이 형식을 반환합니다.
         * @return 텍스처 형식입니다.
         */
        Asset::ETextureFormat GetFormat() const override;

        /**
         * @brief CPU 메모리에 보관한 픽셀 바이트 배열을 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Core::TArray<Core::Uint8>& GetPixels() const;

        /**
         * @brief 텍스처를 sRGB 색 공간으로 해석하는지 반환합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        bool IsSRGB() const override;

        /**
         * @brief 현재 텍스처 형식과 용도에 맞는지 검증하여 sRGB 설정을 변경합니다.
         * @param[in] SRGB 이미지를 sRGB 색 공간으로 해석할지 나타냅니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool SetSRGB(bool SRGB);

        /**
         * @brief 텍스처에 설정된 GPU 사용 용도 플래그를 반환합니다.
         * @return 텍스처 사용 용도 플래그입니다.
         */
        Asset::ETextureUsage GetUsage() const;

        /**
         * @brief 현재 형식·색 공간과 호환되는 GPU 사용 용도 플래그를 설정합니다.
         * @param[in] Usage 셰이더 읽기·색상 출력·깊이 출력의 용도 플래그입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool SetUsage(Asset::ETextureUsage Usage);

        /**
         * @brief 크기·형식·용도와 픽셀 또는 연결된 리소스 구성이 유효한지 검사합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        bool IsValid() const override;

        /**
         * @brief 현재 CPU 데이터를 사용하여 Direct3D 리소스를 생성합니다.
         * @param[in] Device GPU 리소스를 생성할 Direct3D 장치의 비소유 주소입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool CreateResources(ID3D11Device* Device);

        /**
         * @brief 외부 Direct3D 텍스처에 COM 참조를 유지하고 지정 용도의 뷰를 생성합니다.
         * @param[in] Texture 데이터를 읽거나 뷰를 생성할 텍스처이며 소유권을 이전하지 않습니다.
         * @param[in] Usage 셰이더 읽기·색상 출력·깊이 출력의 용도 플래그입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool AttachResource(ID3D11Texture2D* Texture, Asset::ETextureUsage Usage);

        /**
         * @brief 소유한 GPU 리소스와 뷰를 해제하고 메모리 집계를 초기화합니다.
         */
        void ReleaseResources();

        /**
         * @brief 사용할 수 있는 GPU 리소스를 보유하는지 검사합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        bool HasResources() const;

        /**
         * @brief Direct3D 2차원 텍스처의 비소유 주소를 반환합니다.
         * @return GPU 자원의 비소유 주소이며, 아직 생성하지 않았으면 nullptr입니다.
         */
        ID3D11Texture2D* GetTexture() const;

        /**
         * @brief 셰이더에서 읽을 리소스 뷰의 비소유 주소를 반환합니다.
         * @return GPU 자원의 비소유 주소이며, 아직 생성하지 않았으면 nullptr입니다.
         */
        ID3D11ShaderResourceView* GetShaderResourceView() const;

        /**
         * @brief 렌더 타깃 출력 뷰의 비소유 주소를 반환합니다.
         * @return GPU 자원의 비소유 주소이며, 아직 생성하지 않았으면 nullptr입니다.
         */
        ID3D11RenderTargetView* GetRenderTargetView() const;

        /**
         * @brief 깊이·스텐실 출력 뷰의 비소유 주소를 반환합니다.
         * @return GPU 자원의 비소유 주소이며, 아직 생성하지 않았으면 nullptr입니다.
         */
        ID3D11DepthStencilView* GetDepthStencilView() const;

        /**
         * @brief 현재 보유한 GPU 리소스의 추정 바이트 수를 반환합니다.
         * @return 현재 보유한 GPU 리소스의 추정 바이트 수를 나타내는 정수 값입니다.
         */
        std::size_t GetResourceBytes() const;

    private:
        /**
         * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
         */
        ReflectMembers();

        /**
         * @brief DirectX 이미지에서 지원 형식의 픽셀과 DDS 데이터를 구성합니다.
         * @param[in,out] SourceImage 변환하거나 복사할 원본 DirectX 이미지입니다.
         * @param[in] SRGB 이미지를 sRGB 색 공간으로 해석할지 나타냅니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool SetImageData(DirectX::ScratchImage& SourceImage, bool SRGB);

        /**
         * @brief 가로·세로 크기가 Direct3D 텍스처의 허용 범위에 있는지 검사합니다.
         * @param[in] SizeX 텍스처의 가로 픽셀 수입니다.
         * @param[in] SizeY 텍스처의 세로 픽셀 수입니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        static bool IsValidSize(Core::Uint32 SizeX, Core::Uint32 SizeY);

        /**
         * @brief 지정한 형식이 깊이 또는 깊이·스텐실 형식인지 검사합니다.
         * @param[in] Format 검사할 텍스처 형식입니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        static bool IsDepthFormat(Asset::ETextureFormat Format);

        /**
         * @brief 픽셀 형식·용도 플래그·sRGB 설정의 조합이 지원되는지 검사합니다.
         * @param[in] Format 검사할 텍스처 형식입니다.
         * @param[in] Usage 셰이더 읽기·색상 출력·깊이 출력의 용도 플래그입니다.
         * @param[in] SRGB 이미지를 sRGB 색 공간으로 해석할지 나타냅니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        static bool ValidateUsage(Asset::ETextureFormat Format, Asset::ETextureUsage Usage, bool SRGB);

        /**
         * @brief 텍스처 형식의 픽셀 하나가 차지하는 바이트 수를 반환합니다.
         * @param[in] Format 픽셀 크기를 구할 텍스처 형식입니다.
         * @return 텍스처 형식의 픽셀 하나가 차지하는 바이트 수를 나타내는 정수 값입니다.
         */
        static Core::Uint32 GetBytesPerPixel(Asset::ETextureFormat Format);

        /**
         * @brief 텍스처 용도 플래그를 Direct3D 바인딩 플래그로 변환합니다.
         * @param[in] Usage 셰이더 읽기·색상 출력·깊이 출력의 용도 플래그입니다.
         * @return D3D11_BIND_FLAG 비트 조합입니다.
         */
        static Core::Uint32 GetBindFlags(Asset::ETextureUsage Usage);

        /**
         * @brief 픽셀 형식과 색 공간에 대응하는 Direct3D 리소스 형식을 반환합니다.
         * @param[in] Format 변환할 텍스처 형식입니다.
         * @param[in] SRGB 이미지를 sRGB 색 공간으로 해석할지 나타냅니다.
         * @return 리소스 생성에 사용할 DXGI 형식입니다.
         */
        static DXGI_FORMAT GetResourceFormat(Asset::ETextureFormat Format, bool SRGB);

        /**
         * @brief 픽셀·깊이 형식과 색 공간에 대응하는 셰이더 리소스 뷰 형식을 반환합니다.
         * @param[in] Format 변환할 텍스처 형식입니다.
         * @param[in] SRGB 이미지를 sRGB 색 공간으로 해석할지 나타냅니다.
         * @return 셰이더 리소스 뷰에 사용할 DXGI 형식입니다.
         */
        static DXGI_FORMAT GetShaderResourceFormat(Asset::ETextureFormat Format, bool SRGB);

        /**
         * @brief 깊이 형식에 대응하는 Direct3D 깊이·스텐실 뷰 형식을 반환합니다.
         * @param[in] Format 변환할 깊이 텍스처 형식입니다.
         * @return 깊이·스텐실 뷰에 사용할 DXGI 형식입니다.
         */
        static DXGI_FORMAT GetDepthStencilFormat(Asset::ETextureFormat Format);

        /**
         * @brief 텍스처 용도에 따라 셰이더 리소스·렌더 타깃·깊이 뷰를 생성합니다.
         * @param[in] Device GPU 리소스를 생성할 Direct3D 장치의 비소유 주소입니다.
         * @param[in] Texture 데이터를 읽거나 뷰를 생성할 텍스처이며 소유권을 이전하지 않습니다.
         * @param[in] Format 생성할 뷰의 텍스처 형식입니다.
         * @param[in] Usage 셰이더 읽기·색상 출력·깊이 출력의 용도 플래그입니다.
         * @param[in] SRGB 이미지를 sRGB 색 공간으로 해석할지 나타냅니다.
         * @param[out] ShaderResourceView 생성한 셰이더 리소스 뷰를 받을 COM 소유 참조입니다.
         * @param[out] RenderTargetView 생성한 렌더 타깃 뷰를 받을 COM 소유 참조입니다.
         * @param[out] DepthStencilView 생성한 깊이·스텐실 뷰를 받을 COM 소유 참조입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        static bool CreateViews(ID3D11Device* Device, ID3D11Texture2D* Texture, Asset::ETextureFormat Format, Asset::ETextureUsage Usage, bool SRGB, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& ShaderResourceView, Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& RenderTargetView, Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& DepthStencilView);

    private:
        /**
         * @brief 텍스처의 가로 픽셀 수입니다.
         */
        Core::Uint32 mSizeX{};
        ReflectProperty(mSizeX, "SizeX", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 텍스처의 세로 픽셀 수입니다.
         */
        Core::Uint32 mSizeY{};
        ReflectProperty(mSizeY, "SizeY", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 텍스처에 포함된 밉 레벨 수입니다.
         */
        Core::Uint32 mMipLevels{1};
        ReflectProperty(mMipLevels, "MipLevels", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 텍스처의 픽셀 또는 깊이·스텐실 형식입니다.
         */
        Asset::ETextureFormat mFormat{Asset::ETextureFormat::RGBA8};
        ReflectProperty(mFormat, "Format", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief CPU에서 보관하는 텍스처 픽셀 바이트 배열입니다.
         */
        Core::TArray<Core::Uint8> mPixels{};
        ReflectProperty(mPixels, "Pixels", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 밉 레벨과 형식을 유지하기 위해 보존한 DDS 이미지 바이트입니다.
         */
        Core::TArray<Core::Uint8> mDDSData{};
        ReflectProperty(mDDSData, "DDSData", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 픽셀을 sRGB 색 공간으로 해석하는지 나타냅니다.
         */
        bool mSRGB{true};
        ReflectAccessor(mSRGB, IsSRGB, SetSRGB, "SRGB", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 셰이더 읽기·렌더 타깃·깊이 출력의 사용 용도 비트입니다.
         */
        Asset::ETextureUsage mUsage{Asset::ETextureUsage::ShaderResource};
        ReflectAccessor(mUsage, GetUsage, SetUsage, "Usage", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Transient);

        /**
         * @brief 이 객체가 COM 참조를 보유하는 Direct3D 텍스처입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Texture2D> mTexture{};
        ReflectProperty(mTexture, "Texture", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 셰이더 샘플링에 사용하는 소유 Direct3D 뷰입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mShaderResourceView{};
        ReflectProperty(mShaderResourceView, "ShaderResourceView", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 색상 렌더 타깃에 사용하는 소유 Direct3D 뷰입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mRenderTargetView{};
        ReflectProperty(mRenderTargetView, "RenderTargetView", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 깊이·스텐실 출력에 사용하는 소유 Direct3D 뷰입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mDepthStencilView{};
        ReflectProperty(mDepthStencilView, "DepthStencilView", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 외부에서 생성한 Direct3D 텍스처를 연결했는지 나타냅니다.
         */
        bool mExternalResource{};
        ReflectProperty(mExternalResource, "ExternalResource", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 현재 보유한 GPU 리소스의 추정 바이트 수입니다.
         */
        std::size_t mResourceBytes{};
        ReflectProperty(mResourceBytes, "ResourceBytes", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
