#include "Asset/Import/FFBXImageProcessor.h"
#include "Core/FilePath.h"

#include <DirectXTex.h>
#include <algorithm>
#include <array>
#include <d3d11.h>
#include <fstream>
#include <limits>
#include <string_view>
#include <utility>
#include <wincodec.h>

using namespace fbxsdk;

namespace Asset {

    void Asset::FFBXImageProcessor::RegisterEmbeddedCallback(FbxManager& Manager, FbxImporter& Importer) {
        auto* Callback{FbxEmbeddedFileCallback::Create(&Manager, "")};

        // FBX SDK는 C 함수 포인터만 받으므로 캡처 없는 람다로 받고, 사용자 데이터로 넘긴 처리기 주소에서 멤버 함수로 넘깁니다.
        Callback->RegisterReadFunction([](void* UserData, FbxClassId DataHint, const char* FileName, const void* Buffer, std::size_t Size) {
            if (UserData == nullptr) {
                return FbxCallback::eFailure;
            }

            return static_cast<Asset::FFBXImageProcessor*>(UserData)->StoreEmbeddedData(DataHint, FileName, Buffer, Size);
        }, this);

        // 콜백을 등록하면 SDK가 내장 미디어를 .fbm 폴더로 추출하지 않고 이 콜백에 바이트를 넘깁니다.
        Importer.SetEmbeddedFileReadCallback(Callback);
    }

    void Asset::FFBXImageProcessor::ResolveTextures(const FbxScene& Scene, const std::filesystem::path& SourceDirectory) {
        // Import가 끝나면 텍스처와 FbxVideo의 연결, FbxVideo 속성, 내장 바이트를 모두 알 수 있으므로 파일 텍스처마다 한 번씩 판별합니다.
        // 판별 실패는 바로 임포트를 실패시키지 않고 mTextureErrors에 기록해 두며, 재질 속성에서 그 텍스처를 실제로 읽을 때 ReadTexture가 보고합니다.
        // 그래서 어느 재질에서도 쓰지 않는 깨진 텍스처는 임포트를 막지 않습니다.
        for (int Index{}; Index < Scene.GetSrcObjectCount<FbxFileTexture>(); ++Index) {
            auto* Texture{Scene.GetSrcObject<FbxFileTexture>(Index)};
            Asset::FMeshImportTexture Resolved{};
            Core::FString Error{};

            if (ResolveTexture(*Texture, SourceDirectory, Resolved, Error)) {
                mTextures.insert_or_assign(Texture, std::move(Resolved));
            } else {
                mTextureErrors.insert_or_assign(Texture, std::move(Error));
            }
        }

        // LocateTexture가 필요한 바이트를 모두 판별 결과로 복사했으므로 원본 보관분을 해제합니다.
        mEmbeddedData.clear();
    }

    bool Asset::FFBXImageProcessor::ReadTexture(FbxProperty Property, std::span<const Asset::EMeshImportTextureShape> Accepted, Asset::FMeshImportTexture& Result, Core::FString& Error) const {
        // 재질에 이 속성이 없으면 텍스처가 없는 슬롯으로 보고 성공합니다.
        if (!Property.IsValid()) {
            return true;
        }

        // 여러 텍스처를 섞는 레이어드 텍스처는 단일 텍스처 슬롯으로 표현할 수 없습니다.
        if (Property.GetSrcObjectCount<FbxLayeredTexture>() != 0) {
            Error = "Layered FBX material textures are not supported";
            return false;
        }

        auto* Texture{Property.GetSrcObject<FbxFileTexture>()};

        // 속성은 있지만 파일 텍스처가 연결되지 않았으면 색상 값만 쓰는 슬롯입니다.
        if (Texture == nullptr) {
            return true;
        }

        // ResolveTextures가 미뤄 둔 판별 실패를 이 텍스처가 실제로 쓰이는 지금 보고합니다.
        if (const auto Failed{mTextureErrors.find(Texture)}; Failed != mTextureErrors.end()) {
            Error = Failed->second;
            return false;
        }

        const Core::FString RelativeName{Texture->GetRelativeFileName()};
        const Core::FString Name{RelativeName.empty() ? Core::FString{Texture->GetFileName()} : RelativeName};
        const auto Resolved{mTextures.find(Texture)};

        // 장면의 모든 파일 텍스처는 성공이나 실패 중 하나로 기록되므로, 여기에 오면 ResolveTextures 호출이 빠진 것입니다.
        if (Resolved == mTextures.end()) {
            Error = "FBX material texture was not resolved: " + Name;
            return false;
        }

        // 판별은 ResolveTextures가 끝냈으므로 여기서는 재질 슬롯이 그 형태를 받아들이는지만 확인합니다.
        const auto Shape{Resolved->second.GetShape()};

        if (std::find(Accepted.begin(), Accepted.end(), Shape) == Accepted.end()) {
            Error = Core::FString{"This material slot does not support FBX texture shape '"} + ShapeName(Shape) + "': " + Name;
            return false;
        }

        Result = Resolved->second;
        return true;
    }

