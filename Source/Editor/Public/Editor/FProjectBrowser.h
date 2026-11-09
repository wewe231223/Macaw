/**
 * @file FProjectBrowser.h
 * @brief 프로젝트 열기·생성 및 최근 프로젝트 선택 UI를 처리합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "Editor/FImGuiUIScale.h"

#include <array>
#include <filesystem>
#include <vector>

namespace Platform {
    class FWindowsApplication;
}

namespace Renderer {
    class FRenderer;
}

namespace Editor {

    /**
     * @brief 프로젝트 열기·생성 및 최근 프로젝트 선택 UI를 처리합니다.
     */
    class FProjectBrowser {
    public:
        /**
         * @brief 전달된 설정과 기본값으로 FProjectBrowser 객체를 생성합니다.
         */
        FProjectBrowser();

    public:
        /**
         * @brief 프로젝트 선택 화면의 이벤트·그리기 루프를 실행합니다.
         * @param[in] Platform 창과 플랫폼 입력을 제공하는 객체입니다.
         * @param[in] Renderer 뷰포트 자원을 생성할 렌더 모듈입니다.
         * @param[in,out] ProjectFile 처음 선택할 프로젝트 경로이며, 사용자가 고른 프로젝트 파일 경로를 받습니다.
         * @param[in,out] Create 새 프로젝트 생성 여부이며, 사용자의 선택을 받습니다.
         * @param[in] Error 화면에 표시할 이전 오류 메시지입니다.
         * @return 프로젝트를 골랐으면 true, 사용자가 창을 닫았으면 false입니다.
         */
        bool Show(Platform::FWindowsApplication& Platform, Renderer::FRenderer& Renderer, std::filesystem::path& ProjectFile, bool& Create, const Core::FString& Error);

        /**
         * @brief 최근 사용한 프로젝트 목록에 경로를 기록합니다.
         * @param[in] ProjectFile 열거나 기록할 프로젝트 파일 경로입니다.
         */
        static void RememberProject(const std::filesystem::path& ProjectFile);

    private:
        /**
         * @brief 프로젝트 열기·생성 페이지와 오류 상태를 표시합니다.
         * @param[in] Platform 창과 플랫폼 입력을 제공하는 객체입니다.
         * @param[in] Error 화면에 표시할 오류 메시지입니다.
         */
        void Draw(Platform::FWindowsApplication& Platform, const Core::FString& Error);

        /**
         * @brief 기존 프로젝트 열기 UI를 표시합니다.
         * @param[in] Platform 창과 플랫폼 입력을 제공하는 객체입니다.
         */
        void DrawOpenProject(Platform::FWindowsApplication& Platform);

        /**
         * @brief 프로젝트 이름과 폴더를 지정하는 생성 UI를 표시합니다.
         * @param[in] Platform 창과 플랫폼 입력을 제공하는 객체입니다.
         */
        void DrawNewProject(Platform::FWindowsApplication& Platform);

        /**
         * @brief 최근 프로젝트 목록을 디스크에서 읽습니다.
         */
        void LoadRecentProjects();

        /**
         * @brief 입력 이름과 폴더로 구성한 프로젝트 파일 경로를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        std::filesystem::path GetNewProjectFile() const;

        /**
         * @brief 프로젝트 이름이 파일 이름으로 사용 가능한지 검사합니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool IsProjectNameValid() const;

    private:
        /**
         * @brief 에디터 ImGui 스타일의 화면 배율 관리자입니다.
         */
        Editor::FImGuiUIScale mUIScale{};

        /**
         * @brief 프로젝트 이름 입력 버퍼입니다.
         */
        std::array<char, 256> mProjectName{};

        /**
         * @brief 프로젝트 생성 폴더의 UTF-8 입력 버퍼입니다.
         */
        std::array<char, 4096> mLocation{};

        /**
         * @brief 최근 사용한 프로젝트 경로 목록입니다.
         */
        std::vector<std::filesystem::path> mRecentProjects{};

        /**
         * @brief 열기 또는 생성할 프로젝트 파일 경로입니다.
         */
        std::filesystem::path mSelectedFile{};

        /**
         * @brief 프로젝트 생성 화면 표시 여부입니다.
         */
        bool mCreatePage{};

        /**
         * @brief 프로젝트 선택 완료 여부입니다.
         */
        bool mSubmitted{};
    };

}
