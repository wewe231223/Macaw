/**
 * @file FTranslucencyMeshPass.h
 * @brief 반투명 메시·스프라이트·텍스트를 정렬하고 반투명 패스로 그립니다.
 * @ingroup Renderer
 */

#pragma once

#include "FMeshPass.h"
#include "FSpriteRenderer.h"
#include "FTextRenderer.h"

namespace Renderer {

    /**
     * @brief 반투명 메시·스프라이트·텍스트를 정렬하고 반투명 패스로 그립니다.
     */
    class FTranslucencyMeshPass final : public Renderer::FMeshPass {
    public:
        /**
         * @brief 반투명 패스가 참조하는 렌더러와 블렌드·깊이·래스터 상태입니다.
         */
        struct FRenderParameters {
        public:
            /**
             * @brief 스프라이트 그리기를 제공하는 렌더러의 비소유 주소입니다.
             */
            Renderer::FSpriteRenderer* mSpriteRenderer{};

            /**
             * @brief 텍스트 그리기를 제공하는 렌더러의 비소유 주소입니다.
             */
            Renderer::FTextRenderer* mTextRenderer{};

            /**
             * @brief 반투명 렌더링에 사용할 블렌드 상태의 비소유 주소입니다.
             */
            ID3D11BlendState* mBlend{};

            /**
             * @brief 깊이를 검사하되 쓰지 않는 렌더링 상태의 비소유 주소입니다.
             */
            ID3D11DepthStencilState* mDepthReadOnly{};

            /**
             * @brief 깊이 검사를 생략하는 렌더링 상태의 비소유 주소입니다.
             */
            ID3D11DepthStencilState* mDepthDisabled{};

            /**
             * @brief 일반 반투명 렌더링 래스터 상태의 비소유 주소입니다.
             */
            ID3D11RasterizerState* mRasterizer{};

            /**
             * @brief 와이어프레임 반투명 렌더링 상태의 비소유 주소입니다.
             */
            ID3D11RasterizerState* mWireframeRasterizer{};
        };

        using Renderer::FMeshPass::FMeshPass;

    public:
        Renderer::EMeshPass GetType() const override;

        /**
         * @brief 반투명 메시·스프라이트·텍스트를 패스 순서로 렌더링합니다.
         * @param[in] Family 뷰·렌더 대상과 표시 설정을 제공하는 뷰 패밀리입니다.
         * @param[in] EditorIcons 에디터 전용 아이콘도 그릴지 지정합니다.
         * @param[in] Parameters 반투명 패스가 참조할 렌더러와 GPU 상태입니다.
         * @return 실행한 그리기 호출 수입니다.
         */
        Uint32 Render(const Engine::FSceneViewFamily& Family, bool EditorIcons, const Renderer::FTranslucencyMeshPass::FRenderParameters& Parameters);

        /**
         * @brief 깊이 검사를 생략하는 오버레이 텍스트를 별도 순서로 그립니다.
         * @param[in] Family 뷰·렌더 대상과 표시 설정을 제공하는 뷰 패밀리입니다.
         * @param[in] Parameters 반투명 패스가 참조할 렌더러와 GPU 상태입니다.
         * @return 실행한 그리기 호출 수입니다.
         */
        Uint32 RenderOverlayTexts(const Engine::FSceneViewFamily& Family, const Renderer::FTranslucencyMeshPass::FRenderParameters& Parameters);

    private:
        bool BeginCommands(const Engine::FSceneViewFamily& Family) override;
        bool Includes(const Renderer::FScenePrimitive& Primitive, const Engine::FSceneViewFamily& Family) const override;

        void AddCommands(const Renderer::FScenePrimitive& Primitive, Asset::UStaticMesh& Mesh, const Engine::FSceneViewFamily& Family, Core::TArray<Renderer::FMeshDrawCommand>& Commands) override;
    };

}
