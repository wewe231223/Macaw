/**
 * @file FOpaqueMeshPass.h
 * @brief 보이는 메시의 표면과 깊이를 G-buffer에 기록하는 메시 패스입니다.
 * @ingroup Renderer
 */

#pragma once

#include "CoreUObject/TObjectRef.h"

#include "FMeshPass.h"

namespace Asset {
    class UMaterial;
}

namespace Renderer {

    class FMaterialPipeline;

    /**
     * @brief 보이는 메시의 표면과 깊이를 G-buffer에 기록하는 메시 패스입니다.
     * @note 장치·렌더러 연결을 구성하는 FMeshPass 생성자를 상속합니다.
     */
    class FOpaqueMeshPass final : public Renderer::FMeshPass {
    public:
        using Renderer::FMeshPass::FMeshPass;

    public:
        /**
         * @brief 표면과 깊이를 기록하는 기본 패스 종류를 반환합니다.
         * @return EMeshPass::OpaqueMeshPass입니다.
         */
        Renderer::EMeshPass GetType() const override;

    private:
        /**
         * @brief 머티리얼이 없거나 준비에 실패한 SubMesh가 쓸 기본 머티리얼과 파이프라인을 준비합니다.
         * @param[in] Family 파이프라인 변형을 고를 표시 모드를 가진 현재 뷰 요청입니다.
         * @return 기본 머티리얼과 파이프라인을 준비하면 true입니다.
         */
        bool BeginCommands(const Engine::FSceneViewFamily& Family) override;

        /**
         * @brief 프리미티브 표시 플래그가 켜져 있으면 모든 메시 프리미티브를 대상으로 합니다.
         * @param[in] Primitive 검사할 렌더 프리미티브입니다.
         * @param[in] Family 표시 플래그를 가진 현재 뷰 요청입니다.
         * @return 프리미티브 표시 플래그가 켜져 있으면 true입니다.
         */
        bool Includes(const Renderer::FScenePrimitive& Primitive, const Engine::FSceneViewFamily& Family) const override;

        /**
         * @brief SubMesh마다 덮어쓰기·슬롯·기본 머티리얼 순으로 머티리얼을 정해 해당 파이프라인 드로를 추가합니다.
         * @param[in] Primitive 월드 행렬과 머티리얼 슬롯을 가진 렌더 프리미티브입니다.
         * @param[in] Mesh GPU 자원이 준비된 정적 메시입니다.
         * @param[in] Family 파이프라인 변형을 고를 표시 모드를 가진 현재 뷰 요청입니다.
         * @param[in,out] Commands 드로 명령을 추가할 목록입니다.
         */
        void AddCommands(const Renderer::FScenePrimitive& Primitive, Asset::UStaticMesh& Mesh, const Engine::FSceneViewFamily& Family, Core::TArray<Renderer::FMeshDrawCommand>& Commands) override;

    private:
        /**
         * @brief 머티리얼이 없거나 준비에 실패한 SubMesh가 쓰는 기본 머티리얼입니다.
         */
        CoreUObject::TObjectRef<Asset::UMaterial> mDefaultMaterial{};
    };

}
