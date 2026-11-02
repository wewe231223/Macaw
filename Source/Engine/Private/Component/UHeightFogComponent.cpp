/**
 * @file UHeightFogComponent.cpp
 * @brief 밀도·높이 감쇠·거리·불투명도·산란 색상을 검증하고 렌더 장면의 안개 프록시를 동기화합니다.
 * @ingroup Engine
 */

#include "Engine/Component/UHeightFogComponent.h"
#include "Engine/UWorld.h"
#include "Engine/FSceneInterface.h"
#include "Asset/FAssetManager.h"
#include "Asset/UTexture2D.h"
#include "CoreUObject/Reflection/NewObject.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>

namespace Engine {

    Engine::UHeightFogComponent::UHeightFogComponent() = default;

    Engine::UHeightFogComponent::~UHeightFogComponent() {
        UnregisterComponent();
    }

    float Engine::UHeightFogComponent::GetFogDensity() const {
        return mFogDensity;
    }

    bool Engine::UHeightFogComponent::SetFogDensity(float Density) {
        if (!std::isfinite(Density) || Density < 0.0f || Density > 10.0f) {
            return false;
        }

        mFogDensity = Density;
        SynchronizeFog();

        return true;
    }

    float Engine::UHeightFogComponent::GetFogHeightFalloff() const {
        return mFogHeightFalloff;
    }

    bool Engine::UHeightFogComponent::SetFogHeightFalloff(float Falloff) {
        if (!std::isfinite(Falloff) || Falloff < 0.0f || Falloff > 100.0f) {
            return false;
        }

        mFogHeightFalloff = Falloff;
        SynchronizeFog();

        return true;
    }

    float Engine::UHeightFogComponent::GetStartDistance() const {
        return mStartDistance;
    }

    bool Engine::UHeightFogComponent::SetStartDistance(float Distance) {
        if (!std::isfinite(Distance) || Distance < 0.0f) {
            return false;
        }

        mStartDistance = Distance;
        SynchronizeFog();

        return true;
    }

    float Engine::UHeightFogComponent::GetEndDistance() const {
        return mEndDistance;
    }

    bool Engine::UHeightFogComponent::SetEndDistance(float Distance) {
        if (!std::isfinite(Distance) || Distance < 0.0f) {
            return false;
        }

        mEndDistance = Distance;
        SynchronizeFog();

        return true;
    }

    float Engine::UHeightFogComponent::GetFogCutoffDistance() const {
        return mFogCutoffDistance;
    }

    bool Engine::UHeightFogComponent::SetFogCutoffDistance(float Distance) {
        if (!std::isfinite(Distance) || Distance < 0.0f) {
            return false;
        }

        mFogCutoffDistance = Distance;
        SynchronizeFog();

        return true;
    }

    float Engine::UHeightFogComponent::GetFogMaxOpacity() const {
        return mFogMaxOpacity;
    }

    bool Engine::UHeightFogComponent::SetFogMaxOpacity(float Opacity) {
        if (!std::isfinite(Opacity) || Opacity < 0.0f || Opacity > 1.0f) {
            return false;
        }

        mFogMaxOpacity = Opacity;
        SynchronizeFog();

        return true;
    }

    const Core::FVector4& Engine::UHeightFogComponent::GetFogInscatteringColor() const {
        return mFogInscatteringColor;
    }

    bool Engine::UHeightFogComponent::SetFogInscatteringColor(const Core::FVector4& Color) {
        const auto Values{Color.ToArray()};

        if (!std::all_of(Values.begin(), Values.end(), [](float Value) {
            return std::isfinite(Value) && Value >= 0.0f && Value <= 1.0f;
        })) {
            return false;
        }

        mFogInscatteringColor = Color;
        SynchronizeFog();

        return true;
    }

    bool Engine::UHeightFogComponent::IsVisible() const {
        return mVisible;
    }

    bool Engine::UHeightFogComponent::SetVisible(bool Visible) {
        mVisible = Visible;
        SynchronizeFog();

        return true;
    }

    bool Engine::UHeightFogComponent::SetTransform(const Core::FTransform& Transform) {
        if (!Engine::USceneComponent::SetTransform(Transform)) {
            return false;
        }

        SynchronizeFog();

        return true;
    }

    void Engine::UHeightFogComponent::OnRegister() {
        SynchronizeFog();
    }

    void Engine::UHeightFogComponent::UnregisterComponent() {
        if (GetWorld() != nullptr && GetWorld()->GetScene() != nullptr) {
            GetWorld()->GetScene()->RemoveHeightFog(GetGuid());
        }

        mIcon = nullptr;
        mIconAssets = nullptr;
        mFallbackIcon.reset();
        Engine::UActorComponent::UnregisterComponent();
    }

    void Engine::UHeightFogComponent::SynchronizeFog() {
        Asset::FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};

        if (Assets != mIconAssets) {
            mIcon = nullptr;
            mFallbackIcon.reset();
            mIconAssets = Assets;
        }

        if (GetWorld() != nullptr && mIcon == nullptr) {
            ResolveIcon();
        }

        if (GetWorld() != nullptr && GetWorld()->GetScene() != nullptr) {
            GetWorld()->GetScene()->AddHeightFog(CreateFogProxy());
        }
    }

    Engine::FHeightFogSceneProxy Engine::UHeightFogComponent::CreateFogProxy() const {
        Engine::FHeightFogSceneProxy Proxy{};
        Proxy.mId = GetGuid();
        Proxy.mPosition = GetTransform().mLocation;
        Proxy.mDensity = mFogDensity;
        Proxy.mHeightFalloff = mFogHeightFalloff;
        Proxy.mStartDistance = mStartDistance;
        Proxy.mEndDistance = mEndDistance;
        Proxy.mCutoffDistance = mFogCutoffDistance;
        Proxy.mMaxOpacity = mFogMaxOpacity;
        Proxy.mInscatteringColor = mFogInscatteringColor;
        Proxy.mVisible = mVisible;
        Proxy.mIcon = mIcon;

        return Proxy;
    }

    void Engine::UHeightFogComponent::ResolveIcon() {
        if (mIconAssets != nullptr) {
            Core::FString Error{};
            mIcon = mIconAssets->LoadTexture("Editor/Icon/ExponentialHeightFog_64x.png", Error);
        }

        if (mIcon != nullptr) {
            return;
        }

        std::array<wchar_t, 32768> Executable{};
        const DWORD Length{GetModuleFileNameW(nullptr, Executable.data(), static_cast<DWORD>(Executable.size()))};
        std::filesystem::path Path{Length > 0 && Length < Executable.size() ? std::filesystem::path{Executable.data()}.parent_path() / "Templates/Project/Content/Editor/Icon/ExponentialHeightFog_64x.png" : std::filesystem::path{}};
        std::error_code Error{};

        if (Path.empty() || !std::filesystem::is_regular_file(Path, Error)) {
            Path = "Templates/Project/Content/Editor/Icon/ExponentialHeightFog_64x.png";
        }

        auto Icon{CoreUObject::NewObject<Asset::UTexture2D>()};

        if (Icon->LoadFromFile(Path, true)) {
            mFallbackIcon = std::move(Icon);
            mIcon = mFallbackIcon.get();
        }
    }

}
