/**
 * @file FScene.h
 * @brief 프리미티브 프록시와 객체 상수 버퍼를 소유하는 Direct3D 11 렌더 장면입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FSceneInterface.h"
#include "Core/GuidHash.h"
#include "Core/FRevision.h"
#include "FScenePrimitive.h"
#include "Engine/FLightSceneProxy.h"
#include "Engine/FHeightFogSceneProxy.h"

#include <d3d11.h>
#include <wrl/client.h>

namespace Renderer {

    /**
     * @brief 프리미티브 프록시와 객체 상수 버퍼를 소유하는 Direct3D 11 렌더 장면입니다.
     * @ingroup Renderer
     */
    class FScene final : public Engine::FSceneInterface {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FScene, Engine::FSceneInterface);

    public:
        /**
         * @brief FScene의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in,out] Device 리소스 생성에 사용할 Direct3D 11 장치입니다.
         * @param[in,out] Context GPU 명령을 기록할 Direct3D 11 장치 컨텍스트입니다.
         */
        FScene(ID3D11Device* Device, ID3D11DeviceContext* Context);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FScene() override;

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FScene);

        /**
         * @brief 광원 프록시의 GUID로 렌더 장면에 광원을 추가하거나 교체합니다.
         * @param[in] Proxy 장면에 복사할 광원 상태입니다.
         */
        void AddLight(const Engine::FLightSceneProxy& Proxy) override;

        /**
         * @brief 지정한 GUID의 광원을 렌더 장면에서 제거합니다.
         * @param[in] Id 제거할 광원의 GUID입니다.
         */
        void RemoveLight(const Core::FGuid& Id) override;

        /**
         * @brief 장면에 등록된 광원 목록을 반환합니다.
         * @return 장면이 유지하는 광원 프록시 배열의 참조입니다.
         */
        const Core::TArray<Engine::FLightSceneProxy>& GetLights() const;

        /**
         * @brief 컴포넌트 GUID를 키로 높이 안개 프록시를 렌더 장면에 추가하거나 교체합니다.
         * @param[in] Proxy 장면에 복사할 높이 안개 프록시입니다.
         */
        void AddHeightFog(const Engine::FHeightFogSceneProxy& Proxy) override;
        /**
         * @brief 지정한 GUID의 높이 안개 프록시를 렌더 장면에서 제거합니다.
         * @param[in] Id 제거할 안개 컴포넌트의 GUID입니다.
         */
        void RemoveHeightFog(const Core::FGuid& Id) override;
        /**
         * @brief 현재 렌더 장면에 등록된 높이 안개 프록시 목록을 조회합니다.
         * @return 장면이 소유하는 안개 프록시 목록의 읽기 전용 참조입니다.
         */
        const Core::TArray<Engine::FHeightFogSceneProxy>& GetHeightFogs() const;

        /**
         * @copydoc Engine::FSceneInterface::AddPrimitive
         */
        void AddPrimitive(const Engine::FPrimitiveSceneProxy& Proxy) override;

        /**
         * @brief 지정한 프리미티브의 월드 행렬과 경계 상자를 갱신합니다.
         * @param[in] Id 장면 프리미티브의 식별자입니다.
         * @param[in] World 프리미티브의 새 월드 변환 행렬입니다.
         * @param[in] Bounds 프리미티브의 새 월드 경계 상자입니다.
         */
        void UpdatePrimitiveTransform(const Core::FGuid& Id, const Core::FMatrix& World, const Core::FBox& Bounds) override;

        /**
         * @brief 식별자에 해당하는 프리미티브와 관련 GPU 리소스를 제거합니다.
         * @param[in] Id 장면 프리미티브의 식별자입니다.
         */
        void RemovePrimitive(const Core::FGuid& Id) override;

        /**
         * @brief 현재 장면에 등록된 프리미티브 개수를 반환합니다.
         * @return 현재 장면에 등록된 프리미티브 개수입니다.
         */
        std::size_t GetPrimitiveCount() const override;

        /**
         * @brief 프리미티브 추가, 제거 또는 변환 갱신 시 증가하는 장면 개정 번호를 반환합니다.
         * @return 프리미티브 추가, 제거 또는 변환 갱신 시 증가하는 장면 개정 번호입니다.
         */
        Core::Uint64 GetRevision() const override;

        /**
         * @brief 장면이 소유한 렌더 프리미티브를 반환합니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        const Core::TArray<Renderer::FScenePrimitive>& GetPrimitives() const;

        /**
         * @brief 누적 변환 업로드 횟수를 반환하고 카운터를 초기화합니다.
         * @return 초기화하기 전까지 누적된 업로드 횟수입니다.
         */
        Core::Uint32 TakeTransformUpdates() const;

        /**
         * @brief 장면 인스턴스마다 생성 시 한 번 발급하는 고유 번호를 반환합니다.
         * @details 여러 장면이 공유하는 캐시에서 장면을 구분하는 키로 사용하며, 같은 주소에 다시 만든 장면과도 겹치지 않습니다.
         * @return 이 장면의 고유 번호입니다.
         */
        Core::FRevision GetIdentity() const;

        /**
         * @brief 그림자 깊이 결과에 영향을 주는 캐스터 상태의 리비전을 반환합니다.
         * @details 그림자를 드리우는 메시 프리미티브의 추가·제거·변환·프록시 교체와, 그 프리미티브가 쓰는 메시 데이터의 교체를 모두 반영합니다.
         * 모든 리비전이 전역에서 증가하므로 최댓값을 취하면 어떤 변경이든 값이 커집니다.
         * @return 캐스터 상태가 바뀔 때마다 커지는 리비전입니다.
         */
        Core::FRevision GetCasterRevision() const;

    private:
        /**
         * @brief 프리미티브의 객체 상수 버퍼를 현재 프록시 값으로 갱신합니다.
         * @param[in,out] Primitive 상수 버퍼를 갱신할 프리미티브입니다.
         */
        void Upload(Renderer::FScenePrimitive& Primitive);

        /**
         * @brief 프록시가 그림자 깊이 패스에 그려지는 캐스터인지 검사합니다.
         * @param[in] Proxy 검사할 프리미티브 프록시입니다.
         * @return 그림자를 드리우는 메시 프리미티브이면 true입니다.
         */
        static bool IsShadowCaster(const Engine::FPrimitiveSceneProxy& Proxy);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 등록된 광원 프록시 목록입니다.
         */
        Core::TArray<Engine::FLightSceneProxy> mLights{};
        ReflectProperty(mLights, "Lights", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 컴포넌트 GUID별로 추가·갱신되는 높이 안개 프록시 목록입니다.
         */
        Core::TArray<Engine::FHeightFogSceneProxy> mHeightFogs{};
        ReflectProperty(mHeightFogs, "HeightFogs", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 광원 GUID를 조밀한 광원 배열의 인덱스에 연결합니다.
         */
        Core::TMap<Core::FGuid, std::size_t, Core::FGuidHash> mLightIndices{};
        ReflectProperty(mLightIndices, "LightIndices", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 장면의 GPU 자원 생성에 사용할 Direct3D 장치의 비소유 주소입니다.
         */
        ID3D11Device* mDevice{};
        ReflectProperty(mDevice, "Device", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief Direct3D 11 즉시 컨텍스트입니다.
         */
        ID3D11DeviceContext* mContext{};
        ReflectProperty(mContext, "Context", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 장면이 소유한 렌더 프리미티브입니다.
         */
        Core::TArray<Renderer::FScenePrimitive> mPrimitives{};
        ReflectProperty(mPrimitives, "Primitives", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 프리미티브 GUID에서 밀집 배열 인덱스로의 조회 맵입니다.
         */
        Core::TMap<Core::FGuid, std::size_t, Core::FGuidHash> mIndices{};
        ReflectProperty(mIndices, "Indices", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 프리미티브 추가·제거 또는 변환 갱신 시 증가하는 장면 개정 번호입니다.
         */
        Core::Uint64 mRevision{};
        ReflectProperty(mRevision, "Revision", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 소비 전 누적된 객체 상수 버퍼 갱신 수입니다.
         */
        mutable Core::Uint32 mTransformUpdates{};
        ReflectProperty(mTransformUpdates, "TransformUpdates", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 장면 생성 시 발급한 고유 번호입니다.
         */
        Core::FRevision mIdentity{Core::FRevision::Next()};
        ReflectProperty(mIdentity, "Identity", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 그림자 캐스터 프리미티브가 추가·제거·변경될 때 새로 받는 리비전입니다.
         */
        Core::FRevision mCasterRevision{Core::FRevision::Next()};
        ReflectProperty(mCasterRevision, "CasterRevision", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
