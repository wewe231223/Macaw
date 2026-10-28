/**
 * @file UBillboardComponent.cpp
 * @brief 카메라 정렬과 고정 평면 텍스처 스프라이트를 제공합니다.
 * @ingroup Engine
 */

#include "Engine/Component/UBillboardComponent.h"
#include "Engine/FAssetManager.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/FMeshData.h"
#include "Engine/UWorld.h"
#include "Core/FilePath.h"
#include "Core/Base/ErrorHandler.h"

#include <algorithm>
#include <cmath>

UBillboardComponent::UBillboardComponent() = default;
UBillboardComponent::~UBillboardComponent() = default;

const FString& UBillboardComponent::GetTexturePath() const {
    return mTexturePath;
}

bool UBillboardComponent::SetTexturePath(const FString& Path) {
    const std::filesystem::path AssetPath{FilePath::FromUtf8(Path)};
    if (Path.find('\0') != FString::npos || (!Path.empty() && (AssetPath.has_root_path() || AssetPath.extension() != L".masset"))) {
        return false;
    }
    for (const auto& Part : AssetPath) {
        if (Part == L"..") {
            return false;
        }
    }
    UTexture2D* Texture{LoadTexture(Path)};
    if (!Path.empty() && GetWorld() != nullptr && GetWorld()->GetAssets() != nullptr && Texture == nullptr) {
        return false;
    }
    mTexturePath = Path;
    mTexture = Texture;
    MarkRenderStateDirty();
    return true;
}

UTexture2D* UBillboardComponent::LoadTexture(const FString& Path) const {
    FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};
    if (Assets == nullptr || Path.empty()) {
        return nullptr;
    }
    FString Error{};
    UTexture2D* Texture{Assets->LoadTexture(FilePath::FromUtf8(Path), Error)};
    if (Texture == nullptr) {
        ErrorHandler::Report("Sprite", "Cannot load texture '" + Path + "': " + Error, ErrorHandler::EErrorLevel::Warning);
    }
    return Texture;
}

bool UBillboardComponent::ResolveTexture() {
    mTexture = LoadTexture(mTexturePath);
    MarkRenderStateDirty();
    return mTexturePath.empty() || mTexture != nullptr;
}

UTexture2D* UBillboardComponent::GetTexture() const {
    return mTexture;
}

const FVector2& UBillboardComponent::GetSize() const {
    return mSize;
}

bool UBillboardComponent::SetSize(const FVector2& Size) {
    if (!std::isfinite(Size.x) || !std::isfinite(Size.y) || Size.x <= 0.0f || Size.y <= 0.0f || Size.x > 10000.0f || Size.y > 10000.0f) {
        return false;
    }
    mSize = Size;
    MarkRenderStateDirty();
    return true;
}

const FVector4& UBillboardComponent::GetColor() const {
    return mColor;
}

bool UBillboardComponent::SetColor(const FVector4& Color) {
    const auto Values{Color.ToArray()};
    if (!std::all_of(Values.begin(), Values.end(), [](float Value) {
        return std::isfinite(Value) && Value >= 0.0f && Value <= 1.0f;
    })) {
        return false;
    }
    mColor = Color;
    MarkRenderStateDirty();
    return true;
}

bool UBillboardComponent::IsScreenAligned() const {
    return mScreenAligned;
}

void UBillboardComponent::SetScreenAligned(bool ScreenAligned) {
    mScreenAligned = ScreenAligned;
    MarkRenderStateDirty();
}

bool UBillboardComponent::IsVisible() const {
    return mVisible;
}

void UBillboardComponent::SetVisible(bool Visible) {
    mVisible = Visible;
    MarkRenderStateDirty();
}

EPrimitiveType UBillboardComponent::GetPrimitiveType() const {
    return EPrimitiveType::Plane;
}

const FMeshData& UBillboardComponent::GetMeshData() const {
    static const FMeshData Empty{};
    return Empty;
}

FPrimitiveSceneProxy UBillboardComponent::CreateSceneProxy() const {
    FPrimitiveSceneProxy Proxy{UPrimitiveComponent::CreateSceneProxy()};
    Proxy.mBounds = GetWorldBounds();
    Proxy.mSprite = FDrawSprite{mTexture, mSize, mColor, mScreenAligned};
    return Proxy;
}

FBox UBillboardComponent::GetWorldBounds() const {
    const FMatrix World{GetWorldMatrix()};
    const FVector Center{World.TransformPosition({})};
    const float Width{mSize.x * World.TransformVector({0.0f, 1.0f, 0.0f}).Length()};
    const float Height{mSize.y * World.TransformVector({0.0f, 0.0f, 1.0f}).Length()};
    const float Radius{0.5f * std::sqrt(Width * Width + Height * Height)};
    const FVector Extent{Radius, Radius, Radius};
    return {Center - Extent, Center + Extent};
}

bool UBillboardComponent::ShouldCreateRenderState() const {
    return mVisible && mColor.w > 0.0f && (mTexturePath.empty() || mTexture != nullptr);
}

void UBillboardComponent::OnRegister() {
    ResolveTexture();
}

void UBillboardComponent::UnregisterComponent() {
    UPrimitiveComponent::UnregisterComponent();
    mTexture = nullptr;
}