    Core::FString Asset::FFBXImageProcessor::TextureKey(Core::FString Path) const {
        // FBX에 기록된 경로와 콜백이 넘긴 경로는 구분자와 대소문자가 다를 수 있으므로 슬래시와 소문자로 맞춥니다.
        std::replace(Path.begin(), Path.end(), '\\', '/');
        return Core::FilePath::ToGenericUtf8(Core::FilePath::ToLower(Core::FilePath::FromUtf8(Path).lexically_normal()));
    }

    FbxCallback::State Asset::FFBXImageProcessor::StoreEmbeddedData(FbxClassId DataHint, const char* FileName, const void* Buffer, std::size_t Size) {
        // 무효한 인수는 실패를 반환합니다.
        if (FileName == nullptr || Buffer == nullptr || Size == 0) {
            return FbxCallback::eFailure;
        }

        // 텍스처 이미지의 바이트는 FbxVideo 객체에 속하므로 FbxVideo용 데이터만 보관합니다.
        // FbxVideo에는 정적 이미지(BaseColor, Normal, Roughness 등), 동영상 클립, 001·002처럼 이어지는 이미지 시퀀스가 모두 해당합니다.
        if (DataHint == FbxVideo::ClassId) {
            const auto* Bytes{static_cast<const Core::Uint8*>(Buffer)};

            // 콜백 시점에는 이 FbxVideo가 어느 텍스처에 연결되는지와 시퀀스 여부를 알 수 없으므로 바이트만 보관하고, 판별은 Import 이후 ResolveTextures가 맡습니다.
            // Buffer는 콜백이 끝나면 무효해지므로 복사본을 보관합니다.
            mEmbeddedData.insert_or_assign(TextureKey(FileName), Core::TArray<Core::Uint8>{Bytes, Bytes + Size});
        }

        // 보관하지 않은 데이터도 SDK가 디스크에 추출하지 않도록 eHandled를 반환합니다.
        return FbxCallback::eHandled;
    }

    const char* Asset::FFBXImageProcessor::ShapeName(Asset::EMeshImportTextureShape Shape) const {
        switch (Shape) {
        case Asset::EMeshImportTextureShape::Texture2D:
            return "2D image";
        case Asset::EMeshImportTextureShape::Texture2DArray:
            return "2D image array";
        case Asset::EMeshImportTextureShape::TextureCube:
            return "cubemap";
        case Asset::EMeshImportTextureShape::Texture3D:
            return "volume image";
        case Asset::EMeshImportTextureShape::Animated:
            return "animated image";
        case Asset::EMeshImportTextureShape::ImageSequence:
            return "image sequence";
        default:
            return "unknown image";
        }
    }

