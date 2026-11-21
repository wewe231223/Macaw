/**
 * @file FTextureEditorToolkit.h
 * @brief 텍스처 정보를 표시하고 이미지 미리 보기를 제공하는 에셋 툴킷입니다.
 * @ingroup Editor
 */

#pragma once

#include "FAssetEditorToolkit.h"
#include "CoreUObject/TObjectRef.h"
#include "Asset/UTexture2D.h"
#include "Editor/FEditorContext.h"
#include "Viewer/FAssetViewer.h"
#include <memory>

namespace Editor {

    /**
     * @brief 텍스처 정보를 표시하고 이미지 미리 보기를 제공하는 에셋 툴킷입니다.
     */
    class FTextureEditorToolkit : public Editor::FAssetEditorToolkit {

    public:
        /**
         * @brief FTextureEditorToolkit의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Context 편집 대상과 공유 서비스를 제공하는 에디터 컨텍스트의 비소유 주소입니다.
         * @param[in] Path 대상 에셋 또는 콘텐츠 경로입니다.
         * @param[in] Texture 미리 보기에 사용할 텍스처 또는 D3D11 텍스처 뷰입니다.
         */
        FTextureEditorToolkit(Editor::FEditorContext* Context, const std::filesystem::path& Path, Asset::UTexture2D* Texture);

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FTextureEditorToolkit() override;

    private:
        /**
         * @brief 텍스처 탭 이름을 갱신하고 텍스처나 GPU 리소스가 없으면 탭을 닫습니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         */
        void UpdateEditor(float DeltaTime) override;

        /**
         * @brief 에셋 또는 프로젝트의 미리 보기 이미지를 표시합니다.
         */
        void DrawPreview();

    private:
        /**
         * @brief 편집 대상과 공유 서비스에 접근하는 에디터 컨텍스트의 비소유 주소입니다.
         */
        Editor::FEditorContext* mContext{};

        /**
         * @brief 현재 미리 보는 텍스처의 비소유 주소입니다.
         */
        CoreUObject::TObjectRef<Asset::UTexture2D> mPreviewTexture{};

        /**
         * @brief 텍스처 이미지의 확대·이동·표시를 담당하는 뷰어입니다.
         */
        std::unique_ptr<Viewer::FAssetViewer> mTextureViewer{};
    };

}
