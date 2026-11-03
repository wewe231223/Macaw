/**
 * @file FAssetThumbnailCache.h
 * @brief 메시와 구형 재질 미리보기를 공유하는 썸네일 캐시입니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Guid.h"
#include "Asset/FAssetFile.h"
#include "Engine/FRenderTarget.h"

namespace Asset {
    class FAssetManager;
    class UStaticMesh;
    class UMaterial;
}

namespace Engine {
    class FSceneInterface;
}

namespace RenderCore {
    class IRendererModule;
}

namespace Editor {

    /**
     * @brief 렌더 타깃을 유지하며 보이는 에셋의 썸네일을 프레임 예산 안에서 갱신합니다.
     */
    class FAssetThumbnailCache {
    private:
        /**
         * @brief 캐시한 렌더 타깃과 마지막 사용·갱신 정보를 보관합니다.
         */
        struct FThumbnail {
        private:
            /**
             * @brief 캐시가 렌더 타깃을 갱신하도록 허용합니다.
             */
            friend class FAssetThumbnailCache;

        private:
            /**
             * @brief 썸네일 전용 렌더 타깃입니다.
             */
            std::unique_ptr<Engine::FRenderTarget> mTarget{};

            /**
             * @brief 마지막으로 렌더링한 에셋 설정의 서명입니다.
             */
            std::size_t mSignature{};

            /**
             * @brief 마지막 사용 프레임입니다.
             */
            Core::Uint64 mLastUsed{};
        };

    public:
        /**
         * @brief 에셋 관리자와 렌더러를 연결합니다.
         * @param Assets 프로젝트 에셋 관리자입니다.
         * @param Renderer 미리보기를 그릴 렌더러입니다.
         */
        FAssetThumbnailCache(Asset::FAssetManager* Assets, RenderCore::IRendererModule* Renderer);

        /**
         * @brief 장면과 썸네일 GPU 자원을 해제합니다.
         */
        ~FAssetThumbnailCache();

    public:
        /**
         * @brief UI 프레임 시작 시 생성 예산과 오래된 캐시를 갱신합니다.
         * @param RenderBudget 이번 프레임에 새로 그릴 최대 썸네일 수입니다.
         */
        void BeginFrame(Core::Uint32 RenderBudget = 4);

        /**
         * @brief 타깃 주소를 유지하면서 다음 조회에서 썸네일을 갱신하도록 표시합니다.
         */
        void Invalidate();

        /**
         * @brief 메시·재질·텍스처의 표시 자원을 조회합니다.
         * @param Path 실제 에셋 경로이며 빈 경로는 기본 메시·재질입니다.
         * @param Type 에셋 종류입니다.
         * @return UI에 표시할 SRV의 비소유 주소이며 준비 중이거나 실패하면 nullptr입니다.
         */
        void* GetThumbnail(const std::filesystem::path& Path, Asset::EAssetType Type);

        /**
         * @brief 편집 중인 재질을 구에 적용한 미리보기를 조회합니다.
         * @param Material 재질의 비소유 주소입니다.
         * @return 변경된 설정을 반영한 SRV의 비소유 주소입니다.
         */
        void* GetMaterialPreview(Asset::UMaterial* Material);

    private:
        /**
         * @brief 메시 또는 구형 재질을 캐시에 렌더링합니다.
         * @param Key 캐시 키입니다.
         * @param Mesh 렌더링할 메시입니다.
         * @param Material 전체 메시 재질이며 nullptr이면 슬롯 재질을 사용합니다.
         * @param Signature 에셋 설정의 서명입니다.
         * @return 캐시 렌더 타깃의 SRV 주소입니다.
         */
        void* Render(const Core::FString& Key, Asset::UStaticMesh* Mesh, Asset::UMaterial* Material, std::size_t Signature);

        /**
         * @brief 재질 설정의 변경을 감지할 서명을 만듭니다.
         * @param Material 검사할 재질입니다.
         * @return 셰이더·색상·텍스처·양면 설정의 서명입니다.
         */
        static std::size_t MaterialSignature(const Asset::UMaterial* Material);

    private:
        /**
         * @brief 프로젝트 에셋 관리자입니다.
         */
        Asset::FAssetManager* mAssets{};

        /**
         * @brief 미리보기 렌더러입니다.
         */
        RenderCore::IRendererModule* mRenderer{};

        /**
         * @brief 썸네일만 그리는 독립 장면입니다.
         */
        std::unique_ptr<Engine::FSceneInterface> mScene{};

        /**
         * @brief 재질 미리보기에 사용할 구 메시입니다.
         */
        std::unique_ptr<Asset::UStaticMesh> mSphere{};

        /**
         * @brief 빈 메시 선택기의 기본 큐브입니다.
         */
        std::unique_ptr<Asset::UStaticMesh> mCube{};

        /**
         * @brief 경로별 썸네일입니다.
         */
        Core::TMap<Core::FString, FThumbnail> mThumbnails{};

        /**
         * @brief 장면에서 재사용할 프리미티브 식별자입니다.
         */
        Core::FGuid mPrimitiveId{};

        /**
         * @brief 목록 갱신에 따른 캐시 세대입니다.
         */
        std::size_t mGeneration{1};

        /**
         * @brief 현재 UI 프레임입니다.
         */
        Core::Uint64 mFrame{};

        /**
         * @brief 남은 썸네일 생성 예산입니다.
         */
        Core::Uint32 mRenderBudget{4};
    };

}