    bool Asset::FFBXImageProcessor::HasPNGAnimation(std::span<const Core::Uint8> Data, const std::filesystem::path& Path) const {
        std::ifstream Stream{};

        // 내장 바이트가 없으면 외부 파일을 열어 필요한 청크 헤더만 읽습니다.
        if (Data.empty()) {
            Stream.open(Path, std::ios::binary);

            if (!Stream) {
                return false;
            }
        }

        // 메모리와 파일을 같은 방식으로 읽도록 오프셋 기반 읽기 함수를 만듭니다. 범위를 벗어나면 false입니다.
        const auto Read{[&](std::size_t Offset, std::span<Core::Uint8> Out) {
            if (Data.empty()) {
                Stream.seekg(static_cast<std::streamoff>(Offset));
                return static_cast<bool>(Stream.read(reinterpret_cast<char*>(Out.data()), static_cast<std::streamsize>(Out.size())));
            }

            if (Offset > Data.size() || Out.size() > Data.size() - Offset) {
                return false;
            }

            std::copy_n(Data.begin() + static_cast<std::ptrdiff_t>(Offset), Out.size(), Out.begin());
            return true;
        }};

        // PNG 시그니처가 아니면 APNG일 수 없습니다.
        constexpr std::array<Core::Uint8, 8> Signature{137, 80, 78, 71, 13, 10, 26, 10};
        std::array<Core::Uint8, 8> Header{};

        if (!Read(0, Header) || Header != Signature) {
            return false;
        }

        // 청크는 길이 4바이트·타입 4바이트·데이터·CRC 4바이트입니다. 명세상 acTL은 첫 IDAT보다 앞에 오므로 IDAT를 만나면 멈춥니다.
        // 시그니처와 같은 8바이트 버퍼를 재사용해 각 청크의 길이와 타입만 읽고 데이터는 건너뜁니다.
        for (std::size_t Offset{Signature.size()}; Read(Offset, Header);) {
            const std::string_view Type{reinterpret_cast<const char*>(Header.data()) + 4, 4};

            if (Type == "acTL") {
                return true;
            }

            if (Type == "IDAT") {
                return false;
            }

            // 길이는 빅엔디언이며 데이터 길이만 담으므로 길이·타입·CRC 12바이트를 더해 다음 청크로 넘어갑니다.
            const std::size_t Length{(static_cast<std::size_t>(Header[0]) << 24) | (static_cast<std::size_t>(Header[1]) << 16) | (static_cast<std::size_t>(Header[2]) << 8) | Header[3]};
            Offset += Length + 12;
        }

        return false;
    }

    bool Asset::FFBXImageProcessor::ClassifyMetadata(const DirectX::TexMetadata& Metadata, const Core::FString& Name, Asset::EMeshImportTextureShape& Shape, Core::FString& Error) const {
        std::size_t Limit{};
        std::size_t LayerLimit{1};

        // 차원·큐브맵·배열 크기로 형태를 정하고, 그 형태에 맞는 D3D11 한 변 크기와 배열 크기 한계를 고릅니다.
        if (Metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D) {
            Shape = Asset::EMeshImportTextureShape::Texture3D;
            Limit = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        } else if (Metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE2D && Metadata.IsCubemap()) {
            // 큐브맵도 2D 차원이므로 일반 2D보다 먼저 확인합니다.
            Shape = Asset::EMeshImportTextureShape::TextureCube;
            Limit = D3D11_REQ_TEXTURECUBE_DIMENSION;
            LayerLimit = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        } else if (Metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE2D) {
            Shape = Metadata.arraySize > 1 ? Asset::EMeshImportTextureShape::Texture2DArray : Asset::EMeshImportTextureShape::Texture2D;
            Limit = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            LayerLimit = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        } else {
            // 1D 텍스처는 재질 텍스처로 쓰지 않습니다.
            Error = "Unsupported FBX texture image dimension: " + Name;
            return false;
        }

        // 깊이는 3D 텍스처에서만 의미가 있으므로 다른 형태는 1로 봅니다.
        const std::size_t Depth{Shape == Asset::EMeshImportTextureShape::Texture3D ? Metadata.depth : 1};

        if (Metadata.width == 0 || Metadata.height == 0 || Metadata.width > Limit || Metadata.height > Limit || Depth > Limit || Metadata.arraySize == 0 || Metadata.arraySize > LayerLimit) {
            Shape = Asset::EMeshImportTextureShape::Unknown;
            Error = "FBX texture image dimensions exceed texture limits: " + Name;
            return false;
        }

        return true;
    }

