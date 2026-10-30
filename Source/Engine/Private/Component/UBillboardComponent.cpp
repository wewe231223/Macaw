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
#include "Engine/UTexture2D.h"
#include "Core/Archive.h"
#include "Core/FilePath.h"
#include "Core/Base/ErrorHandler.h"
#include "CoreUObject/Reflection/Cast.h"

#include <algorithm>
#include <cmath>
#include <cwctype>

namespace Engine {

    Engine::UBillboardComponent::UBillboardComponent() = default;
    Engine::UBillboardComponent::~UBillboardComponent() = default;

    Engine::UTexture* Engine::UBillboardComponent::GetSprite() const {
        return mSprite;
    }

    bool Engine::UBillboardComponent::SetSprite(Engine::UTexture* Sprite) {
        if (Sprite == nullptr) {
            return SetTexturePath({});
        }

        const auto* Texture{CoreUObject::Cast<Engine::UTexture2D>(Sprite)};
        Engine::FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};

        if (Texture == nullptr || !Texture->IsValid() || Assets == nullptr) {
            return false;
        }

        const Core::FString Path{Assets->GetTexturePath(Texture)};

        return !Path.empty() && SetTexturePath(Path);
    }

    const Core::FString& Engine::UBillboardComponent::GetTexturePath() const {
        return mTexturePath;
    }

    bool Engine::UBillboardComponent::SetTexturePath(const Core::FString& Path) {
        const std::filesystem::path AssetPath{Core::FilePath::FromUtf8(Path)};
        std::wstring Extension{AssetPath.extension().wstring()};
        std::transform(Extension.begin(), Extension.end(), Extension.begin(), [](wchar_t Character) {
            return static_cast<wchar_t>(std::towlower(Character));
        });

        const bool Supported{Extension == L".masset" || Extension == L".png" || Extension == L".jpg" || Extension == L".jpeg" || Extension == L".bmp" || Extension == L".tga" || Extension == L".dds" || Extension == L".hdr"};

        if (Path.find('\0') != Core::FString::npos || (!Path.empty() && (AssetPath.has_root_path() || !Supported))) {
            return false;
        }

        for (const auto& Part : AssetPath) {
            if (Part == L"..") {
                return false;
            }
        }

        Engine::UTexture2D* Texture{LoadTexture(Path)};

        if (!Path.empty() && GetWorld() != nullptr && GetWorld()->GetAssets() != nullptr && Texture == nullptr) {
            return false;
        }

        mTexturePath = Core::FilePath::ToGenericUtf8(AssetPath.lexically_normal());
        mSprite = Texture;
        MarkRenderStateDirty();

        return true;
    }

    Engine::UTexture2D* Engine::UBillboardComponent::LoadTexture(const Core::FString& Path) const {
        Engine::FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};

        if (Assets == nullptr || Path.empty()) {
            return nullptr;
        }

        Core::FString Error{};
        Engine::UTexture2D* Texture{Assets->LoadTexture(Core::FilePath::FromUtf8(Path), Error)};

        if (Texture == nullptr) {
            Core::ErrorHandler::Report("Sprite", "Cannot load texture '" + Path + "': " + Error, Core::ErrorHandler::EErrorLevel::Warning);
        }

        return Texture;
    }

    bool Engine::UBillboardComponent::ResolveTexture() {
        mSprite = LoadTexture(mTexturePath);
        MarkRenderStateDirty();
        return mTexturePath.empty() || mSprite != nullptr;
    }

    Engine::UTexture2D* Engine::UBillboardComponent::GetTexture() const {
        return CoreUObject::Cast<Engine::UTexture2D>(mSprite);
    }

    const Core::FVector2& Engine::UBillboardComponent::GetSize() const {
        return mSize;
    }

    bool Engine::UBillboardComponent::SetSize(const Core::FVector2& Size) {
        if (!std::isfinite(Size.x) || !std::isfinite(Size.y) || Size.x <= 0.0f || Size.y <= 0.0f || Size.x > 10000.0f || Size.y > 10000.0f) {
            return false;
        }

        mSize = Size;
        MarkRenderStateDirty();

        return true;
    }

    const Core::FVector4& Engine::UBillboardComponent::GetColor() const {
        return mColor;
    }

    bool Engine::UBillboardComponent::SetColor(const Core::FVector4& Color) {
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

    bool Engine::UBillboardComponent::IsScreenAligned() const {
        return mScreenAligned;
    }

    void Engine::UBillboardComponent::SetScreenAligned(bool ScreenAligned) {
        mScreenAligned = ScreenAligned;
        MarkRenderStateDirty();
    }

    bool Engine::UBillboardComponent::IsVisible() const {
        return mVisible;
    }

    void Engine::UBillboardComponent::SetVisible(bool Visible) {
        mVisible = Visible;
        MarkRenderStateDirty();
    }

    Engine::EPrimitiveType Engine::UBillboardComponent::GetPrimitiveType() const {
        return Engine::EPrimitiveType::Plane;
    }

    const Engine::FMeshData& Engine::UBillboardComponent::GetMeshData() const {
        static const Engine::FMeshData Empty{};
        return Empty;
    }

    Engine::FPrimitiveSceneProxy Engine::UBillboardComponent::CreateSceneProxy() const {
        Engine::FPrimitiveSceneProxy Proxy{Engine::UPrimitiveComponent::CreateSceneProxy()};
        Proxy.mBounds = GetWorldBounds();
        Proxy.mSprite = Engine::FDrawSprite{GetTexture(), mSize, mColor, mScreenAligned};
        return Proxy;
    }

    Core::FBox Engine::UBillboardComponent::GetWorldBounds() const {
        const Core::FMatrix World{GetWorldMatrix()};
        const Core::FVector Center{World.TransformPosition({})};
        const float Width{mSize.x * World.TransformVector({0.0f, 1.0f, 0.0f}).Length()};
        const float Height{mSize.y * World.TransformVector({0.0f, 0.0f, 1.0f}).Length()};
        const float Radius{0.5f * std::sqrt(Width * Width + Height * Height)};
        const Core::FVector Extent{Radius, Radius, Radius};

        return {Center - Extent, Center + Extent};
    }

    bool Engine::UBillboardComponent::ShouldCreateRenderState() const {
        return mVisible && mColor.w > 0.0f && (mTexturePath.empty() || mSprite != nullptr);
    }

    void Engine::UBillboardComponent::OnRegister() {
        ResolveTexture();
    }

    void Engine::UBillboardComponent::UnregisterComponent() {
        Engine::UPrimitiveComponent::UnregisterComponent();
        mSprite = nullptr;
    }

    void Engine::UBillboardComponent::Serialize(Core::FArchive& Archive) {
        Engine::UPrimitiveComponent::Serialize(Archive);

        if (Archive.IsLoading() && !Archive.HasError() && !Archive.HasField("Sprite") && Archive.HasField("Texture")) {
            Core::FString Path{};
            Archive.Serialize("Texture", Path);

            if (!Archive.HasError() && !SetTexturePath(Path)) {
                Archive.SetError("Invalid billboard texture path");
            }
        }
    }

}
