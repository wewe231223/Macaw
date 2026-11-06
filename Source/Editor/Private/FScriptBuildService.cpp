/**
 * @file FScriptBuildService.cpp
 * @brief 프로젝트의 C# 스크립트를 자식 프로세스로 빌드하고 클래스 목록을 최신으로 유지하는 서비스의 선언입니다.
 * @ingroup Editor
 */

#include "Editor/FScriptBuildService.h"
#include "Editor/FEditorContext.h"
#include "Core/FilePath.h"
#include "Engine/FProject.h"
#include "Script/FScriptRuntime.h"

#include <Windows.h>
#include <oleauto.h>
#include <shellapi.h>
#include <wrl/client.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <functional>
#include <set>
#include <sstream>
#include <string>
#include <system_error>

namespace Editor {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        /**
         * @brief 스크립트 C# 프로젝트 파일 이름입니다.
         */
        constexpr const char* ProjectFileName{"GameScripts.csproj"};

        /**
         * @brief 엔진 경로를 C# 프로젝트에 전달할 생성 속성 파일 이름입니다.
         */
        constexpr const char* GeneratedPropsName{"Macaw.Generated.props"};

        /**
         * @brief 스크립트 빌드 로그의 상대 경로입니다.
         */
        constexpr const char* BuildLogPath{"obj/MacawBuild.log"};

        /**
         * @brief Visual Studio에서 열 솔루션 파일 이름입니다.
         */
        constexpr const char* SolutionName{"GameScripts.slnx"};

        /**
         * @brief 실행 중인 모듈의 경로에서 실행 파일 디렉터리를 구합니다.
         * @return 실행 파일이 위치한 디렉터리 경로입니다.
         */
        std::filesystem::path GetExecutableDirectory() {
            std::wstring Path(MAX_PATH, L'\0');

            for (;;) {
                const DWORD Length{GetModuleFileNameW(nullptr, Path.data(), static_cast<DWORD>(Path.size()))};

                if (Length < Path.size()) {
                    Path.resize(Length);

                    return std::filesystem::path{Path}.parent_path();
                }

                Path.resize(Path.size() * 2);
            }
        }

        /**
         * @brief 스크립트 소스의 변경 감지 해시와 최근 수정 시각입니다.
         */
        struct FSourceState {
            /**
             * @brief 소스 경로·수정 상태를 합친 변경 감지 해시입니다.
             */
            std::size_t mStamp{};

            /**
             * @brief 스크립트 소스의 가장 최근 수정 시각입니다.
             */
            std::filesystem::file_time_type mNewest{};
        };

        // Visual Studio가 솔루션을 여는 동안 기다리는 최대 시간입니다.

        /**
         * @brief Visual Studio 솔루션 열림을 기다릴 최대 시간입니다.
         */
        constexpr std::chrono::seconds OpenTimeout{120};

        /**
         * @brief Visual Studio 열림 상태의 재확인 간격입니다.
         */
        constexpr std::chrono::milliseconds OpenPollInterval{500};

        using Microsoft::WRL::ComPtr;

        // 실패하면 Result를 비워 둡니다. VS가 바쁘면 RPC_E_CALL_REJECTED로 실패하므로 호출자가 다시 시도합니다.

        /**
         * @brief COM 멤버를 이름으로 찾아 인자와 호출 플래그로 실행합니다.
         * @param[in] Object 조회하거나 호출할 엔진·COM 객체입니다.
         * @param[in] Name 조회·등록·출력할 대상 이름입니다.
         * @param[in] Flags COM 호출 또는 등록 동작 플래그입니다.
         * @param[in] Arguments 호출·파싱에 사용할 인자 목록입니다.
         * @param[in] Count 처리할 원소 수입니다.
         * @param[in] Result COM 호출 결과 또는 호스팅 결과 코드입니다.
         * @return 이름 해석과 COM 호출에 성공하면 true입니다.
         */
        bool InvokeDispatch(IDispatch* Object, const wchar_t* Name, WORD Flags, VARIANT* Arguments, UINT Count, VARIANT* Result) {
            DISPID Id{};
            LPOLESTR Names[]{const_cast<LPOLESTR>(Name)};

            if (Object == nullptr || FAILED(Object->GetIDsOfNames(IID_NULL, Names, 1, LOCALE_USER_DEFAULT, &Id))) {
                return false;
            }

            DISPPARAMS Parameters{Arguments, nullptr, Count, 0};

            return SUCCEEDED(Object->Invoke(Id, IID_NULL, LOCALE_USER_DEFAULT, Flags, &Parameters, Result, nullptr, nullptr));
        }

