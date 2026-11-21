/**
 * @file FAssetImportController.h
 * @brief 파일 선택과 드롭을 가져오기 옵션·대상 디렉터리·완료 통지에 연결합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Core/Math/FVector4.h"
#include "Asset/Import/FAssetImportOption.h"
#include "Editor/FEditorContext.h"
#include "Platform/Window.h"
#include <filesystem>
#include <functional>

namespace Editor {

    /**
     * @brief 파일 선택과 드롭을 가져오기 옵션·대상 디렉터리·완료 통지에 연결합니다.
     */
    class FAssetImportController {

    public:
        /**
         * @brief FAssetImportController의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Context 편집 대상과 공유 서비스를 제공하는 에디터 컨텍스트의 비소유 주소입니다.
         * @param[in] Platform 창·입력·파일 대화상자를 제공하는 플랫폼 애플리케이션입니다.
         */
        FAssetImportController(Editor::FEditorContext* Context, Platform::FWindowsApplication* Platform);

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FAssetImportController();

    public:
        /**
         * @brief 가져오기 대상 디렉터리를 조회할 콜백을 지정합니다.
         * @param[in] Provider 가져오기 대상 디렉터리를 반환할 콜백입니다.
         */
        void SetDirectoryProvider(std::function<std::filesystem::path()> Provider);

        /**
         * @brief 가져오기 경로·개수·오류를 전달할 완료 콜백을 지정합니다.
         * @param[in] Handler 가져오기 경로·개수·오류 문자열을 전달받을 콜백입니다.
         */
        void SetImportedHandler(std::function<void(const std::filesystem::path&, Uint32, const Core::FString&)> Handler);

        /**
         * @brief 프레임별 콘텐츠 드롭 대상 사각형과 대응 디렉터리를 비웁니다.
         */
        void BeginFrame();

        /**
         * @brief 파일 선택 대화상자를 열어 에셋 가져오기를 요청합니다.
         */
        void ImportAsset();

        /**
         * @brief 원본 파일과 대상 폴더를 보관하고 가져오기 옵션 표시를 요청합니다.
         * @param[in] Sources 가져올 원본 파일 경로 목록입니다.
         * @param[in] Directory 탐색 또는 에셋 생성·가져오기 대상 디렉터리입니다.
         */
        void RequestImport(Core::TArray<std::filesystem::path> Sources, const std::filesystem::path& Directory);

        /**
         * @brief 대기 중인 가져오기 옵션을 표시하고 실행 또는 취소를 처리합니다.
         */
        void DrawImportOptions();

        /**
         * @brief 지정한 옵션으로 파일을 가져오고 결과를 완료 콜백에 전달합니다.
         * @param[in] Sources 가져올 원본 파일 경로 목록입니다.
         * @param[in] Directory 탐색 또는 에셋 생성·가져오기 대상 디렉터리입니다.
         * @param[in] Option 적용할 에셋 가져오기 옵션입니다.
         */
        void ImportFiles(const Core::TArray<std::filesystem::path>& Sources, const std::filesystem::path& Directory, const Asset::FAssetImportOption& Option);

        /**
         * @brief Win32 파일 드롭의 원본 경로와 대상 폴더를 큐에 기록합니다.
         * @param[in] Handle Win32 파일 드롭 핸들입니다.
         */
        void QueueFileDrop(void* Handle);

        /**
         * @brief 큐에 쌓인 파일 드롭을 가져오기 요청으로 처리합니다.
         */
        void ImportDroppedFiles();

        /**
         * @brief 이번 프레임에 파일을 받을 콘텐츠 영역과 대상 폴더를 등록합니다.
         * @param[in] Directory 탐색 또는 에셋 생성·가져오기 대상 디렉터리입니다.
         * @param[in] Bounds 왼쪽 위·오른쪽 아래 화면 좌표로 구성한 드롭 영역입니다.
         */
        void RegisterContentDropTarget(const std::filesystem::path& Directory, const FVector4& Bounds);

    private:
        /**
         * @brief 편집 대상과 공유 서비스에 접근하는 에디터 컨텍스트의 비소유 주소입니다.
         */
        Editor::FEditorContext* mContext{};

        /**
         * @brief Win32 창·입력·파일 선택을 제공하는 플랫폼 애플리케이션입니다.
         */
        Platform::FWindowsApplication* mPlatform{};

        /**
         * @brief 파일 드롭 화면 영역에 대응하는 대상 폴더 목록입니다.
         */
        Core::TArray<std::filesystem::path> mContentDropDirectories{};

        /**
         * @brief 이번 프레임에 파일 드롭을 받을 화면 사각형 목록입니다.
         */
        Core::TArray<FVector4> mContentDropBounds{};

        /**
         * @brief 처리 대기 중인 파일 드롭 원본 경로입니다.
         */
        Core::TArray<Core::TArray<std::filesystem::path>> mDroppedFiles{};

        /**
         * @brief 드롭 큐의 각 원본 파일에 대응하는 대상 폴더입니다.
         */
        Core::TArray<std::filesystem::path> mDropDirectories{};

        /**
         * @brief 가져오기 옵션 확인 대기 중인 원본 경로입니다.
         */
        Core::TArray<Core::TArray<std::filesystem::path>> mPendingImports{};

        /**
         * @brief 가져오기 옵션 확인 대기 파일에 대응하는 대상 폴더입니다.
         */
        Core::TArray<std::filesystem::path> mPendingImportDirectories{};

        /**
         * @brief 가져오기 옵션 창에서 편집하는 설정입니다.
         */
        Asset::FAssetImportOption mImportOption{};

        /**
         * @brief 현재 가져오기 대상 폴더를 조회하는 콜백입니다.
         */
        std::function<std::filesystem::path()> mGetDirectory{};

        /**
         * @brief 가져오기 경로·개수·오류를 전달하는 완료 콜백입니다.
         */
        std::function<void(const std::filesystem::path&, Uint32, const Core::FString&)> mImported{};
    };

}
