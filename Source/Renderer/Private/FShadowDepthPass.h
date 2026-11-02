/**
 * @file FShadowDepthPass.h
 * @brief 그림자를 드리우는 메시를 깊이 전용 파이프라인으로 아틀라스 타일에 기록하는 메시 패스입니다.
 * @ingroup Renderer
 */

#pragma once

#include "FMeshPass.h"

namespace Renderer {

    class FMaterialPipeline;

    /**
     * @brief 그림자를 드리우는 메시를 깊이 전용 파이프라인으로 아틀라스 타일에 기록하는 메시 패스입니다.
     */
    class FShadowDepthPass final : public Renderer::FMeshPass {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(FShadowDepthPass, Renderer::FMeshPass);

    public:
        using Renderer::FMeshPass::FMeshPass;

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FShadowDepthPass);

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
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 이번 패스의 모든 드로가 공유하는 깊이 전용 파이프라인입니다.
         */
        Renderer::FMaterialPipeline* mPipeline{};
        ReflectProperty(mPipeline, "Pipeline", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
