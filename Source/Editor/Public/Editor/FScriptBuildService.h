/**
 * @file FScriptBuildService.h
 * @brief 프로젝트의 C# 스크립트를 자식 프로세스로 빌드하고 클래스 목록을 최신으로 유지하는 서비스의 선언입니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"

#include <chrono>
#include <cstddef>
#include <filesystem>

namespace Editor {

    class FEditorContext;

    /**
     * @brief Scripts 폴더의 변경을 감지해 dotnet build를 실행하고, 끝나면 오류를 콘솔에 표시합니다.
     * @note 작업 스레드 없이 매 프레임 프로세스 상태만 확인합니다. IDE에서 빌드한 결과도 포커스가 돌아올 때 반영합니다. Scripts/GameScripts.csproj가 없는 프로젝트는
     * 아무 비용도 치르지 않으며 .NET SDK도 필요하지 않습니다.
     */
    class FScriptBuildService final {
    public:
        /**
         * @brief 로그와 프로젝트 경로를 얻을 컨텍스트를 연결합니다.
         * @param[in] Context 프로세스가 끝날 때까지 유효한 에디터 컨텍스트입니다.
         */
        explicit FScriptBuildService(Editor::FEditorContext* Context);

        /**
         * @brief 프로세스 핸들을 닫습니다. 실행 중인 빌드는 스스로 끝나도록 둡니다.
         */
        ~FScriptBuildService();

        /**
         * @brief 소유 핸들이 중복되지 않도록 복사를 금지합니다.
         */
        FScriptBuildService(const Editor::FScriptBuildService&) = delete;

        /**
         * @brief 소유 핸들이 중복되지 않도록 대입을 금지합니다.
         */
        Editor::FScriptBuildService& operator=(const Editor::FScriptBuildService&) = delete;

    public:
        /**
         * @brief 빌드 종료를 확인하고, 포커스를 얻은 뒤이면 변경된 소스를 빌드합니다.
         * @param[in] CanReloadCatalog 실행 세션이 없어 클래스 목록을 다시 읽어도 되는지 나타냅니다.
         */
        void Update(bool CanReloadCatalog);

        /**
         * @brief 창이 포커스를 얻었음을 알립니다. 다음 Update에서 소스 변경을 확인합니다.
         * @note 최소화 중에는 프레임이 돌지 않으므로 포커스 상태를 프레임마다 비교하는 대신 창 메시지로 받습니다.
         */
        void NotifyFocusGained();

        /**
         * @brief 프로젝트에 스크립트 프로젝트가 있는지 검사합니다.
         * @return Scripts/GameScripts.csproj가 있으면 true입니다.
         */
        bool HasProject() const;

        /**
         * @brief 템플릿을 복사해 스크립트 프로젝트를 만들고 빌드를 시작합니다.
         * @return 프로젝트를 만들면 true입니다. 이미 있는 파일은 덮어쓰지 않습니다.
         */
        bool CreateProject();

        /**
         * @brief 스크립트 솔루션을 Visual Studio로 엽니다. 프로젝트가 없으면 먼저 만듭니다.
         * @return 열기를 요청하면 true입니다.
         */
        bool OpenProject();

        /**
         * @brief C# 파일을 이 프로젝트의 스크립트 솔루션을 연 Visual Studio에서 엽니다.
         * @param[in] File 열 파일이며, 비어 있으면 솔루션 창만 앞으로 가져옵니다.
         * @return 열기를 요청하면 true입니다.
         * @note 솔루션을 연 VS가 없으면 솔루션부터 열고, 로드가 끝나면 Update에서 파일을 엽니다.
         */
        bool OpenScriptFile(const std::filesystem::path& File);

        /**
         * @brief ScriptComponent를 상속한 새 C# 클래스 파일을 만들고 Visual Studio에서 엽니다.
         * @param[in] ClassName C# 식별자 규칙을 따르는 클래스 이름이며 파일 이름으로도 씁니다.
         * @param[out] Error 실패한 이유입니다.
         * @return 파일을 만들면 true입니다. C# 프로젝트가 없으면 먼저 만듭니다.
         */
        bool CreateScript(const Core::FString& ClassName, Core::FString& Error);

        /**
         * @brief 스크립트 폴더를 다시 읽어 C# 파일 목록을 갱신합니다.
         */
        void RefreshScriptFiles();

        /**
         * @brief 스크립트 폴더의 C# 파일 목록을 반환합니다. 포커스를 얻거나 빌드할 때 갱신됩니다.
         * @return 절대 경로 목록이며 이름순입니다.
         */
        const Core::TArray<std::filesystem::path>& GetScriptFiles() const;

        /**
         * @brief 소스가 바뀌었는지 확인하고 필요하면 빌드를 시작합니다.
         */
        void BuildIfChanged();

        /**
         * @brief 변경 여부와 상관없이 빌드를 시작합니다.
         */
        void Rebuild();

        /**
         * @brief 빌드가 진행 중인지 검사합니다.
         * @return 자식 프로세스가 실행 중이면 true입니다.
         */
        bool IsBuilding() const;

        /**
         * @brief 마지막 빌드에 컴파일 오류가 있었는지 검사합니다.
         * @return 컴파일 오류가 있으면 true입니다. SDK가 없는 등 도구 실패는 포함하지 않습니다.
         */
        bool HasCompileErrors() const;

        /**
         * @brief 게임 내보내기용으로 스크립트를 Release 구성으로 빌드하고 끝날 때까지 기다립니다.
         * @param[in] OutputDirectory 빌드 결과를 둘 폴더입니다. 스크립트 DLL과 NuGet 의존성이 들어갑니다.
         * @param[out] Error 실패한 이유입니다.
         * @return 빌드에 성공하면 true입니다.
         * @note 진행 중인 자동 빌드가 있으면 먼저 끝날 때까지 기다립니다. 내보내기는 한 번 하는 작업이라 에디터를 잠시 멈춥니다.
         */
        bool BuildForExport(const std::filesystem::path& OutputDirectory, Core::FString& Error);

    private:
        /**
         * @brief 스크립트 프로젝트 폴더를 반환합니다.
         * @return 프로젝트 루트의 Scripts 폴더 경로입니다.
         */
        std::filesystem::path GetScriptsDirectory() const;

        /**
         * @brief 스크립트 솔루션 파일 경로를 반환합니다.
         * @return Scripts 폴더의 GameScripts.slnx 경로입니다.
         */
        std::filesystem::path GetSolutionPath() const;

        /**
         * @brief 솔루션을 여는 중인 Visual Studio를 주기적으로 찾아, 찾으면 대기 중인 파일을 엽니다.
         */
        void UpdatePendingOpen();

        /**
         * @brief 이 에디터의 Managed 폴더를 가리키도록 생성 props를 갱신합니다.
         * @note 내용이 같으면 쓰지 않으므로 불필요한 재빌드가 일어나지 않습니다.
         */
        void WriteGeneratedProps() const;

        /**
         * @brief dotnet build 자식 프로세스를 시작합니다.
         */
        void StartBuild();

        /**
         * @brief 파일을 셸의 기본 프로그램으로 엽니다.
         * @param[in] Path 열 파일입니다.
         * @return 열기를 요청하면 true입니다.
         */
        bool OpenWithShell(const std::filesystem::path& Path) const;

        /**
         * @brief 끝난 빌드의 종료 코드와 로그를 읽어 결과를 기록합니다.
         */
        void FinishBuild();

        /**
         * @brief 빌드 로그의 오류와 경고를 콘솔에 옮깁니다.
         * @param[in] LogPath 읽을 UTF-8 빌드 로그입니다.
         * @return 오류 개수입니다.
         */
        std::size_t ReportBuildLog(const std::filesystem::path& LogPath) const;

        /**
         * @brief 어셈블리가 바뀌었으면 클래스 목록을 다시 읽습니다.
         */
        void RefreshCatalogIfChanged();

    private:
        /**
         * @brief 로그와 프로젝트 경로를 제공하는 컨텍스트의 비소유 주소입니다.
         */
        Editor::FEditorContext* mContext{};

        /**
         * @brief 실행 중인 빌드의 프로세스 핸들이며, 없으면 nullptr입니다.
         */
        void* mProcess{};

        /**
         * @brief 마지막으로 확인한 소스 파일 목록과 수정 시각의 해시입니다.
         * @note 같은 소스로 실패한 빌드를 포커스가 돌아올 때마다 반복하지 않게 합니다.
         */
        std::size_t mKnownStamp{};

        /**
         * @brief 스크립트 폴더의 C# 파일 목록입니다.
         */
        Core::TArray<std::filesystem::path> mScriptFiles{};

        /**
         * @brief Visual Studio가 솔루션을 연 뒤 열 파일입니다.
         */
        std::filesystem::path mPendingFile{};

        /**
         * @brief 솔루션 열기를 기다리는 마감 시각이며, 기다리지 않으면 기본값입니다.
         */
        std::chrono::steady_clock::time_point mPendingDeadline{};

        /**
         * @brief 다음으로 실행 중인 Visual Studio를 찾을 시각입니다.
         */
        std::chrono::steady_clock::time_point mNextPoll{};

        /**
         * @brief 클래스 목록을 읽은 어셈블리의 수정 시각입니다.
         */
        std::filesystem::file_time_type mCatalogTime{};

        /**
         * @brief 다음 Update에서 소스 변경을 확인해야 하는지 나타냅니다.
         */
        bool mCheckSources{true};

        /**
         * @brief 첫 Update에서 초기 확인을 수행했는지 나타냅니다.
         */
        bool mInitialized{};

        /**
         * @brief 마지막 빌드에 컴파일 오류가 있었는지 나타냅니다.
         */
        bool mCompileErrors{};

        /**
         * @brief 클래스 목록을 다시 확인해야 하는지 나타냅니다.
         */
        bool mCatalogDirty{true};
    };

}
