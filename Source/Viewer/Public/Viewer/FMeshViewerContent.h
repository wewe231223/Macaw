/**
 * @file FMeshViewerContent.h
 * @brief 메시·재질·스켈레톤의 미리보기 장면과 텍스처의 직접 표시를 제공합니다.
 * @ingroup Viewer
 */

#pragma once

#include "Viewer/IViewerContent.h"
#include "Viewer/FPreviewScene.h"
#include "Viewer/FBonePreview.h"
#include "Asset/FAssetFile.h"
#include "Core/Guid.h"
#include "Engine/FSkeletalMeshPose.h"

namespace CoreUObject {
    class UObject;
}

namespace Asset {
    class FAssetManager;
    class UStaticMesh;
    class USkeletalMesh;
    class USkeleton;
    class UMaterial;
}

namespace RenderCore {
    class IRendererModule;
}

namespace Viewer {

    /**
     * @brief 메시·재질·스켈레톤의 미리보기 장면과 텍스처의 직접 표시를 제공합니다.
     * @ingroup Viewer
     */
    class FMeshViewerContent final : public Viewer::IViewerContent {
    public:

        /**
         * @brief 렌더러·에셋 서비스와 빈 정적 에셋 미리보기 장면을 구성합니다.
         * @param Renderer 외부에서 소유하는 렌더러 서비스입니다.
         * @param Assets 외부에서 소유하는 에셋 관리자입니다.
         */
        FMeshViewerContent(RenderCore::IRendererModule* Renderer, Asset::FAssetManager* Assets);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FMeshViewerContent() override;

    public:

        /**
         * @brief 에셋 형식을 검증하고 GPU 리소스·미리보기 프록시 또는 2D 텍스처를 준비합니다.
         * @param Object 외부에서 수명을 보장하는 에셋 객체입니다.
         * @param Type Object의 실제 형식과 일치해야 하는 에셋 종류입니다.
         * @param Error 실패 원인을 기록할 출력 문자열입니다.
         * @return 에셋을 표시할 콘텐츠를 구성했으면 true이며 실패 원인은 Error에 기록합니다.
         */
        bool SetAsset(CoreUObject::UObject* Object, Asset::EAssetType Type, Core::FString& Error);
        

        /**
         * @brief 콘텐츠가 렌더링 또는 촬영 준비에 사용할 수 있는 상태인지 확인합니다.
         * @return 필요한 에셋과 표시 리소스가 준비되었으면 true입니다.
         */
        bool IsReady() const override;

        /**
         * @brief 현재 콘텐츠의 월드 공간 경계를 제공합니다.
         * @return 콘텐츠 수명에 종속되는 AABB의 읽기 전용 참조입니다.
         */
        const Core::FBox& GetBounds() const override;

        /**
         * @brief 렌더링에 사용할 독립 미리보기 장면을 반환합니다.
         * @return 콘텐츠 또는 월드가 소유하는 비소유 장면 포인터이며 없으면 nullptr입니다.
         */
        Engine::FSceneInterface* GetScene() const override;

        /**
         * @brief 장면 렌더링 대신 직접 표시할 2D 텍스처를 조회합니다.
         * @return 외부 소유의 비소유 포인터이며 직접 표시 콘텐츠가 아니면 nullptr입니다.
         */
        Asset::UTexture2D* GetTexture() const override;

        /**
         * @brief 변경된 콘텐츠 상태를 렌더 장면과 경계에 반영합니다.
         */
        void Synchronize() override;

        /**
         * @brief 콘텐츠의 본·관절 등 추가 표시 도형을 오버레이에 기록합니다.
         * @param Overlay 추가 도형을 기록할 오버레이입니다.
         * @param Radius 현재 스켈레톤 도형 생성에서는 사용하지 않는 표시 크기 인수입니다.
         */
        void BuildOverlay(Engine::FPrimitiveDrawInterface& Overlay, float Radius) const override;

        /**
         * @brief 본 도형을 그릴 에디터 메시 종류를 반환합니다.
         * @return 본 또는 관절 표시용 에디터 메시 종류입니다.
         */
        Asset::UStaticMesh* GetMesh() const;