    bool Asset::FFBXImageProcessor::InspectWICImage(std::span<const Core::Uint8> Data, const std::filesystem::path& Path, Asset::EMeshImportTextureShape& Shape, Core::FString& Error) const {
        const Core::FString Name{Core::FilePath::ToUtf8(Path)};

        // WIC 메모리 API는 UINT 크기만 받습니다.
        if (Data.size() > std::numeric_limits<UINT>::max()) {
            Error = "FBX texture image exceeds decoder limits: " + Name;
            return false;
        }

        // WIC는 COM 객체이므로 이 스레드에서 COM을 초기화합니다. 다른 모드로 이미 초기화되었으면 그대로 씁니다.
        const HRESULT InitializeResult{CoInitializeEx(nullptr, COINIT_MULTITHREADED)};

        if (FAILED(InitializeResult) && InitializeResult != RPC_E_CHANGED_MODE) {
            Error = "Cannot initialize the FBX image decoder: " + Name;
            return false;
        }

        DirectX::TexMetadata Metadata{};
        HRESULT Result{E_FAIL};

        // WIC_FLAGS_ALL_FRAMES로 열어야 GIF 같은 다중 프레임 이미지의 프레임 수가 arraySize에 담깁니다.
        if (Data.empty()) {
            Result = DirectX::GetMetadataFromWICFile(Path.c_str(), DirectX::WIC_FLAGS_ALL_FRAMES, Metadata);
        } else {
            Result = DirectX::GetMetadataFromWICMemory(Data.data(), Data.size(), DirectX::WIC_FLAGS_ALL_FRAMES, Metadata);
        }

        if (SUCCEEDED(Result) && Metadata.arraySize == 0) {
            Error = "FBX texture image has no frames: " + Name;
            Result = WINCODEC_ERR_UNSUPPORTEDOPERATION;
        }

        if (SUCCEEDED(Result) && (Metadata.width == 0 || Metadata.height == 0 || Metadata.width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || Metadata.height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)) {
            Error = "FBX texture image dimensions exceed texture limits: " + Name;
            Result = WINCODEC_ERR_UNSUPPORTEDOPERATION;
        }

        // WIC 형식은 항상 2D이므로 프레임이 여러 개면 애니메이션, 하나면 정적 2D 이미지입니다.
        if (SUCCEEDED(Result)) {
            Shape = Metadata.arraySize > 1 ? Asset::EMeshImportTextureShape::Animated : Asset::EMeshImportTextureShape::Texture2D;
        }

        // 이 호출이 초기화한 경우에만 COM을 해제합니다.
        if (SUCCEEDED(InitializeResult)) {
            CoUninitialize();
        }

        if (FAILED(Result)) {
            if (Error.empty()) {
                Error = "Unsupported or invalid FBX texture image: " + Name;
            }

            return false;
        }

        return true;
    }

    bool Asset::FFBXImageProcessor::InspectImage(std::span<const Core::Uint8> Data, const std::filesystem::path& Path, Asset::EMeshImportTextureShape& Shape, Core::FString& Error) const {
        const Core::FString Name{Core::FilePath::ToUtf8(Path)};
        Shape = Asset::EMeshImportTextureShape::Unknown;

        const std::wstring Extension{Core::FilePath::ToLowerExtension(Path)};

        // DDS·TGA·HDR은 WIC가 읽지 못하므로 DirectXTex 전용 함수로 읽고, 나머지(PNG·JPG·GIF·BMP 등)는 WIC로 읽습니다.
        if (Extension != L".dds" && Extension != L".tga" && Extension != L".hdr") {
            if (!InspectWICImage(Data, Path, Shape, Error)) {
                return false;
            }

            // WIC는 APNG를 단일 프레임으로 읽으므로 정적 2D로 나온 PNG는 애니메이션 청크를 따로 확인합니다.
            if (Shape == Asset::EMeshImportTextureShape::Texture2D && HasPNGAnimation(Data, Path)) {
                Shape = Asset::EMeshImportTextureShape::Animated;
            }

            return true;
        }

        DirectX::TexMetadata Metadata{};
        HRESULT Result{E_FAIL};

        // 내장 바이트가 있으면 메모리에서, 없으면 외부 파일에서 헤더 메타데이터만 읽습니다.
        if (Data.empty()) {
            if (Extension == L".dds") {
                Result = DirectX::GetMetadataFromDDSFile(Path.c_str(), DirectX::DDS_FLAGS_NONE, Metadata);
            } else if (Extension == L".tga") {
                Result = DirectX::GetMetadataFromTGAFile(Path.c_str(), DirectX::TGA_FLAGS_NONE, Metadata);
            } else {
                Result = DirectX::GetMetadataFromHDRFile(Path.c_str(), Metadata);
            }
        } else {
            if (Extension == L".dds") {
                Result = DirectX::GetMetadataFromDDSMemory(Data.data(), Data.size(), DirectX::DDS_FLAGS_NONE, Metadata);
            } else if (Extension == L".tga") {
                Result = DirectX::GetMetadataFromTGAMemory(Data.data(), Data.size(), DirectX::TGA_FLAGS_NONE, Metadata);
            } else {
                Result = DirectX::GetMetadataFromHDRMemory(Data.data(), Data.size(), Metadata);
            }
        }

        if (FAILED(Result)) {
            Error = "Unsupported or invalid FBX texture image: " + Name;
            return false;
        }

        // DDS는 큐브맵·배열·3D를 담을 수 있으므로 메타데이터로 형태를 분류합니다.
        return ClassifyMetadata(Metadata, Name, Shape, Error);
    }

