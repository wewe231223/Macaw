/**
 * @file UBillboardComponent.cpp
 * @brief 카메라 정렬과 고정 평면 텍스처 스프라이트를 제공합니다.
 * @ingroup Engine
 */

#include "Engine/Component/UBillboardComponent.h"
#include "Asset/FAssetManager.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Asset/FMeshData.h"
#include "Engine/UWorld.h"
#include "Asset/UTexture2D.h"
#include "Core/Archive.h"
#include "Core/FilePath.h"
#include "Core/Base/ErrorHandler.h"
#include "CoreUObject/Reflection/Cast.h"

#include <cmath>

namespace Engine {

    /**
     * @brief 전달된 설정과 기본값으로 UBillboardComponent 객체를 생성합니다.
     */
    Engine::UBillboardComponent::UBillboardComponent() = default;

    /**
     * @brief 기본 소멸자이며, 렌더 장면 등록 해제는 UPrimitiveComponent 소멸자가 처리합니다.
     */
    Engine::UBillboardComponent::~UBillboardComponent() = default;

    /**
     * @brief 빌보드에 할당된 Sprite 텍스처를 반환합니다.
     */
    Asset::UTexture* Engine::UBillboardComponent::GetSprite() const {
        return mSprite;
    }

    /**
     * @brief 현재 월드의 에셋 관리자가 보유한 2D 텍스처를 Sprite에 할당하고 저장할 경로를 갱신합니다.
     */
    bool Engine::UBillboardComponent::SetSprite(Asset::UTexture* Sprite) {
        if (Sprite == nullptr) {
            return SetTexturePath({});
        }

        const auto* Texture{CoreUObject::Cast<Asset::UTexture2D>(Sprite)};
        Asset::FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};

        if (Texture == nullptr || !Texture->IsValid() || Assets == nullptr) {
            return false;
        }

        const Core::FString Path{Assets->GetTexturePath(Texture)};