        /**
         * @brief COM 객체에서 이름에 대응하는 IDispatch 프로퍼티를 읽습니다.
         * @param[in] Object 조회하거나 호출할 엔진·COM 객체입니다.
         * @param[in] Name 조회·등록·출력할 대상 이름입니다.
         * @return 읽은 자동화 객체이며 실패하면 빈 COM 포인터입니다.
         */
        ComPtr<IDispatch> GetDispatchProperty(IDispatch* Object, const wchar_t* Name) {
            VARIANT Value{};

            VariantInit(&Value);

            ComPtr<IDispatch> Result{};

            if (InvokeDispatch(Object, Name, DISPATCH_PROPERTYGET, nullptr, 0, &Value) && Value.vt == VT_DISPATCH) {
                Result = Value.pdispVal;
            }

            VariantClear(&Value);

            return Result;
        }

        // 실행 중인 Visual Studio 중 지정한 솔루션을 연 인스턴스의 DTE를 찾습니다.

        /**
         * @brief 지정 솔루션을 연 Visual Studio 자동화 객체를 찾습니다.
         * @param[in] Solution 조회하거나 열 솔루션 경로입니다.
         * @return 해당 솔루션의 자동화 객체이며 없으면 빈 COM 포인터입니다.
         */
        ComPtr<IDispatch> FindVisualStudio(const std::filesystem::path& Solution) {
            ComPtr<IRunningObjectTable> Table{};
            ComPtr<IEnumMoniker> Monikers{};

            if (FAILED(GetRunningObjectTable(0, &Table)) || FAILED(Table->EnumRunning(&Monikers))) {
                return {};
            }

            ComPtr<IMoniker> Moniker{};

            while (Monikers->Next(1, &Moniker, nullptr) == S_OK) {
                ComPtr<IBindCtx> Context{};
                LPOLESTR DisplayName{};

                if (SUCCEEDED(CreateBindCtx(0, &Context)) && SUCCEEDED(Moniker->GetDisplayName(Context.Get(), nullptr, &DisplayName))) {
                    const bool IsVisualStudio{std::wstring_view{DisplayName}.starts_with(L"!VisualStudio.DTE.")};

                    CoTaskMemFree(DisplayName);

                    ComPtr<IUnknown> Object{};
                    ComPtr<IDispatch> Dte{};

                    if (IsVisualStudio && SUCCEEDED(Table->GetObject(Moniker.Get(), &Object)) && SUCCEEDED(Object.As(&Dte))) {
                        VARIANT Name{};

                        VariantInit(&Name);

                        const ComPtr<IDispatch> Opened{GetDispatchProperty(Dte.Get(), L"Solution")};
                        std::error_code Code{};
                        const bool Matches{InvokeDispatch(Opened.Get(), L"FullName", DISPATCH_PROPERTYGET, nullptr, 0, &Name) && Name.vt == VT_BSTR && Name.bstrVal != nullptr && SysStringLen(Name.bstrVal) > 0 && std::filesystem::equivalent(std::filesystem::path{Name.bstrVal}, Solution, Code)};

                        VariantClear(&Name);

                        if (Matches) {
                            return Dte;
                        }
                    }
                }

                Moniker.Reset();
            }

            return {};
        }

        // 파일을 열고 VS 창을 앞으로 가져옵니다. 파일이 비어 있으면 창만 앞으로 가져옵니다.

