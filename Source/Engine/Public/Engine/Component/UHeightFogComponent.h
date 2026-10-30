/**
 * @file UHeightFogComponent.h
 * @brief 밀도·높이 감쇠·거리·불투명도·산란 색상을 검증하고 렌더 장면의 안개 프록시를 동기화합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/USceneComponent.h"
#include "Engine/FHeightFogSceneProxy.h"

namespace Asset {
    class FAssetManager;
    class UTexture2D;
}

namespace Engine {

    /**
     * @brief 밀도·높이 감쇠·거리·불투명도·산란 색상을 검증하고 렌더 장면의 안개 프록시를 동기화합니다.
     */
    class UHeightFogComponent final : public Engine::USceneComponent {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(UHeightFogComponent, Engine::USceneComponent);

    public:
        /**
         * @brief 기본 설정으로 UHeightFogComponent 객체를 초기화합니다.
         */
        UHeightFogComponent();
        /**
         * @brief 소멸 전에 렌더 장면에서 안개를 등록 해제하고 아이콘 자원을 정리합니다.
         */
        ~UHeightFogComponent() override;

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectClass(UHeightFogComponent);

        /**
         * @brief 안개 기준 높이의 밀도를 반환합니다.
         * @return 거리당 안개 밀도이며 기본값은 0.02입니다.
         */
        float GetFogDensity() const;
        /**
         * @brief 0~10 범위의 유한한 안개 밀도를 설정하고 장면 프록시를 갱신합니다.
         * @param[in] Density 안개 기준 높이에 적용할 유한한 0~10 밀도입니다.
         * @return 유효한 값을 적용하면 true이며 범위를 벗어나면 기존 값을 유지하고 false입니다.
         */
        bool SetFogDensity(float Density);

        /**
         * @brief 높이에 따른 안개 밀도의 지수 감쇠 계수를 반환합니다.
         * @return 높이 감쇠 계수이며 0이면 높이에 따라 밀도가 변하지 않습니다.
         */
        float GetFogHeightFalloff() const;
        /**
         * @brief 0~100 범위의 유한한 높이 감쇠 계수를 적용하고 장면을 동기화합니다.
         * @param[in] Falloff 밀도의 높이 감쇠에 사용할 유한한 0~100 계수입니다.
         * @return 유효한 값을 적용하면 true이며 유효하지 않으면 false입니다.
         */
        bool SetFogHeightFalloff(float Falloff);

        /**
         * @brief 카메라에서 안개 적분을 시작하는 거리를 반환합니다.
         * @return 0 이상의 월드 거리입니다.
         */
        float GetStartDistance() const;
        /**
         * @brief 0 이상의 유한한 안개 시작 거리를 설정하고 장면 프록시를 갱신합니다.
         * @param[in] Distance 0 이상의 유한한 월드 거리입니다.
         * @return 값을 적용하면 true이며 음수 또는 비유한 값이면 false입니다.
         */
        bool SetStartDistance(float Distance);

        /**
         * @brief 안개를 적분할 최대 거리를 반환합니다.
         * @return 적분 종료 거리이며 0이면 종료 거리 제한이 없습니다.
         */
        float GetEndDistance() const;
        /**
         * @brief 안개 적분의 종료 거리를 설정하며 0이면 거리 상한을 해제합니다.
         * @param[in] Distance 0 이상의 유한한 월드 거리입니다.
         * @return 0 이상의 유한한 값을 적용하면 true이며 그렇지 않으면 false입니다.
         */
        bool SetEndDistance(float Distance);

        /**
         * @brief 먼 표면을 안개 적용에서 제외하는 경계 거리를 반환합니다.
         * @return 적용 제외 경계 거리이며 0이면 거리로 제외하지 않습니다.
         */
        float GetFogCutoffDistance() const;
        /**
         * @brief 이 거리보다 먼 표면의 안개를 생략하도록 설정하며 0이면 제한을 해제합니다.
         * @param[in] Distance 0 이상의 유한한 월드 거리입니다.
         * @return 0 이상의 유한한 값을 적용하면 true이며 그렇지 않으면 false입니다.
         */
        bool SetFogCutoffDistance(float Distance);

        /**
         * @brief 안개 불투명도의 상한을 반환합니다.
         * @return 0~1 범위의 최대 불투명도입니다.
         */
        float GetFogMaxOpacity() const;
        /**
         * @brief 0~1 범위의 유한한 최대 안개 불투명도를 적용하고 장면을 갱신합니다.
         * @param[in] Opacity 0~1 범위의 유한한 최대 불투명도입니다.
         * @return 유효한 값을 적용하면 true이며 그렇지 않으면 false입니다.
         */
        bool SetFogMaxOpacity(float Opacity);

        /**
         * @brief 기본 산란에 사용할 안개 색상을 반환합니다.
         * @return 컴포넌트가 보관하는 산란 RGBA 색상의 읽기 전용 참조입니다.
         */
        const Core::FVector4& GetFogInscatteringColor() const;
        /**
         * @brief 각 채널이 유한한 0~1 범위인 산란 색상을 적용하고 장면을 갱신합니다.
         * @param[in] Color 적용할 RGBA 색상입니다.
         * @return 모든 채널이 유효하면 true이며 하나라도 유효하지 않으면 false입니다.
         */
        bool SetFogInscatteringColor(const Core::FVector4& Color);

        /**
         * @brief 안개 효과의 컴포넌트 표시 설정을 조회합니다.
         * @return 효과를 사용하도록 설정했으면 true이며 에디터 아이콘 표시 여부와는 독립적입니다.
         */
        bool IsVisible() const;
        /**
         * @brief 안개 효과의 활성 여부를 설정하고 장면 프록시를 갱신합니다.
         * @param[in] Visible 안개 효과를 사용할지 나타냅니다.
         * @return 설정을 반영하면 항상 true입니다.
         */
        bool SetVisible(bool Visible);

        /**
         * @brief 유효한 공간 변환을 적용하고 안개 기준 높이와 아이콘 위치를 장면에 반영합니다.
         * @param[in] Transform 유효한 위치·정규화 회전·비특이 크기를 가진 루트 변환입니다.
         * @return 변환을 수락하면 true이며 유효하지 않으면 false입니다.
         */
        bool SetTransform(const Core::FTransform& Transform) override;
        /**
         * @brief 렌더 장면의 안개와 아이콘 참조를 정리한 뒤 기반 컴포넌트의 등록을 해제합니다.
         */
        void UnregisterComponent() override;
        /**
         * @brief 에셋 관리자 변경을 확인하여 아이콘을 확보하고 현재 안개 설정을 렌더 장면에 추가·갱신합니다.
         */
        void SynchronizeFog();
        /**
         * @brief 현재 GUID·변환·안개 설정·아이콘을 렌더 장면에 복사할 값으로 구성합니다.
         * @return 컴포넌트의 현재 상태를 가진 안개 프록시 값입니다.
         */
        Engine::FHeightFogSceneProxy CreateFogProxy() const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();
        /**
         * @brief 월드에 등록된 시점에 아이콘과 안개 프록시를 동기화합니다.
         */
        void OnRegister() override;
        /**
         * @brief 프로젝트의 언리얼 안개 아이콘을 찾고 없으면 실행 파일 또는 저장소의 배포 템플릿에서 로드합니다.
         */
        void ResolveIcon();

    private:
        /**
         * @brief 기준 높이의 안개 밀도이며 유한한 0~10 범위를 허용합니다.
         */
        float mFogDensity{0.02f};
        ReflectAccessor(mFogDensity, GetFogDensity, SetFogDensity, "FogDensity", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 높이에 따른 지수 밀도 감쇠 계수이며 0~100 범위를 허용합니다.
         */
        float mFogHeightFalloff{0.2f};
        ReflectAccessor(mFogHeightFalloff, GetFogHeightFalloff, SetFogHeightFalloff, "FogHeightFalloff", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 카메라부터 안개 적분을 시작하는 월드 거리입니다.
         */
        float mStartDistance{};
        ReflectAccessor(mStartDistance, GetStartDistance, SetStartDistance, "StartDistance", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 안개 적분을 종료하는 월드 거리이며 0이면 거리 제한을 두지 않습니다.
         */
        float mEndDistance{1000.0f};
        ReflectAccessor(mEndDistance, GetEndDistance, SetEndDistance, "EndDistance", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 이 거리보다 먼 물체의 안개를 생략하며 0이면 거리 제한이 없습니다.
         */
        float mFogCutoffDistance{};
        ReflectAccessor(mFogCutoffDistance, GetFogCutoffDistance, SetFogCutoffDistance, "FogCutoffDistance", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 최종 안개 불투명도의 0~1 상한입니다.
         */
        float mFogMaxOpacity{1.0f};
        ReflectAccessor(mFogMaxOpacity, GetFogMaxOpacity, SetFogMaxOpacity, "FogMaxOpacity", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 장면 색상과 혼합할 기본 산란 색상이며 각 RGBA 채널은 0~1 범위입니다.
         */
        Core::FVector4 mFogInscatteringColor{0.65f, 0.75f, 0.85f, 1.0f};
        ReflectAccessor(mFogInscatteringColor, GetFogInscatteringColor, SetFogInscatteringColor, "FogInscatteringColor", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 안개 효과를 렌더 장면에서 사용할지 나타내며 에디터 아이콘 표시와 독립적입니다.
         */
        bool mVisible{true};
        ReflectAccessor(mVisible, IsVisible, SetVisible, "Visible", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 프로젝트 에셋 또는 대체 텍스처가 소유하는 언리얼 안개 아이콘의 비소유 주소입니다.
         */
        Asset::UTexture2D* mIcon{};
        ReflectProperty(mIcon, "Icon", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 현재 아이콘을 조회한 에셋 관리자의 비소유 주소이며 관리자가 바뀌면 아이콘을 다시 찾습니다.
         */
        Asset::FAssetManager* mIconAssets{};
        ReflectProperty(mIconAssets, "IconAssets", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 프로젝트에서 아이콘을 찾지 못했을 때 배포 템플릿에서 로드하는 소유 텍스처입니다.
         */
        std::unique_ptr<Asset::UTexture2D> mFallbackIcon{};
        ReflectProperty(mFallbackIcon, "FallbackIcon", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
