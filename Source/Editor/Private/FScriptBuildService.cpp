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

    namespace {
        constexpr const char* ProjectFileName{"GameScripts.csproj"};
        constexpr const char* GeneratedPropsName{"Macaw.Generated.props"};
        constexpr const char* BuildLogPath{"obj/MacawBuild.log"};
        constexpr const char* SolutionName{"GameScripts.slnx"};

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

        struct FSourceState {
            std::size_t mStamp{};
            std::filesystem::file_time_type mNewest{};
        };

        // Visual Studio가 솔루션을 여는 동안 기다리는 최대 시간입니다.
        constexpr std::chrono::seconds OpenTimeout{120};
        constexpr std::chrono::milliseconds OpenPollInterval{500};

        using Microsoft::WRL::ComPtr;

        // 실패하면 Result를 비워 둡니다. VS가 바쁘면 RPC_E_CALL_REJECTED로 실패하므로 호출자가 다시 시도합니다.
        bool InvokeDispatch(IDispatch* Object, const wchar_t* Name, WORD Flags, VARIANT* Arguments, UINT Count, VARIANT* Result) {
            DISPID Id{};
            LPOLESTR Names[]{const_cast<LPOLESTR>(Name)};

            if (Object == nullptr || FAILED(Object->GetIDsOfNames(IID_NULL, Names, 1, LOCALE_USER_DEFAULT, &Id))) {
                return false;
            }

            DISPPARAMS Parameters{Arguments, nullptr, Count, 0};

            return SUCCEEDED(Object->Invoke(Id, IID_NULL, LOCALE_USER_DEFAULT, Flags, &Parameters, Result, nullptr, nullptr));
        }

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

    Editor::FScriptBuildService::FScriptBuildService(Editor::FEditorContext* Context)
		: mContext{Context} {
    }

    Editor::FScriptBuildService::~FScriptBuildService() {
        if (mProcess != nullptr) {
            CloseHandle(mProcess);
        }
    }

    std::filesystem::path Editor::FScriptBuildService::GetScriptsDirectory() const {
        return mContext->GetProject()->GetRootPath() / "Scripts";
    }

    bool Editor::FScriptBuildService::HasProject() const {
        std::error_code Code{};
        return mContext->GetProject() != nullptr && std::filesystem::is_regular_file(GetScriptsDirectory() / ProjectFileName, Code);
    }

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

    bool Editor::FScriptBuildService::OpenProject() {
        if (!HasProject() && !CreateProject()) {
            return false;
        }

        return OpenScriptFile({});
    }

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

    std::filesystem::path Editor::FScriptBuildService::GetSolutionPath() const {
        return GetScriptsDirectory() / SolutionName;
    }

    void Editor::FScriptBuildService::RefreshScriptFiles() {
        mScriptFiles.clear();

        if (HasProject()) {
            ReadSourceState(GetScriptsDirectory(), &mScriptFiles);
        }
    }

    const Core::TArray<std::filesystem::path>& Editor::FScriptBuildService::GetScriptFiles() const {
        return mScriptFiles;
    }

    bool Editor::FScriptBuildService::OpenWithShell(const std::filesystem::path& Path) const {
        const auto Result{reinterpret_cast<std::intptr_t>(ShellExecuteW(nullptr, L"open", Path.c_str(), nullptr, Path.parent_path().c_str(), SW_SHOWNORMAL))};

        if (Result <= 32) {
            mContext->AddLog("[C#] Cannot open " + Core::FilePath::ToUtf8(Path) + ". Associate the file type with Visual Studio or another editor.");
            return false;
        }

        return true;
    }

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

    void Editor::FScriptBuildService::Rebuild() {
        if (IsBuilding() || !HasProject()) {
            return;
        }

        WriteGeneratedProps();
        mScriptFiles.clear();
        mKnownStamp = ReadSourceState(GetScriptsDirectory(), &mScriptFiles).mStamp;
        StartBuild();
    }

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

    void Editor::FScriptBuildService::NotifyFocusGained() {
        mCheckSources = true;
        mCatalogDirty = true;
    }

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

    bool Editor::FScriptBuildService::IsBuilding() const {
        return mProcess != nullptr;
    }

    bool Editor::FScriptBuildService::HasCompileErrors() const {
        return mCompileErrors;
    }

}
