/**
 * @file FViewerCapture.cpp
 * @brief 뷰어 렌더 타깃과 독립된 최종 컬러 텍스처의 수명을 소유합니다.
 * @ingroup Viewer
 */

#include "Viewer/FViewerCapture.h"

#include "Asset/UTexture2D.h"

using namespace Viewer;

FViewerCapture::FViewerCapture(std::unique_ptr<Asset::UTexture2D> Texture)
	: mTexture{std::move(Texture)} {
}

FViewerCapture::~FViewerCapture() = default;

void* FViewerCapture::GetNativeTexture() const {
    return mTexture != nullptr ? mTexture->GetShaderResourceView() : nullptr;
}

Core::Uint32 FViewerCapture::GetWidth() const {
    return mTexture != nullptr ? mTexture->GetSizeX() : 0;
}

Core::Uint32 FViewerCapture::GetHeight() const {
    return mTexture != nullptr ? mTexture->GetSizeY() : 0;
}

std::size_t FViewerCapture::GetResourceBytes() const {
    return mTexture != nullptr ? mTexture->GetResourceBytes() : 0;
}

const Asset::UTexture2D* FViewerCapture::GetTexture() const {
    return mTexture.get();
}
