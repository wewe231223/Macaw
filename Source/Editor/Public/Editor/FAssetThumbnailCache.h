/**
 * @file FAssetThumbnailCache.h
 * @brief 공통 에셋 뷰어에서 촬영한 이미지를 캐시하고 변경된 에셋의 생성을 예약합니다.
 * @ingroup Editor
 */

#pragma once

#include "Viewer/FAssetViewer.h"
#include "Viewer/FViewerCapture.h"

#include <deque>

namespace Asset {
    class UMaterial;
    class UStaticMesh;
}

namespace Editor {

    /**
     * @brief 공통 에셋 뷰어에서 촬영한 이미지를 캐시하고 변경된 에셋의 생성을 예약합니다.
     * @ingroup Editor
     */
    class FAssetThumbnailCache {
    private:

        /**
         * @brief 촬영 이미지, 요청·실패 상태와 변경 감지 서명을 보관하는 캐시 항목입니다.
         * @ingroup Editor
         */
        struct FThumbnail {
        public:

            /**
             * @brief 촬영 이미지가 없는 초기 캐시 항목을 구성합니다.
             */
            FThumbnail();

            /**
             * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
             */
            ~FThumbnail();

        private:

            /**
             * @brief 내부 상태를 구성·직렬화하는 FAssetThumbnailCache에 접근을 허용합니다.
             */
            friend class FAssetThumbnailCache;

        private:

            /**
             * @brief 캐시 항목이 소유하는 최종 컬러 캡처입니다.
             */
            std::unique_ptr<Viewer::FViewerCapture> mCapture{};

            /**
             * @brief 현재 이미지에 반영된 에셋 변경 서명입니다.
             */
            std::size_t mSignature{};

            /**
             * @brief 대기 중인 촬영 요청의 에셋 변경 서명입니다.
             */
            std::size_t mPendingSignature{};

            /**
             * @brief 재시도를 보류할 실패 시점의 에셋 변경 서명입니다.
             */
            std::size_t mFailedSignature{};

            /**
             * @brief 마지막으로 이미지를 요청한 프레임 번호입니다.
             */
            Core::Uint64 mLastUsed{};

            /**
             * @brief 현재 이미지의 변경 서명이 유효한지 여부입니다.
             */
            bool mHasSignature{};

            /**
             * @brief 촬영 요청 큐에 이미 등록되어 있는지 여부입니다.
             */
            bool mQueued{};

            /**
             * @brief 최근 촬영 요청이 실패했는지 여부입니다.
             */
            bool mFailed{};
        };

        /**
         * @brief 촬영할 에셋의 비소유 포인터, 경로와 요청 시점 변경 서명입니다.
         * @ingroup Editor
         */
        struct FPendingThumbnail {
        public:

            /**
             * @brief 촬영 대상의 비소유 포인터와 요청 시점 경로·서명을 보관합니다.
             * @param Key 조회하거나 저장할 캐시 키입니다.
             * @param Path 처리할 에셋 또는 파일 경로입니다.
             * @param Object 외부에서 수명을 보장하는 에셋 객체입니다.
             * @param Type 처리할 에셋 또는 노드 종류입니다.
             * @param Signature 요청 시점 에셋의 변경 비교 서명입니다.
             */
            FPendingThumbnail(const Core::FString& Key, const std::filesystem::path& Path, CoreUObject::UObject* Object, Asset::EAssetType Type, std::size_t Signature);

        private:

            /**
             * @brief 내부 상태를 구성·직렬화하는 FAssetThumbnailCache에 접근을 허용합니다.
             */
            friend class FAssetThumbnailCache;

        private:

            /**
             * @brief 에셋 종류와 정규화 경로로 만든 썸네일 캐시 키입니다.
             */
            Core::FString mKey{};

            /**
             * @brief 현재 편집 중인 에셋 파일 경로입니다.
             */
            std::filesystem::path mPath{};

            /**
             * @brief 외부에서 소유하는 촬영 대상 UObject입니다.
             */
            CoreUObject::UObject* mObject{};

            /**
             * @brief 촬영할 객체의 에셋 종류입니다.
             */
            Asset::EAssetType mType{};

