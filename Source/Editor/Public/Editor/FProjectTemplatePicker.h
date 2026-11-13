/**
 * @file FProjectTemplatePicker.h
 * @brief 프로젝트 템플릿 목록과 미리보기 이미지를 읽고 선택 UI를 제공합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"

#include <filesystem>
#include <memory>

namespace Asset {
    class UTexture2D;
}

namespace Editor {

    /**
     * @brief 프로젝트 템플릿 목록과 미리보기 이미지를 읽고 선택 UI를 제공합니다.
     * @ingroup Editor
     */
    class FProjectTemplatePicker {
    private:

        /**
         * @brief 템플릿 메타데이터, 원본 디렉터리와 소유한 미리보기 텍스처입니다.
         * @ingroup Editor
         */
        struct FTemplateInfo {

            /**
             * @brief 템플릿을 식별하는 고유 문자열입니다.
             */
            Core::FString mId{};

            /**
             * @brief 템플릿 선택 화면에 표시할 이름입니다.
             */
            Core::FString mName{};

            /**
             * @brief 템플릿 설명문입니다.
             */
            Core::FString mDescription{};

            /**
             * @brief 템플릿 원본 디렉터리입니다.
             */
            std::filesystem::path mDirectory{};

            /**
             * @brief 템플릿 미리보기 이미지 파일 경로입니다.
             */
            std::filesystem::path mPreviewPath{};

            /**
             * @brief 템플릿 미리보기 이미지의 소유 포인터입니다.
             */
            std::unique_ptr<Asset::UTexture2D> mPreview{};

            /**
             * @brief 템플릿 사용에 스크립트 프로젝트가 필요한지 여부입니다.
             */
            bool mRequiresScripts{};

            /**
             * @brief 미리보기 텍스처 로드를 이미 시도했는지 여부입니다.
             */
            bool mPreviewAttempted{};
        };

    public:

        /**
         * @brief 템플릿 목록과 선택이 없는 상태를 구성합니다.
         */
        FProjectTemplatePicker();

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FProjectTemplatePicker();

        /**
         * @brief 소유 리소스와 편집 상태의 복사 생성을 금지합니다.
         */
        FProjectTemplatePicker(const Editor::FProjectTemplatePicker&) = delete;

        /**
         * @brief 소유 리소스와 편집 상태의 복사 대입을 금지합니다.
         */
        Editor::FProjectTemplatePicker& operator=(const Editor::FProjectTemplatePicker&) = delete;

    public:

        /**
         * @brief 템플릿 루트의 메타데이터를 검색하고 지정한 디렉터리의 선택을 복원합니다.
         * @param Directory 검색하거나 로드할 템플릿 디렉터리입니다.
         * @param SelectedDirectory 초기 선택을 복원할 템플릿 디렉터리입니다.
         * @param Error 실패 원인을 기록할 출력 문자열입니다.
         * @return 템플릿 검색에 성공했으면 true이며 실패 원인은 Error에 기록합니다.
         */
        bool Load(const std::filesystem::path& Directory, const std::filesystem::path& SelectedDirectory, Core::FString& Error);

        /**
         * @brief 템플릿 ID로 목록의 선택 항목을 변경합니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @return 일치하는 템플릿을 찾았으면 true입니다.
         */
        bool SelectTemplate(const Core::FString& Id);

        /**
         * @brief 현재 선택한 템플릿의 디렉터리를 반환합니다.
         * @return 선택한 디렉터리의 참조이며 선택이 없으면 빈 경로입니다.
         */
        const std::filesystem::path& GetSelectedDirectory() const;

        /**
         * @brief 검색한 프로젝트 템플릿 수를 반환합니다.
         * @return 사용 가능한 템플릿 개수입니다.
         */
        Core::Uint32 GetTemplateCount() const;

        /**
         * @brief 템플릿 카드와 설명·미리보기를 표시하고 선택 입력을 처리합니다.
         * @param Device 리소스를 생성할 D3D11 장치입니다.
         * @return 선택한 템플릿이 변경되었으면 true입니다.
         */
        bool Draw(void* Device = nullptr);

    private:

        /**
         * @brief 한 템플릿의 메타데이터를 검증하여 목록에 추가합니다.
         * @param Directory 검색하거나 로드할 템플릿 디렉터리입니다.
         * @param Error 실패 원인을 기록할 출력 문자열입니다.
         * @return 유효한 템플릿을 추가했으면 true입니다.
         */
        bool LoadTemplate(const std::filesystem::path& Directory, Core::FString& Error);

        /**
         * @brief 아직 시도하지 않은 템플릿 미리보기 이미지를 GPU에 로드합니다.
         * @param Template 미리보기 이미지를 읽을 템플릿 항목입니다.
         * @param Device 리소스를 생성할 D3D11 장치입니다.
         */
        void LoadPreview(FTemplateInfo& Template, void* Device);

    private:

        /**
         * @brief 발견한 프로젝트 템플릿 목록입니다.
         */
        Core::TArray<FTemplateInfo> mTemplates{};

        /**
         * @brief 현재 선택한 템플릿의 목록 인덱스입니다.
         */
        std::size_t mSelectedIndex{};
    };

}
