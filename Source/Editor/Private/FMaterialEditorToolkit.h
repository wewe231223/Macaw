/**
 * @file FMaterialEditorToolkit.h
 * @brief 머티리얼 속성·미리 보기·저장과 변경 확인을 제공하는 에셋 툴킷입니다.
 * @ingroup Editor
 */

#pragma once

#include "FAssetEditorToolkit.h"
#include "CoreUObject/TObjectRef.h"
#include "Editor/FAssetPicker.h"
#include "Engine/EPrimitiveType.h"
#include "Viewer/FAssetViewer.h"
#include "Asset/UMaterial.h"
#include "Editor/FAssetThumbnailCache.h"
#include "Editor/FEditorAssetCatalog.h"
#include "Editor/FEditorContext.h"

namespace Editor {

    /**
     * @brief 머티리얼 속성·미리 보기·저장과 변경 확인을 제공하는 에셋 툴킷입니다.
     */
    class FMaterialEditorToolkit : public Editor::FAssetEditorToolkit {

    public:
        /**
         * @brief FMaterialEditorToolkit의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Context 편집 대상과 공유 서비스를 제공하는 에디터 컨텍스트의 비소유 주소입니다.
         * @param[in] Path 대상 에셋 또는 콘텐츠 경로입니다.
         * @param[in] Material 연결할 머티리얼의 비소유 주소입니다.
         * @param[in] Catalog 에셋 검색과 메타데이터를 제공하는 카탈로그입니다.
         * @param[in] Thumbnails 에셋 썸네일과 애니메이션 미리 보기 캐시입니다.
         * @param[in] Messages 저장 확인을 표시하는 서비스입니다.
         */
        FMaterialEditorToolkit(Editor::FEditorContext* Context, const std::filesystem::path& Path, Asset::UMaterial* Material, Editor::FEditorAssetCatalog& Catalog, Editor::FAssetThumbnailCache& Thumbnails, Editor::FMessageBox& Messages);

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FMaterialEditorToolkit() override;

    public:
        /**
         * @brief 저장하지 않은 머티리얼을 확인할 항목을 반환합니다.
         */
        std::optional<Editor::FMessageBox::FSavePrompt> GetClosePrompt() override;

    private:
        /**
         * @brief 편집 중인 머티리얼을 저장하고 미리 보기와 변경 상태를 갱신합니다.
         * @return 저장에 성공하면 true입니다.
         */
        bool SaveMaterial();

        /**
         * @brief 저장하지 않은 머티리얼 변경의 저장·폐기·취소를 확인합니다.
         * @return 변경 처리를 마쳐 계속할 수 있으면 true입니다.
         */
        bool RequestTabClose();

        bool ReloadMaterial();
        void RequestReload();

        /**
         * @brief 선택한 텍스처 경로를 머티리얼에 적용합니다.
         * @param[in] Path 대상 에셋 또는 콘텐츠 경로입니다.
         * @return 텍스처 변경에 성공하면 true입니다.
         */
        bool SetMaterialTexture(const Core::FString& Path);

        /**
         * @brief 선택한 셰이더 경로를 머티리얼에 적용합니다.
         * @param[in] Path 대상 에셋 또는 콘텐츠 경로입니다.
         * @return 셰이더 변경에 성공하면 true입니다.
         */
        bool SetMaterialShader(const Core::FString& Path);

        /**
         * @brief 에셋 편집기의 프레임별 상태를 갱신합니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         */
        void UpdateEditor(float DeltaTime) override;

        /**
         * @brief 편집기 포커스와 저장 가능 상태에 따라 단축키 명령을 처리합니다.
         */
        void HandleCommands() override;

        /**
         * @brief 재질을 표시하고 뷰포트의 카메라 입력을 처리합니다.
         */
        void DrawViewport();

        /**
         * @brief 머티리얼의 편집 가능한 속성과 에셋 선택 UI를 표시합니다.
         */
        void DrawDetails();

    private:
        /**
         * @brief 편집 대상과 공유 서비스에 접근하는 에디터 컨텍스트의 비소유 주소입니다.
         */
        Editor::FEditorContext* mContext{};

        /**
         * @brief 저장 확인을 표시하는 서비스입니다.
         */
        Editor::FMessageBox& mMessages;

        /**
         * @brief 현재 편집 중인 머티리얼의 비소유 주소입니다.
         */
        CoreUObject::TObjectRef<Asset::UMaterial> mEditingMaterial{};

        /**
         * @brief 머티리얼 편집·저장 오류를 표시할 문자열입니다.
         */
        Core::FString mMaterialError{};

        /**
         * @brief 머티리얼에 저장하지 않은 변경이 있는지 나타냅니다.
         */
        bool mMaterialDirty{};

        /**
         * @brief 에셋 검색과 선택에 사용하는 공유 카탈로그입니다.
         */
        Editor::FEditorAssetCatalog* mAssetCatalog{};

        /**
         * @brief 에셋 썸네일과 애니메이션 미리 보기 설정의 공유 캐시입니다.
         */
        Editor::FAssetThumbnailCache* mThumbnails{};

        /**
         * @brief 머티리얼의 텍스처·셰이더를 선택하는 에셋 선택기입니다.
         */
        Editor::FAssetPicker mAssetPicker{};

        Viewer::FAssetViewer mViewer;
        Engine::EPrimitiveType mPreviewPrimitive{Engine::EPrimitiveType::Sphere};
        Core::FString mViewportError{};
        bool mNeedsFrame{true};
    };

}