            /**
             * @brief 현재 이미지에 반영된 에셋 변경 서명입니다.
             */
            std::size_t mSignature{};
        };

    public:

        /**
         * @brief 에셋·렌더러 서비스와 촬영 뷰어를 연결하고 저장된 미리보기 설정을 읽습니다.
         * @param Assets 외부에서 소유하는 에셋 관리자입니다.
         * @param Renderer 외부에서 소유하는 렌더러 서비스입니다.
         */
        FAssetThumbnailCache(Asset::FAssetManager* Assets, RenderCore::IRendererModule* Renderer);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FAssetThumbnailCache();

    public:

        /**
         * @brief 이전 프레임의 임시 캡처를 정리하고 대기 중인 썸네일 요청을 처리합니다.
         */
        void BeginFrame();

        /**
         * @brief 캐시를 무효화하고 기존 이미지의 해제를 프레임 경계까지 보류합니다.
         */
        void Invalidate();

        /**
         * @brief 기존 썸네일을 반환하고 없거나 에셋이 바뀌었으면 촬영을 예약합니다.
         * @param Path 처리할 에셋 또는 파일 경로입니다.
         * @param Type 처리할 에셋 또는 노드 종류입니다.
         * @return 캐시 소유의 네이티브 텍스처 핸들이며 아직 없으면 nullptr입니다.
         */
        void* GetThumbnail(const std::filesystem::path& Path, Asset::EAssetType Type);

        /**
         * @brief 재질을 전용 뷰어로 렌더링하여 UI 표시용 텍스처를 반환합니다.
         * @param Material 외부에서 소유하는 미리보기 재질입니다.
         * @return 비소유 네이티브 텍스처 핸들이며 준비되지 않으면 nullptr입니다.
         */
        void* GetMaterialPreview(Asset::UMaterial* Material);

        /**
         * @brief 시퀀스의 미리보기 메시·스켈레톤 연결을 저장하고 관련 썸네일을 무효화합니다.
         * @param Path 처리할 에셋 또는 파일 경로입니다.
         * @param MeshPath 미리보기 메시의 에셋 경로입니다.
         * @param SkeletonPath 미리보기 스켈레톤의 에셋 경로입니다.
         * @param Error 실패 원인을 기록할 출력 문자열입니다.
         * @return 설정을 저장했으면 true이며 실패 원인은 Error에 기록합니다.
         */
        bool SetAnimationPreview(const std::filesystem::path& Path, const Core::FString& MeshPath, const Core::FString& SkeletonPath, Core::FString& Error);

        /**
         * @brief 시퀀스의 저장된 미리보기 메시·스켈레톤 연결을 조회합니다.
         * @param Path 처리할 에셋 또는 파일 경로입니다.
         * @param MeshPath 미리보기 메시의 에셋 경로입니다.
         * @param SkeletonPath 미리보기 스켈레톤의 에셋 경로입니다.
         * @return 저장된 연결을 찾았으면 true입니다.
         */
        bool GetAnimationPreview(const std::filesystem::path& Path, Core::FString& MeshPath, Core::FString& SkeletonPath) const;

    private:

        /**
         * @brief 정규화한 경로와 에셋 종류로 캐시 키를 만듭니다.
         * @param Path 처리할 에셋 또는 파일 경로입니다.
         * @param Type 처리할 에셋 또는 노드 종류입니다.
         * @return 같은 종류·경로에 대해 일관된 캐시 키입니다.
         */
        Core::FString MakeKey(const std::filesystem::path& Path, Asset::EAssetType Type) const;

        /**
         * @brief 재질 파라미터와 텍스처 변경을 감지할 서명을 계산합니다.
         * @param Material 외부에서 소유하는 미리보기 재질입니다.
         * @return 재질 미리보기 변경 비교용 해시입니다.
         */
        static std::size_t MaterialSignature(const Asset::UMaterial* Material);