        /**
         * @brief Visual Studio 자동화 객체로 소스 파일을 엽니다.
         * @param[in] Dte 솔루션을 연 Visual Studio 자동화 객체입니다.
         * @param[in] File 처리할 원본 소스 파일 경로입니다.
         * @return Visual Studio에 파일 열기를 요청했으면 true입니다.
         */
        bool OpenInVisualStudio(IDispatch* Dte, const std::filesystem::path& File) {
            if (!File.empty()) {
                const ComPtr<IDispatch> Operations{GetDispatchProperty(Dte, L"ItemOperations")};
                VARIANT Argument{};

                VariantInit(&Argument);

                Argument.vt = VT_BSTR;
                Argument.bstrVal = SysAllocString(File.c_str());

                VARIANT Ignored{};

                VariantInit(&Ignored);

                const bool Opened{InvokeDispatch(Operations.Get(), L"OpenFile", DISPATCH_METHOD, &Argument, 1, &Ignored)};

                VariantClear(&Argument);
                VariantClear(&Ignored);

                if (!Opened) {
                    return false;
                }
            }

            const ComPtr<IDispatch> Window{GetDispatchProperty(Dte, L"MainWindow")};
            VARIANT Handle{};

            VariantInit(&Handle);
            InvokeDispatch(Window.Get(), L"Activate", DISPATCH_METHOD, nullptr, 0, nullptr);

            if (InvokeDispatch(Window.Get(), L"HWnd", DISPATCH_PROPERTYGET, nullptr, 0, &Handle) && SUCCEEDED(VariantChangeType(&Handle, &Handle, 0, VT_I8))) {
                const HWND Target{reinterpret_cast<HWND>(static_cast<std::intptr_t>(Handle.llVal))};

                if (IsIconic(Target)) {
                    ShowWindow(Target, SW_RESTORE);
                }

                SetForegroundWindow(Target);
            }

            VariantClear(&Handle);

            return true;
        }

        // 빌드 출력과 숨김 폴더를 제외한 소스, 프로젝트와 props 파일만 봅니다. Scripts가 있으면 .cs 파일 목록도 채웁니다.