        return !Path.empty() && SetTexturePath(Path);
    }

    /**
     * @brief Sprite 텍스처의 Content 기준 에셋 경로를 반환합니다.
     */
    const Core::FString& Engine::UBillboardComponent::GetTexturePath() const {
        return mTexturePath;
    }

    /**
     * @brief 이미지 또는 .masset 텍스처의 Content 상대 경로를 검증하고 Sprite를 갱신합니다.
     */
    bool Engine::UBillboardComponent::SetTexturePath(const Core::FString& Path) {
        const std::filesystem::path AssetPath{Core::FilePath::FromUtf8(Path)};
        const std::wstring Extension{Core::FilePath::ToLowerExtension(AssetPath)};

        const bool Supported{Extension == L".masset" || Extension == L".png" || Extension == L".jpg" || Extension == L".jpeg" || Extension == L".bmp" || Extension == L".tga" || Extension == L".dds" || Extension == L".hdr"};

        if (Path.find('\0') != Core::FString::npos || (!Path.empty() && (AssetPath.has_root_path() || !Supported))) {
            return false;
        }

        for (const auto& Part : AssetPath) {
            if (Part == L"..") {
                return false;
            }
        }

        Asset::UTexture2D* Texture{LoadTexture(Path)};

        if (!Path.empty() && GetWorld() != nullptr && GetWorld()->GetAssets() != nullptr && Texture == nullptr) {
            return false;
        }

        mTexturePath = Core::FilePath::ToGenericUtf8(AssetPath.lexically_normal());
        mSprite = Texture;

        MarkRenderStateDirty();

        return true;
    }

    /**
     * @brief Content 상대 경로의 텍스처를 에셋 관리자에서 조회합니다.
     */
    Asset::UTexture2D* Engine::UBillboardComponent::LoadTexture(const Core::FString& Path) const {
        Asset::FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};

        if (Assets == nullptr || Path.empty()) {
            return nullptr;
        }

        Core::FString Error{};
        Asset::UTexture2D* Texture{Assets->LoadTexture(Core::FilePath::FromUtf8(Path), Error)};

        if (Texture == nullptr) {
            Core::ErrorHandler::Report("Sprite", "Cannot load texture '" + Path + "': " + Error, Core::ErrorHandler::EErrorLevel::Warning);
        }

        return Texture;
    }

    /**
     * @brief 저장된 텍스처 경로를 에셋 관리자에서 해석하고 렌더 상태를 갱신합니다.
     */
    bool Engine::UBillboardComponent::ResolveTexture() {
        mSprite = LoadTexture(mTexturePath);

        MarkRenderStateDirty();

        return mTexturePath.empty() || mSprite != nullptr;
    }

    /**
     * @brief 텍스처의 비소유 주소를 반환합니다.
     */
    Asset::UTexture2D* Engine::UBillboardComponent::GetTexture() const {
        return CoreUObject::Cast<Asset::UTexture2D>(mSprite);
    }

    /**
     * @brief 로컬 쿼드의 가로·세로 월드 크기를 반환합니다.
     */
    const Core::FVector2& Engine::UBillboardComponent::GetSize() const {
        return mSize;
    }

    /**
     * @brief 로컬 쿼드의 가로·세로 월드 크기를 설정합니다.
     */
    bool Engine::UBillboardComponent::SetSize(const Core::FVector2& Size) {
        if (!std::isfinite(Size.x) || !std::isfinite(Size.y) || Size.x <= 0.0f || Size.y <= 0.0f || Size.x > 10000.0f || Size.y > 10000.0f) {
            return false;
        }

        mSize = Size;

        MarkRenderStateDirty();

        return true;
    }

    /**
     * @brief 표시할 RGBA 색상을 반환합니다.
     */
    const Core::FVector4& Engine::UBillboardComponent::GetColor() const {
        return mColor;
    }

    /**
     * @brief 표시할 RGBA 색상을 설정합니다.
     */
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

    /**
     * @brief 카메라의 오른쪽·위쪽 축에 쿼드를 맞출지 여부인지 검사합니다.
     */
    bool Engine::UBillboardComponent::IsScreenAligned() const {
        return mScreenAligned;
    }

    /**
     * @brief 카메라의 오른쪽·위쪽 축에 쿼드를 맞출지 여부를 설정합니다.
     */
    void Engine::UBillboardComponent::SetScreenAligned(bool ScreenAligned) {
        mScreenAligned = ScreenAligned;

        MarkRenderStateDirty();
    }

    /**
     * @brief 빌보드 선택과 경계 계산에 사용하는 기본 도형 종류를 반환합니다.
     */
    Engine::EPrimitiveType Engine::UBillboardComponent::GetPrimitiveType() const {
        return Engine::EPrimitiveType::Plane;
    }

    /**
     * @brief 별도 스프라이트 렌더러를 사용하는 빌보드의 빈 메시 데이터를 반환합니다.
     */
    const Asset::FMeshData& Engine::UBillboardComponent::GetMeshData() const {
        static const Asset::FMeshData Empty{};

        return Empty;
    }

    /**
     * @brief Sprite 텍스처, 크기, 색상과 카메라 정렬 설정을 담은 렌더 프록시를 생성합니다.
     */
    Engine::FPrimitiveSceneProxy Engine::UBillboardComponent::CreateSceneProxy() const {
        Engine::FPrimitiveSceneProxy Proxy{Engine::UPrimitiveComponent::CreateSceneProxy()};
        Proxy.mBounds = GetWorldBounds();
        Proxy.mSprite = Engine::FDrawSprite{GetTexture(), mSize, mColor, mScreenAligned};

        return Proxy;
    }

    /**
     * @brief 카메라 정렬 방향에 관계없이 빌보드를 포함하는 월드 경계를 계산합니다.
     */
    Core::FBox Engine::UBillboardComponent::GetWorldBounds() const {
        const Core::FMatrix World{GetWorldMatrix()};
        const Core::FVector Center{World.TransformPosition({})};
        const float Width{mSize.x * World.TransformVector({0.0f, 1.0f, 0.0f}).Length()};
        const float Height{mSize.y * World.TransformVector({0.0f, 0.0f, 1.0f}).Length()};
        const float Radius{0.5f * std::sqrt(Width * Width + Height * Height)};
        const Core::FVector Extent{Radius, Radius, Radius};

        return {Center - Extent, Center + Extent};
    }

    /**
     * @brief 현재 표시·에셋 상태에서 렌더 프록시를 생성할지 판정합니다.
     */
    bool Engine::UBillboardComponent::ShouldCreateRenderState() const {
        return mColor.w > 0.0f && (mTexturePath.empty() || mSprite != nullptr);
    }

    /**
     * @brief 월드에 등록할 때 저장된 Sprite 경로를 해석하고 렌더 상태 갱신을 예약합니다.
     */
    void Engine::UBillboardComponent::OnRegister() {
        ResolveTexture();
    }

    /**
     * @brief 렌더 상태와 월드 등록을 해제하고 Sprite의 비소유 참조를 비웁니다.
     */
    void Engine::UBillboardComponent::UnregisterComponent() {
        Engine::UPrimitiveComponent::UnregisterComponent();

        mSprite = nullptr;
    }

    /**
     * @brief 빌보드 프로퍼티를 직렬화하고 이전 Texture 필드를 Sprite 경로로 복원합니다.
     */
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