        /**
         * @brief 현재 미리보기 스켈레탈 메시를 반환합니다.
         * @return 외부 소유의 비소유 포인터이며 해당 콘텐츠가 아니면 nullptr입니다.
         */
        Asset::USkeletalMesh* GetSkeletalMesh() const;

        /**
         * @brief 현재 미리보기 또는 컴포넌트가 참조하는 스켈레톤을 반환합니다.
         * @return 외부 소유의 비소유 포인터이며 미설정 시 nullptr입니다.
         */
        Asset::USkeleton* GetSkeleton() const;

        /**
         * @brief 스켈레탈 미리보기의 본 포즈를 제공합니다.
         * @return 콘텐츠가 소유하는 수정 가능한 포즈 참조입니다.
         */
        Engine::FSkeletalMeshPose& GetPose();

        /**
         * @brief 미리보기 메시 슬롯의 재질 포인터 목록을 제공합니다.
         * @return 콘텐츠가 보관하는 비소유 재질 포인터 배열의 수정 가능한 참조입니다.
         */
        Core::TArray<Asset::UMaterial*>& GetMaterials();

        /**
         * @brief 현재 스켈레탈 메시와 호환되는 스켈레톤으로 기준 포즈를 재구성합니다.
         * @param Skeleton 호출자가 수명을 보장하는 스켈레톤입니다.
         * @return 메시와 스켈레톤을 함께 사용할 수 있으면 true입니다.
         */
        bool SetSkeleton(Asset::USkeleton* Skeleton);

        /**
         * @brief 콘텐츠 변경을 표시하고 미리보기 프록시와 경계를 다시 구성합니다.
         */
        void Refresh();

    private:

        /**
         * @brief 외부에서 소유하는 렌더러 서비스입니다.
         */
        RenderCore::IRendererModule* mRenderer{};

        /**
         * @brief 외부에서 소유하는 에셋 관리자입니다.
         */
        Asset::FAssetManager* mAssets{};

        /**
         * @brief 정적 에셋의 렌더 장면을 소유하는 미리보기 월드입니다.
         */
        Viewer::FPreviewScene mScene;

        /**
         * @brief 재질 미리보기에 사용할 구 메시의 소유 포인터입니다.
         */
        std::unique_ptr<Asset::UStaticMesh> mPrimitiveMesh{};
        

        /**
         * @brief 외부에서 소유하는 정적 미리보기 메시입니다.
         */
        Asset::UStaticMesh* mMesh{};

        /**
         * @brief 에셋 관리자가 소유하는 미리보기 스켈레탈 메시입니다.
         */
        Asset::USkeletalMesh* mSkeletalMesh{};

        /**
         * @brief 외부에서 소유하는 미리보기 스켈레톤입니다.
         */
        Asset::USkeleton* mSkeleton{};

        /**
         * @brief 에셋 관리자가 소유하는 미리보기 재질입니다.
         */
        Asset::UMaterial* mMaterial{};

        /**
         * @brief 외부에서 소유하며 직접 2D로 표시할 텍스처입니다.
         */
        Asset::UTexture2D* mTexture{};
        

        /**
         * @brief 스켈레탈 미리보기의 본 변환과 스키닝 행렬입니다.
         */
        Engine::FSkeletalMeshPose mPose{};

        /**
         * @brief 메시 슬롯별 재질의 비소유 포인터 목록입니다.
         */
        Core::TArray<Asset::UMaterial*> mMaterials{};

        /**
         * @brief 스켈레톤 오버레이의 본·관절 도형 캐시입니다.
         */
        Core::TArray<Viewer::FBoneShape> mSkeletonShapes{};
        

        /**
         * @brief Fit과 컬링에 사용하는 미리보기 대상의 월드 공간 AABB입니다.
         */
        Core::FBox mBounds{};

        /**
         * @brief 미리보기 프리미티브를 장면에서 식별하는 GUID입니다.
         */
        Core::FGuid mPrimitiveId{};

        /**
         * @brief 저장 또는 장면 동기화가 필요한 변경이 있는지 여부입니다.
         */
        bool mDirty{true};
    };
}