        /**
         * @brief 에셋 종류에 맞는 관리자 로드 함수를 호출합니다.
         * @param Path 처리할 에셋 또는 파일 경로입니다.
         * @param Type 처리할 에셋 또는 노드 종류입니다.
         * @param Error 실패 원인을 기록할 출력 문자열입니다.
         * @return 에셋 관리자가 소유하는 객체이며 실패하면 nullptr입니다.
         */
        CoreUObject::UObject* LoadAsset(const std::filesystem::path& Path, Asset::EAssetType Type, Core::FString& Error);

        /**
         * @brief 에셋 종류별 렌더링 관련 변경을 감지할 서명을 계산합니다.
         * @param Object 외부에서 수명을 보장하는 에셋 객체입니다.
         * @param Type 처리할 에셋 또는 노드 종류입니다.
         * @return 현재 에셋 내용의 변경 비교용 해시입니다.
         */
        std::size_t AssetSignature(CoreUObject::UObject* Object, Asset::EAssetType Type);

        /**
         * @brief 대기 요청의 에셋과 미리보기 설정을 촬영 뷰어에 연결합니다.
         * @param Pending 현재 처리할 썸네일 요청입니다.
         * @param Error 실패 원인을 기록할 출력 문자열입니다.
         * @return 촬영 콘텐츠를 준비했으면 true이며 실패 원인은 Error에 기록합니다.
         */
        bool BindCaptureAsset(const FPendingThumbnail& Pending, Core::FString& Error);

        /**
         * @brief 요청 큐를 순서대로 준비·Fit·촬영하고 완료 이미지를 캐시에 반영합니다.
         */
        void ProcessRequests();

        /**
         * @brief 저장된 시퀀스별 미리보기 에셋 연결을 읽습니다.
         */
        void LoadPreviewSettings();

        /**
         * @brief 시퀀스별 미리보기 메시·스켈레톤 연결을 설정 파일에 저장합니다.
         * @return 파일 저장에 성공하면 true입니다.
         */
        bool SavePreviewSettings();

    private:

        /**
         * @brief 외부에서 소유하는 에셋 관리자입니다.
         */
        Asset::FAssetManager* mAssets{};

        /**
         * @brief 외부에서 소유하는 렌더러 서비스입니다.
         */
        RenderCore::IRendererModule* mRenderer{};

        /**
         * @brief 재질 미리보기를 위해 소유한 기본 메시입니다.
         */
        std::unique_ptr<Asset::UStaticMesh> mDefaultMesh{};

        /**
         * @brief 기본 미리보기를 위해 소유한 재질입니다.
         */
        std::unique_ptr<Asset::UMaterial> mDefaultMaterial{};

        /**
         * @brief 썸네일 요청을 순차적으로 촬영하는 공통 뷰어입니다.
         */
        Viewer::FAssetViewer mCaptureViewer;

        /**
         * @brief 재질 편집기의 실시간 미리보기용 공통 뷰어입니다.
         */
        Viewer::FAssetViewer mMaterialViewer;

        /**
         * @brief 에셋 키별 촬영 이미지와 요청 상태입니다.
         */
        Core::TMap<Core::FString, FThumbnail> mThumbnails{};

        /**
         * @brief 아직 촬영이 완료되지 않은 썸네일 요청 큐입니다.
         */
        std::deque<FPendingThumbnail> mRequests{};

        /**
         * @brief 현재 UI가 참조할 수 있어 프레임 경계까지 해제를 늦춘 이전 캡처입니다.
         */
        Core::TArray<std::unique_ptr<Viewer::FViewerCapture>> mRetiredCaptures{};

        /**
         * @brief 애니메이션 경로별 미리보기 메시·스켈레톤 경로입니다.
         */
        Core::TMap<Core::FString, std::pair<Core::FString, Core::FString>> mAnimationPreviews{};

        /**
         * @brief 전체 무효화 후 변경 서명을 구분하는 세대 번호입니다.
         */
        std::size_t mGeneration{1};

        /**
         * @brief 캐시 사용 시점을 기록하는 프레임 번호입니다.
         */
        Core::Uint64 mFrame{};

        /**
         * @brief 촬영 뷰어가 현재 요청의 준비 상태를 보관 중인지 여부입니다.
         */
        bool mWorkerActive{};
    };
}
