/**
 * @file FEditorCommands.h
 * @brief 레벨·프로젝트 전환과 저장·실행·내보내기·종료 명령을 조정합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "FMessageBox.h"
#include "Editor/FEditorContext.h"
#include "Editor/FPlayInEditorSession.h"
#include "FEditorViewportLayout.h"
#include "Platform/Window.h"
#include <filesystem>
#include <functional>

namespace Editor {

    /**
     * @brief 레벨·프로젝트 전환과 저장·실행·내보내기·종료 명령을 조정합니다.
     */
    class FEditorCommands {

    public:
        /**
         * @brief FEditorCommands의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Context 편집 대상과 공유 서비스를 제공하는 에디터 컨텍스트의 비소유 주소입니다.
         * @param[in] PlaySession PIE 실행과 입력 상태를 제공하는 세션입니다.
         * @param[in] ViewportLayout 뷰포트 분할과 입력을 관리하는 서비스입니다.
         * @param[in] Platform 창·입력·파일 대화상자를 제공하는 플랫폼 애플리케이션입니다.
         */
        FEditorCommands(Editor::FEditorContext* Context, Editor::FPlayInEditorSession* PlaySession, Editor::FEditorViewportLayout& ViewportLayout, Platform::FWindowsApplication* Platform, Editor::FMessageBox& Messages);

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FEditorCommands();

    public:
        /**
         * @brief 명령 실행에 따른 콘텐츠 변경을 알릴 콜백을 지정합니다.
         * @param[in] Handler 콘텐츠 변경을 알릴 콜백입니다.
         */
        void SetContentChanged(std::function<void()> Handler);

        /**
         * @brief 열린 에셋 편집기의 저장 확인 항목을 수집할 콜백을 지정합니다.
         * @param[in] Handler 저장 확인 항목을 수집할 콜백입니다.
         */
        void SetCollectAssetSavePrompts(std::function<Core::TArray<Editor::FMessageBox::FSavePrompt>()> Handler);

        /**
         * @brief 에디터 종료가 요청되었는지 검사합니다.
         * @return 종료 요청이 확정되었으면 true입니다.
         */
        bool WantsExit() const;

        /**
         * @brief 열린 에셋과 레벨의 변경 확인을 거쳐 에디터 종료를 요청합니다.
         */
        void RequestExit();

        /**
         * @brief 레벨 열기 또는 저장 파일 선택 대화상자를 표시합니다.
         * @param[in] Save 저장 대화상자를 사용할지 지정합니다.
         * @return 사용자가 선택한 경로이며 취소하면 빈 경로입니다.
         */
        std::filesystem::path ChooseFile(bool Save);

        /**
         * @brief 현재 레벨을 저장하거나 새 경로를 선택해 저장합니다.
         * @param[in] SaveAs 새 저장 경로를 선택할지 지정합니다.
         */
        void SaveLevel(bool SaveAs);

        /**
         * @brief 현재 레벨을 저장하고 프로젝트의 시작 레벨로 지정합니다.
         * @note 저장된 레벨이 없거나 저장에 실패하면 아무것도 바꾸지 않습니다. 시작 레벨이 바뀌면 프로젝트 썸네일을 다시 촬영합니다.
         */
        void SetCurrentLevelAsStartup();

        /**
         * @brief 변경 처리 확인 뒤 파일을 선택해 레벨을 엽니다.
         */
        void OpenLevel();
        void OpenLevel(const std::filesystem::path& Path);
        void NewLevel();

        /**
         * @brief PIE 실행을 요청합니다.
         */
        void RequestPlay();

        /**
         * @brief 출력 경로를 선택해 프로젝트의 실행 가능한 게임을 내보냅니다.
         */
        void ExportGame();

        /**
         * @brief 프로젝트 생성 또는 열기 선택을 처리합니다.
         * @param[in] Create 새 프로젝트 생성 여부입니다.
         */
        void ChooseProject(bool Create);

        /**
         * @brief 현재 레벨이 시작 레벨이면 첫 뷰포트의 원근 카메라로 장면을 렌더링해 프로젝트 썸네일 PNG로 저장합니다.
         * @note 결과는 프로젝트의 Saved/ProjectThumbnail.png에 원자적으로 기록하며 성공 여부를 로그에 남깁니다.
         */
        void CaptureProjectThumbnail();

    private:
        /**
         * @brief 레벨 전환 전에 저장하지 않은 변경의 처리 여부를 확인합니다.
         * @param[in] Action 변경 확인이 끝나면 실행할 작업입니다.
         */
        void ConfirmSceneChange(std::function<void()> Action);

        void ExportGameTo(const std::filesystem::path& Destination);

    private:
        /**
         * @brief 편집 대상과 공유 서비스에 접근하는 에디터 컨텍스트의 비소유 주소입니다.
         */
        Editor::FEditorContext* mContext{};

        /**
         * @brief PIE의 월드·재생·입력 상태를 관리하는 공유 세션입니다.
         */
        Editor::FPlayInEditorSession* mPlaySession{};

        /**
         * @brief 뷰포트 분할·입력·최대화 배치를 관리하는 서비스입니다.
         */
        Editor::FEditorViewportLayout& mViewportLayout;

        /**
         * @brief Win32 창·입력·파일 선택을 제공하는 플랫폼 애플리케이션입니다.
         */
        Platform::FWindowsApplication* mPlatform{};

        Editor::FMessageBox& mMessages;

        /**
         * @brief 변경 확인을 마친 에디터 종료 요청 여부입니다.
         */
        bool mExit{};

        /**
         * @brief 명령에 따른 콘텐츠 변경을 알리는 콜백입니다.
         */
        std::function<void()> mContentChanged{};

        /**
         * @brief 열린 에셋 편집기의 저장 확인 항목을 수집하는 콜백입니다.
         */
        std::function<Core::TArray<Editor::FMessageBox::FSavePrompt>()> mCollectAssetSavePrompts{};
    };

}
