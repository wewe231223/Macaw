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

    /**
     * @brief 기본 설정으로 UHeightFogComponent 객체를 초기화합니다.
     */
    Engine::UHeightFogComponent::UHeightFogComponent() = default;

    /**
     * @brief 소멸 전에 렌더 장면에서 안개를 등록 해제하고 아이콘 자원을 정리합니다.
     */
    Engine::UHeightFogComponent::~UHeightFogComponent() {
        UnregisterComponent();
    }

    /**
     * @brief 안개 기준 높이의 밀도를 반환합니다.
     */
    float Engine::UHeightFogComponent::GetFogDensity() const {
        return mFogDensity;
    }

    /**
     * @brief 0~10 범위의 유한한 안개 밀도를 설정하고 장면 프록시를 갱신합니다.
     */
    bool Engine::UHeightFogComponent::SetFogDensity(float Density) {
        if (!std::isfinite(Density) || Density < 0.0f || Density > 10.0f) {
            return false;
        }

        mFogDensity = Density;

        SynchronizeFog();

        return true;
    }

    /**
     * @brief 높이에 따른 안개 밀도의 지수 감쇠 계수를 반환합니다.
     */
    float Engine::UHeightFogComponent::GetFogHeightFalloff() const {
        return mFogHeightFalloff;
    }

    /**
     * @brief 0~100 범위의 유한한 높이 감쇠 계수를 적용하고 장면을 동기화합니다.
     */
    bool Engine::UHeightFogComponent::SetFogHeightFalloff(float Falloff) {
        if (!std::isfinite(Falloff) || Falloff < 0.0f || Falloff > 100.0f) {
            return false;
        }

        mFogHeightFalloff = Falloff;

        SynchronizeFog();

        return true;
    }

    /**
     * @brief 카메라에서 안개 적분을 시작하는 거리를 반환합니다.
     */
    float Engine::UHeightFogComponent::GetStartDistance() const {
        return mStartDistance;
    }

    /**
     * @brief 0 이상의 유한한 안개 시작 거리를 설정하고 장면 프록시를 갱신합니다.
     */
    bool Engine::UHeightFogComponent::SetStartDistance(float Distance) {
        if (!std::isfinite(Distance) || Distance < 0.0f) {
            return false;
        }

        mStartDistance = Distance;

        SynchronizeFog();

        return true;
    }

    /**
     * @brief 안개를 적분할 최대 거리를 반환합니다.
     */
    float Engine::UHeightFogComponent::GetEndDistance() const {
        return mEndDistance;
    }

    /**
     * @brief 안개 적분의 종료 거리를 설정하며 0이면 거리 상한을 해제합니다.
     */
    bool Engine::UHeightFogComponent::SetEndDistance(float Distance) {
        if (!std::isfinite(Distance) || Distance < 0.0f) {
            return false;
        }

        mEndDistance = Distance;

        SynchronizeFog();

        return true;
    }

    /**
     * @brief 먼 표면을 안개 적용에서 제외하는 경계 거리를 반환합니다.
     */
    float Engine::UHeightFogComponent::GetFogCutoffDistance() const {
        return mFogCutoffDistance;
    }

    /**
     * @brief 이 거리보다 먼 표면의 안개를 생략하도록 설정하며 0이면 제한을 해제합니다.
     */
    bool Engine::UHeightFogComponent::SetFogCutoffDistance(float Distance) {
        if (!std::isfinite(Distance) || Distance < 0.0f) {
            return false;
        }

        mFogCutoffDistance = Distance;

        SynchronizeFog();

        return true;
    }

    /**
     * @brief 안개 불투명도의 상한을 반환합니다.
     */
    float Engine::UHeightFogComponent::GetFogMaxOpacity() const {
        return mFogMaxOpacity;
    }

    /**
     * @brief 0~1 범위의 유한한 최대 안개 불투명도를 적용하고 장면을 갱신합니다.
     */
    bool Engine::UHeightFogComponent::SetFogMaxOpacity(float Opacity) {
        if (!std::isfinite(Opacity) || Opacity < 0.0f || Opacity > 1.0f) {
            return false;
        }

        mFogMaxOpacity = Opacity;

        SynchronizeFog();

        return true;
    }

    /**
     * @brief 기본 산란에 사용할 안개 색상을 반환합니다.
     */
    const Core::FVector4& Engine::UHeightFogComponent::GetFogInscatteringColor() const {
        return mFogInscatteringColor;
    }

    /**
     * @brief 각 채널이 유한한 0~1 범위인 산란 색상을 적용하고 장면을 갱신합니다.
     */
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

    /**
     * @brief 소유 액터의 표시 여부 변경을 장면 프록시에 동기화합니다.
     */
    void Engine::UHeightFogComponent::OnOwnerVisibilityChanged() {
        SynchronizeFog();
    }

    /**
     * @brief 유효한 공간 변환을 적용하고 안개 기준 높이와 아이콘 위치를 장면에 반영합니다.
     */
    bool Engine::UHeightFogComponent::SetTransform(const Core::FTransform& Transform) {
        if (!Engine::USceneComponent::SetTransform(Transform)) {
            return false;
        }

        SynchronizeFog();

        return true;
    }

    /**
     * @brief 월드에 등록된 시점에 아이콘과 안개 프록시를 동기화합니다.
     */
    void Engine::UHeightFogComponent::OnRegister() {
        SynchronizeFog();
    }

    /**
     * @brief 렌더 장면의 안개와 아이콘 참조를 정리한 뒤 기반 컴포넌트의 등록을 해제합니다.
     */
    void Engine::UHeightFogComponent::UnregisterComponent() {
        if (GetWorld() != nullptr && GetWorld()->GetScene() != nullptr) {
            GetWorld()->GetScene()->RemoveHeightFog(GetGuid());
        }

        mIcon = nullptr;
        mIconAssets = nullptr;
        mFallbackIcon.reset();
        Engine::UActorComponent::UnregisterComponent();
    }

    /**
     * @brief 에셋 관리자 변경을 확인하여 아이콘을 확보하고 현재 안개 설정을 렌더 장면에 추가·갱신합니다.
     */
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

    /**
     * @brief 현재 GUID·변환·안개 설정·아이콘을 렌더 장면에 복사할 값으로 구성합니다.
     */
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
        Proxy.mVisible = IsOwnerVisible();
        Proxy.mIcon = mIcon;

        return Proxy;
    }

    /**
     * @brief 프로젝트의 언리얼 안개 아이콘을 찾고 없으면 실행 파일 또는 저장소의 배포 템플릿에서 로드합니다.
     */
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
