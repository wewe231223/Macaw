/**
 * @file ULightComponentBase.h
 * @brief 광원의 색상·밝기·표시 상태와 렌더 장면 등록을 관리합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/USceneComponent.h"
#include "Engine/FLightSceneProxy.h"

class FPrimitiveDrawInterface;
class FAssetManager;

/**
 * @brief 광원의 색상·밝기·표시 상태와 렌더 장면 등록을 관리합니다.
 * @ingroup Engine
 */
class ULightComponentBase : public USceneComponent {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(ULightComponentBase, USceneComponent);

public:
    /**
     * @brief ULightComponentBase의 기본 상태를 초기화합니다.
     */
    ULightComponentBase();
    /**
     * @brief 광원을 렌더 장면에서 제거하고 아이콘 자원을 정리합니다.
     */
    ~ULightComponentBase() override;

public:
    /**
     * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
     * @return 등록된 클래스 메타데이터의 비소유 주소입니다.
     */
    ReflectClass(ULightComponentBase, EClassFlags::NoFactory);

    /**
     * @brief 광원의 밝기 배율을 반환합니다.
     * @return 광원의 밝기 배율입니다.
     */
    float GetIntensity() const;
    /**
     * @brief 광원의 밝기 배율을 검증하고 변경합니다.
     * @param[in] Intensity 0 이상인 유한한 밝기 배율입니다.
     * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
     */
    bool SetIntensity(float Intensity);
    /**
     * @brief 광원의 RGBA 색상을 반환합니다.
     * @return 컴포넌트가 소유한 RGBA 색상 값의 참조입니다.
     */
    const FVector4& GetLightColor() const;
    /**
     * @brief 광원의 RGBA 색상을 검증하고 변경합니다.
     * @param[in] Color 각 성분이 0 이상 1 이하인 유한한 RGBA 색상입니다.
     * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
     */
    bool SetLightColor(const FVector4& Color);
    /**
     * @brief 광원의 표시 여부를 반환합니다.
     * @return 광원을 표시하도록 설정했으면 true입니다.
     */
    bool IsVisible() const;
    /**
     * @brief 광원의 표시 여부를 변경하고 렌더 장면에 동기화합니다.
     * @param[in] Visible 광원을 표시하려면 true입니다.
     * @return 표시 설정을 반영한 뒤 항상 true를 반환합니다.
     */
    bool SetVisible(bool Visible);

    /**
     * @brief 현재 컴포넌트의 조명·그림자 설정으로 렌더 프록시를 생성합니다.
     * @return 렌더 장면에 복사할 광원 상태와 아이콘의 비소유 주소입니다.
     */
    virtual FLightSceneProxy CreateLightProxy() const;
    /**
     * @brief 아이콘 의존성을 갱신하고 현재 광원 프록시를 월드의 렌더 장면에 반영합니다.
     */
    void SynchronizeLight();
    /**
     * @brief 월드 변환을 적용하고 렌더 장면의 광원 위치·방향을 갱신합니다.
     * @param[in] Transform 광원에 적용할 월드 변환입니다.
     * @return 변환을 적용하면 true이며 유효하지 않은 변환이면 false입니다.
     */
    bool SetTransform(const FTransform& Transform) override;
    /**
     * @brief 렌더 장면의 광원 등록과 아이콘 참조를 해제하고 컴포넌트를 등록 해제합니다.
     */
    void UnregisterComponent() override;
    /**
     * @brief 광원의 방향과 선택된 광원의 반경·원뿔 보조선을 제출합니다.
     * @param[in] Draw 광원 보조선을 제출할 프리미티브 드로 인터페이스입니다.
     * @param[in] Selected 선택된 광원의 영향 범위를 그리려면 true입니다.
     */
    void DrawVisualization(FPrimitiveDrawInterface& Draw, bool Selected) const;

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();
    /**
     * @brief 월드에 등록된 광원의 상태를 렌더 장면과 동기화합니다.
     */
    void OnRegister() override;
    /**
     * @brief 광원 종류에 맞는 프로젝트 아이콘 또는 대체 아이콘을 로드합니다.
     * @param[in] Assets 아이콘 또는 재질·텍스처를 해석할 에셋 관리자의 비소유 주소입니다.
     */
    void ResolveIcon(FAssetManager* Assets);

private:
    /**
     * @brief 광원의 밝기 배율입니다.
     */
    float mIntensity{1.0f};
    ReflectAccessor(mIntensity, GetIntensity, SetIntensity, "Intensity", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    /**
     * @brief 광원의 RGBA 색상입니다.
     */
    FVector4 mLightColor{1.0f, 1.0f, 1.0f, 1.0f};
    ReflectAccessor(mLightColor, GetLightColor, SetLightColor, "LightColor", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    /**
     * @brief 광원의 표시 여부입니다.
     */
    bool mVisible{true};
    ReflectAccessor(mVisible, IsVisible, SetVisible, "Visible", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    /**
     * @brief 에디터에 표시할 광원 아이콘 텍스처의 비소유 주소입니다.
     */
    UTexture2D* mIcon{};
    ReflectProperty(mIcon, "Icon", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 현재 아이콘을 제공한 에셋 관리자의 비소유 주소입니다.
     */
    FAssetManager* mIconAssets{};
    ReflectProperty(mIconAssets, "IconAssets", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 프로젝트에서 아이콘을 찾지 못했을 때 직접 로드한 대체 텍스처의 소유 객체입니다.
     */
    std::unique_ptr<UTexture2D> mFallbackIcon{};
    ReflectProperty(mFallbackIcon, "FallbackIcon", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