        /**
         * @brief 스크립트 경로·수정 시각으로 변경 감지 상태를 계산합니다.
         * @param[in] Directory 스크립트 소스를 탐색할 디렉터리입니다.
         * @param[out] Scripts 탐색한 소스 경로를 받을 선택적 출력 배열입니다.
         * @return 소스 집합의 해시와 가장 최근 수정 시각입니다.
         */
        FSourceState ReadSourceState(const std::filesystem::path& Directory, Core::TArray<std::filesystem::path>* Scripts = nullptr) {
            FSourceState State{};
            std::error_code Code{};

            State.mNewest = std::filesystem::last_write_time(Directory, Code);

            Code.clear();

            std::filesystem::recursive_directory_iterator Iterator{Directory, std::filesystem::directory_options::skip_permission_denied, Code};
            const std::filesystem::recursive_directory_iterator End{};

            for (; !Code && Iterator != End; Iterator.increment(Code)) {
                const std::filesystem::path Name{Iterator->path().filename()};

                if (Iterator->is_directory(Code)) {
                    if (Name == "bin" || Name == "obj" || Name.native().starts_with(L".")) {
                        Iterator.disable_recursion_pending();
                    } else {
                        // 파일을 지우거나 이름을 바꾸면 파일 시각은 그대로이고 폴더 시각만 바뀌므로 폴더도 봅니다.
                        State.mNewest = std::max(State.mNewest, Iterator->last_write_time(Code));
                    }

                    continue;
                }

                const std::filesystem::path Extension{Name.extension()};

                if (Extension != ".cs" && Extension != ".csproj" && Extension != ".props") {
                    continue;
                }

                const auto Time{Iterator->last_write_time(Code)};

                State.mNewest = std::max(State.mNewest, Time);
                State.mStamp ^= std::hash<std::wstring>{}(Iterator->path().native()) + static_cast<std::size_t>(Time.time_since_epoch().count());

                if (Scripts != nullptr && Extension == ".cs") {
                    Scripts->push_back(Iterator->path());
                }
            }

            if (Scripts != nullptr) {
                std::sort(Scripts->begin(), Scripts->end());
            }

            return State;
        }
    }

    /**
     * @brief 로그와 프로젝트 경로를 얻을 컨텍스트를 연결합니다.
     */
    Editor::FScriptBuildService::FScriptBuildService(Editor::FEditorContext* Context)
		: mContext{Context} {
    }

    /**
     * @brief 프로세스 핸들을 닫습니다. 실행 중인 빌드는 스스로 끝나도록 둡니다.
     */
    Editor::FScriptBuildService::~FScriptBuildService() {
        if (mProcess != nullptr) {
            CloseHandle(mProcess);
        }
    }

    /**
     * @brief 스크립트 프로젝트 폴더를 반환합니다.
     */
    std::filesystem::path Editor::FScriptBuildService::GetScriptsDirectory() const {
        return mContext->GetProject()->GetRootPath() / "Scripts";
    }

    /**
     * @brief 프로젝트에 스크립트 프로젝트가 있는지 검사합니다.
     */
    bool Editor::FScriptBuildService::HasProject() const {
        std::error_code Code{};

        return mContext->GetProject() != nullptr && std::filesystem::is_regular_file(GetScriptsDirectory() / ProjectFileName, Code);
    }

    /**
     * @brief 템플릿을 복사해 스크립트 프로젝트를 만들고 빌드를 시작합니다.
     */
    bool Editor::FScriptBuildService::CreateProject() {
        const std::filesystem::path Template{GetExecutableDirectory() / "Templates/Scripts"};
        std::error_code Code{};

        std::filesystem::copy(Template, GetScriptsDirectory(), std::filesystem::copy_options::recursive | std::filesystem::copy_options::skip_existing, Code);

        if (Code || !HasProject()) {
            mContext->AddLog("Cannot create the C# project from " + Core::FilePath::ToUtf8(Template));

            return false;
        }

        mContext->AddLog("Created the C# project in " + Core::FilePath::ToUtf8(GetScriptsDirectory()));
        Rebuild();

        return true;
    }

    /**
     * @brief 스크립트 솔루션을 Visual Studio로 엽니다. 프로젝트가 없으면 먼저 만듭니다.
     */
    bool Editor::FScriptBuildService::OpenProject() {
        if (!HasProject() && !CreateProject()) {
            return false;
        }

        return OpenScriptFile({});
    }

    /**
     * @brief C# 파일을 이 프로젝트의 스크립트 솔루션을 연 Visual Studio에서 엽니다.
     */
    bool Editor::FScriptBuildService::OpenScriptFile(const std::filesystem::path& File) {
        if (!HasProject()) {
            return false;
        }

        // IDE가 프로젝트를 솔루션으로 열도록 없으면 만듭니다. 사용자가 고친 솔루션은 덮어쓰지 않습니다.
        const std::filesystem::path Solution{GetSolutionPath()};
        std::error_code Code{};

        if (!std::filesystem::exists(Solution, Code)) {
            std::ofstream{Solution, std::ios::binary} << "<Solution>\n  <Project Path=\"GameScripts.csproj\" />\n</Solution>\n";
        }

        WriteGeneratedProps();

        // 이 솔루션을 연 VS가 있으면 그 창에 엽니다. 다른 솔루션을 연 VS에는 열지 않습니다.
        if (const ComPtr<IDispatch> Dte{FindVisualStudio(Solution)}; Dte != nullptr && OpenInVisualStudio(Dte.Get(), File)) {
            mPendingFile.clear();
            mPendingDeadline = {};

            return true;
        }

        // 없으면 솔루션을 열고, VS가 솔루션을 다 열 때까지 Update에서 기다렸다가 파일을 엽니다.
        const bool AlreadyWaiting{mPendingDeadline != std::chrono::steady_clock::time_point{}};

        mPendingFile = File;
        mPendingDeadline = std::chrono::steady_clock::now() + OpenTimeout;
        mNextPoll = std::chrono::steady_clock::now() + OpenPollInterval;

        if (AlreadyWaiting) {
            return true;
        }

        if (!OpenWithShell(Solution)) {
            mPendingDeadline = {};

            return false;
        }

        mContext->AddLog("[C#] Opening the script solution in Visual Studio...");

        return true;
    }

    /**
     * @brief 솔루션을 여는 중인 Visual Studio를 주기적으로 찾아, 찾으면 대기 중인 파일을 엽니다.
     */
    void Editor::FScriptBuildService::UpdatePendingOpen() {
        const auto Now{std::chrono::steady_clock::now()};

        if (mPendingDeadline == std::chrono::steady_clock::time_point{} || Now < mNextPoll) {
            return;
        }

        mNextPoll = Now + OpenPollInterval;

        if (const ComPtr<IDispatch> Dte{FindVisualStudio(GetSolutionPath())}; Dte != nullptr && OpenInVisualStudio(Dte.Get(), mPendingFile)) {
            mPendingFile.clear();
            mPendingDeadline = {};
        } else if (Now > mPendingDeadline) {
            mContext->AddLog("[C#] Visual Studio did not open the script solution in time. Open " + Core::FilePath::ToUtf8(mPendingFile.empty() ? GetSolutionPath() : mPendingFile) + " manually.");
            mPendingFile.clear();
            mPendingDeadline = {};
        }
    }

    /**
     * @brief ScriptComponent를 상속한 새 C# 클래스 파일을 만들고 Visual Studio에서 엽니다.
     */
    bool Editor::FScriptBuildService::CreateScript(const Core::FString& ClassName, Core::FString& Error) {
        const bool Valid{!ClassName.empty() && (std::isalpha(static_cast<unsigned char>(ClassName.front())) || ClassName.front() == '_') && std::all_of(ClassName.begin(), ClassName.end(), [](char Character) {
            return std::isalnum(static_cast<unsigned char>(Character)) || Character == '_';
        })};

        if (!Valid) {
            Error = "클래스 이름은 영문자나 _로 시작하고 영문자, 숫자, _만 쓸 수 있습니다.";

            return false;
        }

        if (mContext->GetProject() == nullptr || (!HasProject() && !CreateProject())) {
            Error = "C# 프로젝트를 만들 수 없습니다. 콘솔을 확인하세요.";

            return false;
        }

        const std::filesystem::path Path{GetScriptsDirectory() / (ClassName + ".cs")};
        std::error_code Code{};

        if (std::filesystem::exists(Path, Code)) {
            Error = ClassName + ".cs 파일이 이미 있습니다.";

            return false;
        }

        std::ofstream Output{Path, std::ios::binary};
        Output << "using System.Numerics;\nusing Macaw;\n\nnamespace Game;\n\npublic sealed class " << ClassName << " : ScriptComponent {\n"
               << "    public override void BeginPlay() {\n    }\n\n    public override void Tick(float DeltaTime) {\n    }\n}\n";

        if (!Output) {
            Error = "파일을 쓸 수 없습니다: " + Core::FilePath::ToUtf8(Path);

            return false;
        }

        Output.close();
        Error.clear();
        mScriptFiles.push_back(Path);
        std::sort(mScriptFiles.begin(), mScriptFiles.end());
        mContext->AddLog("[C#] Created " + Core::FilePath::ToUtf8(Path));
        OpenScriptFile(Path);

        return true;
    }

    /**
     * @brief 스크립트 솔루션 파일 경로를 반환합니다.
     */
    std::filesystem::path Editor::FScriptBuildService::GetSolutionPath() const {
        return GetScriptsDirectory() / SolutionName;
    }

    /**
     * @brief 스크립트 폴더를 다시 읽어 C# 파일 목록을 갱신합니다.
     */
    void Editor::FScriptBuildService::RefreshScriptFiles() {
        mScriptFiles.clear();

        if (HasProject()) {
            ReadSourceState(GetScriptsDirectory(), &mScriptFiles);
        }
    }

    /**
     * @brief 스크립트 폴더의 C# 파일 목록을 반환합니다. 포커스를 얻거나 빌드할 때 갱신됩니다.
     */
    const Core::TArray<std::filesystem::path>& Editor::FScriptBuildService::GetScriptFiles() const {
        return mScriptFiles;
    }

    /**
     * @brief 파일을 셸의 기본 프로그램으로 엽니다.
     */
    bool Editor::FScriptBuildService::OpenWithShell(const std::filesystem::path& Path) const {
        const auto Result{reinterpret_cast<std::intptr_t>(ShellExecuteW(nullptr, L"open", Path.c_str(), nullptr, Path.parent_path().c_str(), SW_SHOWNORMAL))};

        if (Result <= 32) {
            mContext->AddLog("[C#] Cannot open " + Core::FilePath::ToUtf8(Path) + ". Associate the file type with Visual Studio or another editor.");

            return false;
        }

        return true;
    }

    /**
     * @brief 이 에디터의 Managed 폴더를 가리키도록 생성 props를 갱신합니다.
     */
    void Editor::FScriptBuildService::WriteGeneratedProps() const {
        const std::filesystem::path Path{GetScriptsDirectory() / GeneratedPropsName};
        std::ostringstream Content{};

        Content << "<Project>\n  <PropertyGroup>\n    <MacawManagedDirectory>" << Core::FilePath::ToUtf8(GetExecutableDirectory() / "Managed") << "\\</MacawManagedDirectory>\n  </PropertyGroup>\n</Project>\n";

        std::ifstream Current{Path, std::ios::binary};
        const std::string Existing{std::istreambuf_iterator<char>{Current}, std::istreambuf_iterator<char>{}};

        if (Existing != Content.str()) {
            Current.close();
            std::ofstream{Path, std::ios::binary} << Content.str();
        }
    }

    /**
     * @brief 소스가 바뀌었는지 확인하고 필요하면 빌드를 시작합니다.
     */
    void Editor::FScriptBuildService::BuildIfChanged() {
        if (!HasProject()) {
            mScriptFiles.clear();

            return;
        }

        // 빌드 중에도 콘텐츠 브라우저의 파일 목록은 갱신합니다.
        mScriptFiles.clear();

        const FSourceState State{ReadSourceState(GetScriptsDirectory(), &mScriptFiles)};

        if (IsBuilding()) {
            return;
        }

        WriteGeneratedProps();

        if (State.mStamp == mKnownStamp) {
            return;
        }

        // IDE에서 이미 빌드했으면 어셈블리가 소스보다 새것이므로 다시 빌드하지 않습니다.
        std::error_code Code{};
        const auto AssemblyTime{std::filesystem::last_write_time(Script::FScriptRuntime::GetAssemblyPath(mContext->GetProject()->GetRootPath()), Code)};
        const bool Changed{Code || State.mNewest > AssemblyTime};

        mKnownStamp = State.mStamp;

        if (Changed) {
            StartBuild();
        }
    }

    /**
     * @brief 변경 여부와 상관없이 빌드를 시작합니다.
     */
    void Editor::FScriptBuildService::Rebuild() {
        if (IsBuilding() || !HasProject()) {
            return;
        }

        WriteGeneratedProps();
        mScriptFiles.clear();
        mKnownStamp = ReadSourceState(GetScriptsDirectory(), &mScriptFiles).mStamp;

        StartBuild();
    }

    /**
     * @brief dotnet build 자식 프로세스를 시작합니다.
     */
    void Editor::FScriptBuildService::StartBuild() {
        const std::filesystem::path Directory{GetScriptsDirectory()};
        std::error_code Code{};

        std::filesystem::create_directories(Directory / "obj", Code);
        std::filesystem::remove(Directory / BuildLogPath, Code);

        // 콘솔 출력은 코드 페이지가 환경마다 다르므로 UTF-8 파일 로그를 읽습니다.
        SetEnvironmentVariableW(L"DOTNET_CLI_UI_LANGUAGE", L"en");

        std::wstring Command{L"dotnet build GameScripts.csproj --nologo -v q -fl -flp:LogFile=obj/MacawBuild.log;Encoding=UTF-8;Verbosity=minimal;NoSummary"};
        STARTUPINFOW Startup{};
        Startup.cb = sizeof(Startup);

        PROCESS_INFORMATION Process{};

        if (!CreateProcessW(nullptr, Command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, Directory.c_str(), &Startup, &Process)) {
            mCompileErrors = false;
            mContext->AddLog("[C#] Cannot run dotnet. Install the .NET 10 SDK to build scripts.");

            return;
        }

        CloseHandle(Process.hThread);

        mProcess = Process.hProcess;
        mContext->AddLog("[C#] Building scripts...");
    }

    /**
     * @brief 끝난 빌드의 종료 코드와 로그를 읽어 결과를 기록합니다.
     */
    void Editor::FScriptBuildService::FinishBuild() {
        DWORD ExitCode{};

        GetExitCodeProcess(mProcess, &ExitCode);
        CloseHandle(mProcess);

        mProcess = nullptr;

        std::ifstream Log{GetScriptsDirectory() / BuildLogPath, std::ios::binary};
        std::string Line{};
        std::set<std::string> Seen{};
        std::size_t Errors{};

        while (std::getline(Log, Line)) {
            if (!Line.empty() && Line.back() == '\r') {
                Line.pop_back();
            }

            const bool Error{Line.find(": error ") != std::string::npos};

            if ((!Error && Line.find(": warning ") == std::string::npos) || !Seen.insert(Line).second) {
                continue;
            }

            // MSBuild가 줄 끝에 붙이는 프로젝트 경로를 지웁니다.
            if (const std::size_t Suffix{Line.rfind(" [")}; Suffix != std::string::npos && Line.ends_with(".csproj]")) {
                Line.resize(Suffix);
            }

            Errors += Error ? 1 : 0;

            mContext->AddLog("[C#] " + Line);
        }

        mCompileErrors = Errors > 0;
        mCatalogDirty = true;

        if (ExitCode == 0) {
            mContext->AddLog("[C#] Build succeeded");
        } else if (mCompileErrors) {
            mContext->AddLog("[C#] Build failed with " + std::to_string(Errors) + " error(s)");
        } else {
            mContext->AddLog("[C#] dotnet build failed with code " + std::to_string(ExitCode) + ". Check that the .NET 10 SDK is installed.");
        }
    }

    /**
     * @brief 어셈블리가 바뀌었으면 클래스 목록을 다시 읽습니다.
     */
    void Editor::FScriptBuildService::RefreshCatalogIfChanged() {
        const std::filesystem::path Root{mContext->GetProject()->GetRootPath()};
        std::error_code Code{};
        const auto Time{std::filesystem::last_write_time(Script::FScriptRuntime::GetAssemblyPath(Root), Code)};
        const auto Current{Code ? std::filesystem::file_time_type{} : Time};

        // 프로젝트를 연 직후에는 이전 프로젝트의 목록이 남아 있을 수 있으므로 시각과 상관없이 읽습니다.
        if (mInitialized && Current == mCatalogTime) {
            return;
        }

        Script::FScriptRuntime* Runtime{Script::FScriptRuntime::Get()};
        Runtime->SetLogFunction([this](const Core::FString& Message) {
            mContext->AddLog("[C#] " + Message);
        });

        if (Runtime->RefreshCatalog(Root)) {
            mCatalogTime = Current;
        }

        Runtime->SetLogFunction({});
    }

    /**
     * @brief 창이 포커스를 얻었음을 알립니다. 다음 Update에서 소스 변경을 확인합니다.
     */
    void Editor::FScriptBuildService::NotifyFocusGained() {
        mCheckSources = true;
        mCatalogDirty = true;
    }

    /**
     * @brief 빌드 종료를 확인하고, 포커스를 얻은 뒤이면 변경된 소스를 빌드합니다.
     */
    void Editor::FScriptBuildService::Update(bool CanReloadCatalog) {
        if (mContext->GetProject() == nullptr) {
            return;
        }

        if (mProcess != nullptr && WaitForSingleObject(mProcess, 0) == WAIT_OBJECT_0) {
            FinishBuild();
        }

        if (CanReloadCatalog && mCatalogDirty && mProcess == nullptr) {
            RefreshCatalogIfChanged();

            mCatalogDirty = false;
        }

        if (!CanReloadCatalog) {
            mCatalogDirty = true;
        }

        if (mCheckSources) {
            mCheckSources = false;

            BuildIfChanged();
        }

        UpdatePendingOpen();

        mInitialized = true;
    }

    /**
     * @brief 빌드가 진행 중인지 검사합니다.
     */
    bool Editor::FScriptBuildService::IsBuilding() const {
        return mProcess != nullptr;
    }

    /**
     * @brief 마지막 빌드에 컴파일 오류가 있었는지 검사합니다.
     */
    bool Editor::FScriptBuildService::HasCompileErrors() const {
        return mCompileErrors;
    }

}
