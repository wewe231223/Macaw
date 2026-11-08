/**
 * @file FShadowResources.h
 * @brief 광원별 그림자 맵의 아틀라스 할당·행렬과 셰이더·미리보기 자원을 관리합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include "FPassTarget.h"
#include "FShadowTileCache.h"
#include "FShadowAtlasAllocator.h"
#include "Engine/FLightSceneProxy.h"
#include "Engine/FSceneViewFamily.h"
#include "Asset/UTexture2D.h"
#include "Core/Math/FBox.h"
#include "RenderCore/FRenderStats.h"
#include "RenderCore/FShadowAtlasView.h"
#include "RenderCore/FShadowSettings.h"
#include "RenderCore/TConstantBuffer.h"
#include "RenderCore/TStructuredBuffer.h"

#include <span>

namespace Renderer {

    class FScene;

    /**
     * @brief 그림자 샘플링에 필요한 광원 행렬과 아틀라스·캐스케이드 정보입니다.
     * @ingroup Renderer
     */
    struct FGpuShadow {
    public:
        /**
         * @brief 월드 좌표를 그림자 클립 공간으로 변환하는 행렬입니다.
         */
        Core::FMatrix mViewProjection{};

        /**
         * @brief 보호 여백을 제외한 아틀라스 UV 시작점과 크기를 저장합니다.
         */
        Core::FVector4 mAtlasRectangle{};

        float mSharpen{};
        float mInverseAtlasSize{};
        float mPadding[2]{};

        /**
         * @brief 캐스케이드의 시작·끝 거리와 혼합 시작 거리, 예약값을 저장합니다.
         */
        Core::FVector4 mCascade{};

        /**
         * @brief PCSS 계산에 쓰는 광원 크기·타일 UV 배율·근평면·원평면 또는 깊이 범위를 저장합니다.
         * @details 근평면이 0이면 방향광이며 광원 크기는 각반지름의 탄젠트, UV 배율은 월드 길이당 타일 UV, 마지막 값은 깊이 범위입니다.
         *          근평면이 0보다 크면 원근 투영이며 광원 크기는 반지름, UV 배율은 단위 거리의 월드 길이당 타일 UV, 마지막 값은 원평면입니다.
         */
        Core::FVector4 mSoftness{};
    };

    /**
     * @brief 광원별 그림자 맵의 아틀라스 할당·행렬과 셰이더·미리보기 자원을 관리합니다.
     * @details 깊이 패스 그리기는 렌더러가 담당하며, 이 클래스는 타일별 타깃과 행렬만 제공합니다.
     * @ingroup Renderer
     */
    class FShadowResources {
    private:
        struct FTileDepth {
            Renderer::FShadowAtlasBlock mBlock{};
            Engine::EShadowBiasPreset mBiasPreset{Engine::DefaultShadowBiasPreset};
            Engine::EShadowBiasPreset mSlopeBiasPreset{Engine::DefaultShadowBiasPreset};
        };

    public:
        /**
         * @brief 현재 뷰에 필요한 광원별 그림자 타일과 행렬을 구성하고 샘플링 정보를 업로드합니다.
         * @details 깊이는 그리지 않으며, 렌더러가 GetMaps와 GetTarget으로 타일마다 깊이 패스를 그린 뒤 Finish를 호출합니다.
         * @param[in] Device GPU 자원 생성에 사용할 Direct3D 장치의 비소유 주소입니다.
         * @param[in] Context 명령을 기록할 Direct3D 11 장치 컨텍스트의 비소유 주소입니다.
         * @param[in] Scene 광원과 그림자 생성 프리미티브를 조회할 렌더 장면입니다.
         * @param[in] Family 장면·카메라·출력 타깃·표시 플래그를 묶은 현재 뷰 요청입니다.
         * @param[in] Settings 캐스케이드 수·캐시·광원 생략·필터를 결정하는 옵션입니다.
         * @param[in,out] Stats 그림자 패스의 할당 결과를 누적할 렌더 통계입니다.
         * @return 필요한 자원 생성과 업로드에 성공하면 true입니다.
         */
        bool Prepare(ID3D11Device* Device, ID3D11DeviceContext* Context, const Renderer::FScene& Scene, const Engine::FSceneViewFamily& Family, const RenderCore::FShadowSettings& Settings, RenderCore::FRenderStats& Stats);

        /**
         * @brief 깊이 패스가 끝난 아틀라스로 장면 패스의 그림자 샘플링을 활성화합니다.
         * @param[in] Context 명령을 기록할 Direct3D 11 장치 컨텍스트의 비소유 주소입니다.
         * @param[in] Family 캐스케이드 선택에 쓸 카메라를 가진 현재 뷰 요청입니다.
         */
        void Finish(ID3D11DeviceContext* Context, const Engine::FSceneViewFamily& Family);

        /**
         * @brief Prepare가 구성한 그림자 맵 목록을 반환합니다.
         * @return 그림자 맵별 광원 행렬과 아틀라스 영역의 비소유 구간입니다.
         */
        std::span<const Renderer::FGpuShadow> GetMaps() const;

        /**
         * @brief 그림자 맵 하나의 깊이를 기록할 아틀라스 타일 타깃을 반환합니다.
         * @param[in] Index GetMaps 기준 그림자 맵 인덱스입니다.
         * @return 아틀라스 깊이 뷰와 보호 여백을 제외한 타일 뷰포트·시저입니다.
         */
        Renderer::FPassTarget GetTarget(Core::Uint32 Index) const;

        /**
         * @brief 그림자 맵 하나의 타일을 지울 아틀라스 영역을 반환합니다.
         * @details 블록이 다른 맵에 재할당됐을 때 PCF가 이전 깊이를 읽지 않도록 보호 여백까지 포함합니다.
         * @param[in] Index GetMaps 기준 그림자 맵 인덱스입니다.
         * @return 아틀라스 깊이 뷰와 보호 여백을 포함한 블록 전체의 뷰포트·시저입니다.
         */
        Renderer::FPassTarget GetClearTarget(Core::Uint32 Index) const;

        /**
         * @brief 타일 캐시의 프레임을 진행하고 오래 쓰지 않은 항목을 제거합니다.
         */
        void BeginFrame();

        /**
         * @brief 그림자 맵 하나의 깊이 내용을 결정하는 현재 입력 스탬프를 만듭니다.
         * @param[in] Index GetMaps 기준 그림자 맵 인덱스입니다.
         * @param[in] DepthShaderRevision 깊이를 그릴 깊이 전용 정점 셰이더의 리비전입니다.
         * @return 블록·광원 행렬·캐스터·아틀라스·셰이더 리비전과 바이어스 프리셋을 묶은 스탬프입니다.
         */
        Renderer::FShadowTileStamp MakeTileStamp(Core::Uint32 Index, Core::Uint64 DepthShaderRevision) const;

        /**
         * @brief 아틀라스 타일이 이미 같은 입력으로 그려져 있는지 검사합니다.
         * @param[in] Index GetMaps 기준 그림자 맵 인덱스입니다.
         * @param[in] Stamp MakeTileStamp로 만든 현재 입력 스탬프입니다.
         * @return 캐시가 켜져 있고 다시 그릴 필요가 없으면 true입니다.
         */
        bool IsTileCurrent(Core::Uint32 Index, const Renderer::FShadowTileStamp& Stamp);

        /**
         * @brief 타일을 다시 그리는 데 성공했음을 기록하고, 같은 아틀라스 영역을 덮어쓰인 다른 항목을 무효화합니다.
         * @param[in] Index GetMaps 기준 그림자 맵 인덱스입니다.
         * @param[in] Stamp 타일을 그릴 때 사용한 입력 스탬프입니다.
         */
        void CommitTile(Core::Uint32 Index, const Renderer::FShadowTileStamp& Stamp);

        /**
         * @brief 그림자 아틀라스와 샘플링 정보의 셰이더 바인딩을 설정합니다.
         * @param[in] Context 명령을 기록할 Direct3D 11 장치 컨텍스트의 비소유 주소입니다.
         */
        void Bind(ID3D11DeviceContext* Context) const;

        /**
         * @brief 그림자 아틀라스와 샘플링 정보의 셰이더 바인딩을 해제합니다.
         * @param[in] Context 명령을 기록할 Direct3D 11 장치 컨텍스트의 비소유 주소입니다.
         */
        void Unbind(ID3D11DeviceContext* Context) const;

        /**
         * @brief 원본 광원별 그림자 맵 구간을 반환합니다.
         * @return 각 원소가 1 기반 시작 인덱스와 개수인 배열의 참조이며 시작값 0은 그림자 없음입니다.
         */
        const Core::TArray<std::array<Core::Uint32, 2>>& GetLightRanges() const;

        /**
         * @brief 현재 그림자 아틀라스의 미리보기와 타일 정보를 반환합니다.
         * @return GPU 뷰의 비소유 주소와 타일 정보 사본입니다.
         */
        RenderCore::FShadowAtlasView GetView();

        /**
         * @brief 요청된 그림자 아틀라스 미리보기를 현재 깊이 자원에서 갱신합니다.
         * @param[in] Context GPU 명령을 제출할 D3D11 장치 컨텍스트입니다.
         */
        void UpdatePreview(ID3D11DeviceContext* Context);

        /**
         * @brief 캐스케이드 구간과 그림자 생성 물체의 깊이를 포함하는 텍셀 정렬 방향광 행렬을 만듭니다.
         * @param[in] View 카메라 행렬·위치와 출력 크기를 가진 뷰입니다.
         * @param[in] Direction 광원이 비추는 월드 공간 방향입니다.
         * @param[in] NearDistance 캐스케이드가 포함할 카메라 기준 시작 거리입니다.
         * @param[in] FarDistance 캐스케이드가 포함할 카메라 기준 끝 거리입니다.
         * @param[in] Resolution 보호 여백을 제외한 유효 해상도이며 2보다 커야 합니다.
         * @param[in] CasterBounds 깊이 범위에 포함할 그림자 생성 물체의 월드 경계 상자 목록입니다.
         * @return 월드 공간을 해당 방향광 캐스케이드의 클립 공간으로 변환하는 행렬입니다.
         */
        static Core::FMatrix MakeDirectionalMatrix(const Engine::FSceneView& View, const Core::FVector& Direction, float NearDistance, float FarDistance, Core::Uint32 Resolution, std::span<const Core::FBox> CasterBounds);

    private:
        /**
         * @brief 그림자 아틀라스·미리보기·셰이더·버퍼·깊이 상태를 필요할 때 생성합니다.
         * @param[in] Device GPU 자원 생성에 사용할 Direct3D 장치의 비소유 주소입니다.
         * @return 모든 필수 GPU 자원을 사용할 수 있으면 true입니다.
         */
        bool CreateResources(ID3D11Device* Device);

        /**
         * @brief 광원 우선순위에 따라 그림자 타일과 캐스케이드·큐브 면 행렬을 구성합니다.
         * @param[in] Scene 광원과 그림자 생성 프리미티브를 조회할 렌더 장면입니다.
         * @param[in] Family 장면·카메라·출력 타깃·표시 플래그를 묶은 현재 뷰 요청입니다.
         * @param[in,out] Stats 그림자 패스의 드로·할당 결과를 누적할 렌더 통계입니다.
         */
        void Build(const Renderer::FScene& Scene, const Engine::FSceneViewFamily& Family, RenderCore::FRenderStats& Stats);

        /**
         * @brief 광원의 그림자 맵이 현재 뷰의 화면에 영향을 줄 수 있는지 검사합니다.
         * @param[in] Light 검사할 광원입니다.
         * @param[in] Receivers 카메라에 보이고 그림자를 받는 프리미티브의 월드 경계 상자 목록입니다.
         * @param[in] Casters 그림자를 만드는 프리미티브의 월드 경계 상자 목록입니다.
         * @return 유효한 수신 물체와 생성 물체가 모두 광원 범위에 있으면 true입니다.
         */
        static bool AffectsView(const Engine::FLightSceneProxy& Light, std::span<const Core::FBox> Receivers, std::span<const Core::FBox> Casters);

        /**
         * @brief 구성한 그림자 맵의 샘플링 정보를 GPU 구조화 버퍼에 업로드합니다.
         * @param[in] Device GPU 자원 생성에 사용할 Direct3D 장치의 비소유 주소입니다.
         * @param[in] Context 명령을 기록할 Direct3D 11 장치 컨텍스트의 비소유 주소입니다.
         * @return 필요한 버퍼 생성과 업로드에 성공하면 true입니다.
         */
        bool Upload(ID3D11Device* Device, ID3D11DeviceContext* Context);

        /**
         * @brief 그림자 맵 하나를 타일 캐시에서 구분하는 키를 만듭니다.
         * @param[in] Index GetMaps 기준 그림자 맵 인덱스입니다.
         * @return 장면 고유 번호·광원 GUID·면 번호를 묶은 키입니다.
         */
        Renderer::FShadowTileKey MakeTileKey(Core::Uint32 Index) const;

        /**
         * @brief 깊이 아틀라스를 색상 미리보기 텍스처로 변환합니다.
         * @param[in] Context 명령을 기록할 Direct3D 11 장치 컨텍스트의 비소유 주소입니다.
         */
        void RenderPreview(ID3D11DeviceContext* Context);

    private:
        /**
         * @brief 그림자 깊이를 저장하는 아틀라스 텍스처의 소유 객체입니다.
         */
        std::unique_ptr<Asset::UTexture2D> mAtlas{};

        /**
         * @brief 깊이 아틀라스를 색상으로 표시하는 미리보기 텍스처의 소유 객체입니다.
         */
        std::unique_ptr<Asset::UTexture2D> mPreview{};

        /**
         * @brief 그림자 맵 타일의 아틀라스 공간 할당기입니다.
         */
        Renderer::FShadowAtlasAllocator mAllocator{};

        /**
         * @brief 현재 뷰에서 렌더링할 GPU 그림자 맵 정보입니다.
         */
        Core::TArray<Renderer::FGpuShadow> mMaps{};

        /**
         * @brief mMaps와 같은 순서로 저장한 아틀라스 블록과 깊이 기록 설정입니다.
         */
        Core::TArray<FTileDepth> mTileDepths{};

        /**
         * @brief Prepare가 그림자 맵을 구성한 장면의 고유 번호입니다.
         */
        Core::FRevision mSceneIdentity{};

        /**
         * @brief Prepare가 그림자 맵을 구성할 때의 장면 캐스터 리비전입니다.
         */
        Core::FRevision mCasterRevision{};

        /**
         * @brief 아틀라스 텍스처를 만들 때마다 새로 받는 리비전입니다.
         */
        Core::FRevision mAtlasRevision{};

        /**
         * @brief 아틀라스 타일마다 마지막으로 그린 입력을 기억해 같은 입력이면 다시 그리지 않게 하는 캐시입니다.
         */
        Renderer::FShadowTileCache mTileCache{"Shadow tile", Core::FFrameAgeEviction{600}};

        /**
         * @brief 미리보기에 표시할 광원별 아틀라스 타일 목록입니다.
         */
        Core::TArray<RenderCore::FShadowAtlasTile> mTiles{};

        /**
         * @brief 원본 광원마다 그림자 맵의 1 기반 시작 인덱스와 개수를 저장합니다.
         */
        Core::TArray<std::array<Core::Uint32, 2>> mLightRanges{};

        /**
         * @brief 미리보기 전체 화면 삼각형의 정점 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11VertexShader> mPreviewVertexShader{};

        /**
         * @brief 깊이 값을 미리보기 명도로 변환하는 픽셀 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11PixelShader> mPreviewPixelShader{};

        /**
         * @brief 모든 그림자 맵의 샘플링 정보를 담는 구조화 버퍼입니다.
         */
        RenderCore::TStructuredBuffer<Renderer::FGpuShadow> mShadowBuffer{};

        /**
         * @brief 그림자 캐스케이드 선택에 필요한 카메라와 필터 옵션의 상수 버퍼입니다.
         */
        RenderCore::TConstantBuffer<std::array<Core::FVector4, 3>> mViewBuffer{};

        /**
         * @brief 현재 뷰에 적용 중인 그림자 렌더링 옵션입니다.
         */
        RenderCore::FShadowSettings mSettings{};

        /**
         * @brief 그림자 깊이를 비교 샘플링하는 샘플러입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11SamplerState> mSampler{};

        /**
         * @brief 깊이 검사를 끈 미리보기 패스의 상태입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mPreviewDepthState{};

        /**
         * @brief 현재 뷰에서 유효한 그림자 맵을 셰이더에 제공할 수 있는지 나타냅니다.
         */
        bool mEnabled{};

        /**
         * @brief 현재 프레임의 그림자 미리보기 요청 상태입니다.
         */
        bool mPreviewRequested{};

        /**
         * @brief 그림자 미리보기 재생성 필요 여부입니다.
         */
        bool mPreviewDirty{true};
    };

}
