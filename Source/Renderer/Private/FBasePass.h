/**
 * @file FBasePass.h
 * @brief 보이는 메시를 머티리얼 파이프라인과 조명으로 장면 타깃에 그리는 메시 패스입니다.
 * @ingroup Renderer
 */

#pragma once

#include "FMeshPass.h"

namespace Asset {
    class UMaterial;
}

namespace Renderer {

    class FMaterialPipeline;

    /**
     * @brief 보이는 메시를 머티리얼 파이프라인과 조명으로 장면 타깃에 그리는 메시 패스입니다.
     */
    class FBasePass final : public Renderer::FMeshPass {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(FBasePass, Renderer::FMeshPass);

    public:
        using Renderer::FMeshPass::FMeshPass;

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FBasePass);

        /**
         * @brief 머티리얼과 조명을 바인딩하는 기본 패스 종류를 반환합니다.
         * @return EMeshPass::BasePass입니다.
         */
        Renderer::EMeshPass GetType() const override;

    protected:
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
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 머티리얼이 없거나 준비에 실패한 SubMesh가 쓰는 기본 머티리얼입니다.
         */
        Asset::UMaterial* mDefaultMaterial{};
        ReflectProperty(mDefaultMaterial, "DefaultMaterial", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 머티리얼 파이프라인을 만들지 못한 SubMesh가 쓰는 기본 파이프라인입니다.
         */
        Renderer::FMaterialPipeline* mDefaultPipeline{};
        ReflectProperty(mDefaultPipeline, "DefaultPipeline", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
