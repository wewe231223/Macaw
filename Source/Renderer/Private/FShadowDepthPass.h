/**
 * @file FShadowDepthPass.h
 * @brief 그림자를 드리우는 메시를 깊이 전용 파이프라인으로 아틀라스 타일에 기록하는 메시 패스입니다.
 * @ingroup Renderer
 */

#pragma once

#include "FMeshPass.h"
#include "Engine/EShadowBiasPreset.h"

#include <array>
#include <wrl/client.h>

namespace Renderer {

    class FMaterialPipeline;

    /**
     * @brief 그림자를 드리우는 메시를 깊이 전용 파이프라인으로 아틀라스 타일에 기록하는 메시 패스입니다.
     * @note 장치·렌더러 연결을 구성하는 FMeshPass 생성자를 상속합니다.
     */
    class FShadowDepthPass final : public Renderer::FMeshPass {
    public:
        using Renderer::FMeshPass::FMeshPass;

    public:

        /**
         * @brief 깊이·경사 바이어스 단계의 모든 조합에 대한 래스터라이저를 만듭니다.
         * @return 모든 GPU 상태를 생성했으면 true입니다.
         */
        bool Initialize();

        /**
         * @brief 깊이·경사 바이어스 조합에 대응하는 래스터라이저를 반환합니다.
         * @param DepthPreset 깊이 바이어스 프리셋 단계입니다.
         * @param SlopePreset 경사 바이어스 프리셋 단계입니다.
         * @return 패스가 소유하는 비소유 GPU 상태 포인터입니다.
         */
        ID3D11RasterizerState* GetRasterizer(Engine::EShadowBiasPreset DepthPreset, Engine::EShadowBiasPreset SlopePreset) const;

        /**
         * @brief 깊이 전용 입력 레이아웃을 쓰는 그림자 깊이 패스 종류를 반환합니다.
         * @return EMeshPass::ShadowDepth입니다.
         */
        Renderer::EMeshPass GetType() const override;

    protected:

        /**
         * @brief 모든 드로가 공유할 깊이 전용 파이프라인을 조회합니다.
         * @param[in] Family 현재 뷰 요청입니다.
         * @return 깊이 전용 파이프라인을 준비하면 true입니다.
         */
        bool BeginCommands(const Engine::FSceneViewFamily& Family) override;

        /**
         * @brief 그림자를 드리우는 프리미티브만 대상으로 합니다.
         * @param[in] Primitive 검사할 렌더 프리미티브입니다.
         * @param[in] Family 현재 뷰 요청입니다.
         * @return 그림자를 드리우면 true입니다.
         */
        bool Includes(const Renderer::FScenePrimitive& Primitive, const Engine::FSceneViewFamily& Family) const override;

        /**
         * @brief SubMesh마다 머티리얼 없이 깊이 전용 파이프라인 드로를 추가합니다.
         * @param[in] Primitive 월드 행렬을 가진 렌더 프리미티브입니다.
         * @param[in] Mesh GPU 자원이 준비된 정적 메시입니다.
         * @param[in] Family 현재 뷰 요청입니다.
         * @param[in,out] Commands 드로 명령을 추가할 목록입니다.
         */
        void AddCommands(const Renderer::FScenePrimitive& Primitive, Asset::UStaticMesh& Mesh, const Engine::FSceneViewFamily& Family, Core::TArray<Renderer::FMeshDrawCommand>& Commands) override;

    private:

        /**
         * @brief 이번 패스의 모든 드로가 공유하는 깊이 전용 파이프라인입니다.
         */
        Renderer::FMaterialPipeline* mPipeline{};

        /**
         * @brief 깊이·경사 바이어스 프리셋 조합별 래스터라이저 상태 캐시입니다.
         */
        std::array<std::array<Microsoft::WRL::ComPtr<ID3D11RasterizerState>, Engine::ShadowBiasPresetCount>, Engine::ShadowBiasPresetCount> mRasterizers{};
    };

}
