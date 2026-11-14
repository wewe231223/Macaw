/**
 * @file FBlendSpaceEditor.h
 * @brief 블렌드 스페이스 에셋의 축과 샘플을 편집하고 가중치를 미리 보는 창입니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Editor/FAssetPicker.h"

#include <array>
#include <filesystem>
#include <memory>

namespace Asset {
    class FAssetManager;
    class UBlendSpace;
}

namespace Editor {

    class FAssetThumbnailCache;
    class FEditorAssetCatalog;

    /**
     * @brief 블렌드 스페이스 하나를 편집 사본으로 열어 축 범위와 샘플(시퀀스·위치)을 편집하고 저장합니다.
     * @details 저장하면 캐시된 원본 객체에 사본 내용을 적용한 뒤 파일로 씁니다. 플레이 중인 인스턴스도 같은 원본을 쓰므로 저장 즉시 반영됩니다.
     *
     * 언리얼 블렌드 스페이스 편집기처럼 가운데 축 캔버스에서 직접 편집합니다.
     * - 샘플(마름모)을 끌어 위치를 바꾸며, 격자 맞춤이 켜져 있으면 격자 간격에 맞춥니다. Ctrl을 누르고 끌면 맞추지 않습니다.
     * - 빈 곳을 클릭하거나 끌면 미리보기 입력(초록 선)이 움직이고, 섞이는 샘플 위에 가중치 막대가 올라옵니다.
     * - 콘텐츠 브라우저의 시퀀스를 캔버스에 놓거나 빈 곳을 우클릭하면 그 위치에 샘플을 추가합니다. 샘플을 우클릭하면 지울 수 있습니다.
     */
    class FBlendSpaceEditor {
    public:

        /**
         * @brief 에셋 관리자와 선택기 서비스를 연결합니다.
         * @param[in] Assets 프로젝트 에셋 관리자이며 창보다 오래 살아 있어야 합니다.
         * @param[in] Catalog 시퀀스 선택기에 쓸 목록입니다.
         * @param[in] Thumbnails 시퀀스 선택기에 쓸 썸네일 캐시입니다.
         */
        FBlendSpaceEditor(Asset::FAssetManager* Assets, Editor::FEditorAssetCatalog* Catalog, Editor::FAssetThumbnailCache* Thumbnails);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FBlendSpaceEditor();

    public:

        /**
         * @brief 블렌드 스페이스를 열어 편집 사본을 만듭니다. 저장하지 않은 변경은 버립니다.
         * @param[in] Path Content 기준 경로입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 열었으면 true입니다.
         */
        bool Open(const std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief 열려 있으면 창을 그립니다.
         */
        void Draw();

        /**
         * @brief 열린 에셋이 지정한 경로나 그 아래에 있으면 닫습니다. 콘텐츠 브라우저에서 지우거나 이름을 바꿀 때 호출합니다.
         * @param[in] Path 절대 경로나 Content 기준 경로입니다.
         */
        void CloseRetired(const std::filesystem::path& Path);

        /**
         * @brief 창이 열려 있는지 반환합니다.
         * @return 열려 있으면 true입니다.
         */
        bool IsOpen() const;

        /**
         * @brief 에디터 창 또는 그 하위 창이 현재 입력 포커스를 갖는지 확인합니다.
         * @return 해당 에디터에 포커스가 있으면 true입니다.
         */
        bool IsFocused() const;

    private:

        /**
         * @brief 블렌드 공간 편집과 샘플 선택 상태를 정리하고 창을 닫습니다.
         */
        void Close();

        /**
         * @brief 편집 사본을 원본에 적용하고 파일로 저장합니다.
         * @return 저장했으면 true입니다.
         */
        bool Save();

        /**
         * @brief 축 캔버스를 그리고 샘플 끌기·미리보기·추가·삭제 입력을 처리합니다.
         */
        void DrawCanvas();

        /**
         * @brief 블렌드 샘플의 표시·이름·선택 관련 메뉴를 표시합니다.
         */
        void DrawSampleMenus();

        /**
         * @brief 축 이름·범위·격자 설정을 그립니다.
         */
        void DrawAxis();

        /**
         * @brief 샘플 목록을 그립니다. 선택한 샘플을 강조합니다.
         */
        void DrawSamples();

        /**
         * @brief 시퀀스를 로드해 지정한 위치에 샘플로 추가하고 선택합니다.
         * @param[in] AnimationPath Content 기준 시퀀스 경로입니다.
         * @param[in] Value 축 위치입니다.
         */
        void AddSampleAt(const Core::FString& AnimationPath, float Value);

        /**
         * @brief 격자 맞춤이 켜져 있으면 값을 가장 가까운 격자 위치로 옮깁니다.
         * @param[in] Value 축 위치입니다.
         * @param[in] Free 맞추지 않으려면 true입니다.
         * @return 축 범위로 제한한 값입니다.
         */
        float Snap(float Value, bool Free) const;

    private:

        /**
         * @brief 프로젝트 에셋 관리자입니다.
         */
        Asset::FAssetManager* mAssets{};

        /**
         * @brief 시퀀스 선택기에 쓸 목록입니다.
         */
        Editor::FEditorAssetCatalog* mCatalog{};

        /**
         * @brief 시퀀스 선택기에 쓸 썸네일 캐시입니다.
         */
        Editor::FAssetThumbnailCache* mThumbnails{};

        /**
         * @brief 시퀀스 선택기입니다.
         */
        Editor::FAssetPicker mPicker{};

        /**
         * @brief 편집 사본입니다.
         */
        std::unique_ptr<Asset::UBlendSpace> mEditing{};

        /**
         * @brief Content 기준 경로입니다.
         */
        std::filesystem::path mPath{};

        /**
         * @brief 저장하지 않은 변경이 있는지 여부입니다.
         */
        bool mDirty{};

        /**
         * @brief 창이 열려 있는지 여부입니다.
         */
        bool mOpen{};

        /**
         * @brief 미리보기 입력 값입니다.
         */
        float mPreviewInput{};

        /**
         * @brief 선택한 샘플 순서이며 없으면 -1입니다.
         */
        Core::Index_t mSelectedSample{-1};

        /**
         * @brief 끌고 있는 샘플 순서이며 없으면 -1입니다.
         */
        Core::Index_t mDraggingSample{-1};

        /**
         * @brief 미리보기 입력을 끌고 있는지 여부입니다.
         */
        bool mDraggingPreview{};

        /**
         * @brief 샘플을 격자에 맞출지 여부입니다.
         */
        bool mSnapToGrid{true};

        /**
         * @brief 샘플 지점에 이름을 표시할지 여부입니다.
         */
        bool mShowSampleNames{true};

        /**
         * @brief 선택한 샘플이 목록에 보이도록 스크롤할지 여부입니다.
         */
        bool mRevealSample{};

        /**
         * @brief 우클릭 메뉴로 샘플을 추가할 축 위치입니다.
         */
        float mAddValue{};

        /**
         * @brief 우클릭 샘플 추가 메뉴의 검색어입니다.
         */
        std::array<char, 64> mAddSearch{};

        /**
         * @brief 마지막 저장·검증 오류입니다.
         */
        Core::FString mError{};
    };

}