    bool Asset::FFBXImageProcessor::LocateTexture(const FbxFileTexture& Texture, const std::filesystem::path& SourceDirectory, Asset::FMeshImportTexture& Result, Core::FString& Error) const {
        const Core::FString FileName{Texture.GetFileName()};
        const Core::FString RelativeName{Texture.GetRelativeFileName()};

        // 내장 바이트가 우선입니다. 콜백이 넘긴 이름이 기록된 경로와 상대 경로 중 어느 쪽과 같은지 알 수 없으므로 둘 다 키로 찾아봅니다.
        for (const auto& Name : {FileName, RelativeName}) {
            if (const auto Embedded{mEmbeddedData.find(TextureKey(Name))}; !Name.empty() && Embedded != mEmbeddedData.end()) {
                // mEmbeddedData는 ResolveTextures 끝에 비우므로 바이트를 복사해 결과에 담습니다.
                Result = Asset::FMeshImportTexture::FromEmbedded(Embedded->first, Embedded->second);
                return true;
            }
        }

        // 내장되지 않았으면 원본 FBX 폴더 기준으로 상대 경로, 기록된 경로, 파일 이름만 순서대로 찾습니다.
        // 기록된 경로가 다른 PC의 절대 경로일 수 있으므로 마지막에는 파일 이름만으로 찾아봅니다.
        const std::array<std::filesystem::path, 3> Candidates{SourceDirectory / Core::FilePath::FromUtf8(RelativeName), SourceDirectory / Core::FilePath::FromUtf8(FileName), SourceDirectory / Core::FilePath::FromUtf8(FileName).filename()};

        for (const auto& Candidate : Candidates) {
            std::error_code Code{};

            if (std::filesystem::is_regular_file(Candidate, Code) && !Code) {
                Result = Asset::FMeshImportTexture{Candidate.lexically_normal()};
                return true;
            }
        }

        Error = "Cannot find FBX material texture: " + (RelativeName.empty() ? FileName : RelativeName);
        return false;
    }

    bool Asset::FFBXImageProcessor::InspectTexture(Asset::FMeshImportTexture& Texture, Core::FString& Error) const {
        auto Shape{Asset::EMeshImportTextureShape::Unknown};
        const bool Embedded{Texture.GetSource() == Asset::EMeshImportTextureSource::Embedded};
        // 외부 파일은 바이트를 넘기지 않아 InspectImage가 파일에서 헤더만 읽게 합니다.
        const std::span<const Core::Uint8> Data{Embedded ? std::span<const Core::Uint8>{Texture.GetEmbeddedData()} : std::span<const Core::Uint8>{}};
        // 내장 텍스처도 확장자로 디코더를 고르므로 내장 참조 이름을 경로로 넘깁니다.
        const std::filesystem::path Path{Embedded ? Core::FilePath::FromUtf8(Texture.GetEmbeddedReference()) : Texture.GetFilePath()};

        if (!InspectImage(Data, Path, Shape, Error)) {
            return false;
        }

        Texture.SetShape(Shape);
        return true;
    }

    bool Asset::FFBXImageProcessor::IsImageSequence(const FbxFileTexture& Texture) const {
        // 시퀀스 여부는 이미지 바이트가 아니라 텍스처에 연결된 FbxVideo의 속성입니다.
        for (int Index{}; Index < Texture.GetSrcObjectCount<FbxVideo>(); ++Index) {
            if (Texture.GetSrcObject<FbxVideo>(Index)->GetImageSequence()) {
                return true;
            }
        }

        return false;
    }

    bool Asset::FFBXImageProcessor::ResolveTexture(const FbxFileTexture& Texture, const std::filesystem::path& SourceDirectory, Asset::FMeshImportTexture& Result, Core::FString& Error) const {
        // 이미지를 찾은 뒤 헤더로 형태를 판별합니다. 어느 단계든 실패하면 Error에 원인이 남습니다.
        if (!LocateTexture(Texture, SourceDirectory, Result, Error) || !InspectTexture(Result, Error)) {
            return false;
        }

        // 이미지 시퀀스는 바이트가 아니라 연결된 FbxVideo 속성으로만 알 수 있으므로 첫 프레임으로 판별한 형태를 덮어씁니다.
        if (IsImageSequence(Texture)) {
            Result.SetShape(Asset::EMeshImportTextureShape::ImageSequence);
        }

        return true;
    }

}
