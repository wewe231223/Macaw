#include "Script/FScriptRuntime.h"
#include "Core/FilePath.h"

#include "ThirdParty/hostfxr.h"
#include "ThirdParty/coreclr_delegates.h"

#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <system_error>
#include <utility>

namespace Script {

    namespace {
        // Source/Managed/Macaw.Engine/Interop.cs의 NativeApi와 순서가 같아야 합니다.
        struct FNativeApi {
            std::intptr_t mSize{};
            void(__stdcall* mLog)(const char*){};
        };

        // Source/Managed/Macaw.Engine/Interop.cs의 ManagedApi와 순서가 같아야 합니다.
        struct FManagedApi {
            std::intptr_t mSize{};
            int(__stdcall* mLoadScripts)(const char*){};
            int(__stdcall* mUnloadScripts)(){};
            std::intptr_t(__stdcall* mCreateInstance)(const char*){};
            void(__stdcall* mTick)(std::intptr_t, float){};
            void(__stdcall* mDestroyInstance)(std::intptr_t){};
        };

        void __stdcall NativeLog(const char* Message) {
            Script::FScriptRuntime::Get()->WriteLog(Message);
        }

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

        // 배치 스크립트가 버전 폴더를 하나만 유지하므로 처음 찾은 폴더를 사용합니다.
        std::filesystem::path FindHostFxr(const std::filesystem::path& DotnetRoot) {
            std::error_code Code{};

            for (const auto& Entry : std::filesystem::directory_iterator{DotnetRoot / "host/fxr", Code}) {
                auto Candidate{Entry.path() / "hostfxr.dll"};

                if (std::filesystem::is_regular_file(Candidate, Code)) {
                    return Candidate;
                }
            }

            return {};
        }

        Core::FString FormatHostError(const char* Step, int Result) {
            char Buffer[16]{};
            snprintf(Buffer, sizeof(Buffer), "0x%08X", static_cast<unsigned int>(Result));

            return Core::FString{Step} + " failed with " + Buffer;
        }
    }

    struct Script::FScriptRuntime::FState {
        FNativeApi mNative{};
        FManagedApi mManaged{};
    };

    Script::FScriptRuntime::FScriptRuntime() = default;

    Script::FScriptRuntime::~FScriptRuntime() = default;

    Script::FScriptRuntime* Script::FScriptRuntime::Get() {
        static Script::FScriptRuntime Runtime{};
        return &Runtime;
    }

    void Script::FScriptRuntime::SetLogFunction(FLogFunction Function) {
        mLogFunction = std::move(Function);
    }

    bool Script::FScriptRuntime::Initialize(Core::FString& Error) {
        if (IsInitialized()) {
            return true;
        }

        const std::filesystem::path Directory{GetExecutableDirectory()};
        const std::filesystem::path DotnetRoot{Directory / "dotnet"};
        const std::filesystem::path Engine{Directory / "Managed/Macaw.Engine.dll"};
        const std::filesystem::path Config{Directory / "Managed/Macaw.Engine.runtimeconfig.json"};
        const std::filesystem::path FxrPath{FindHostFxr(DotnetRoot)};

        if (FxrPath.empty()) {
            Error = "Cannot find the .NET runtime in " + Core::FilePath::ToUtf8(DotnetRoot);
            return false;
        }

        // 프로세스 수명 동안 사용하므로 해제하지 않습니다.
        const HMODULE Fxr{LoadLibraryW(FxrPath.c_str())};

        if (Fxr == nullptr) {
            Error = "Cannot load " + Core::FilePath::ToUtf8(FxrPath);
            return false;
        }

        const auto InitializeRuntime{reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(GetProcAddress(Fxr, "hostfxr_initialize_for_runtime_config"))};
        const auto GetDelegate{reinterpret_cast<hostfxr_get_runtime_delegate_fn>(GetProcAddress(Fxr, "hostfxr_get_runtime_delegate"))};

        if (InitializeRuntime == nullptr || GetDelegate == nullptr) {
            Error = "hostfxr does not export the hosting functions";
            return false;
        }

        const hostfxr_initialize_parameters Parameters{sizeof(hostfxr_initialize_parameters), nullptr, DotnetRoot.c_str()};
        hostfxr_handle Context{};
        int Result{InitializeRuntime(Config.c_str(), &Parameters, &Context)};

        // 0이 아닌 성공 코드는 이미 기동된 런타임을 뜻합니다. 음수만 실패입니다.
        if (Result < 0 || Context == nullptr) {
            Error = FormatHostError("hostfxr_initialize_for_runtime_config", Result);
            return false;
        }

        load_assembly_fn LoadAssembly{};
        get_function_pointer_fn GetFunctionPointer{};

        if (Result = GetDelegate(Context, hdt_load_assembly, reinterpret_cast<void**>(&LoadAssembly)); Result != 0) {
            Error = FormatHostError("hdt_load_assembly", Result);
            return false;
        }

        if (Result = GetDelegate(Context, hdt_get_function_pointer, reinterpret_cast<void**>(&GetFunctionPointer)); Result != 0) {
            Error = FormatHostError("hdt_get_function_pointer", Result);
            return false;
        }

        // Macaw.Engine을 기본 로드 컨텍스트에 두어야 언로드되는 스크립트와 타입을 공유합니다.
        if (Result = LoadAssembly(Engine.c_str(), nullptr, nullptr); Result != 0) {
            Error = FormatHostError("Loading Macaw.Engine", Result);
            return false;
        }

        auto State{std::make_unique<FState>()};
        State->mNative.mSize = sizeof(FNativeApi);
        State->mNative.mLog = &NativeLog;
        State->mManaged.mSize = sizeof(FManagedApi);

        int(__stdcall* Bootstrap)(FNativeApi*, FManagedApi*){};
        Result = GetFunctionPointer(L"Macaw.Host, Macaw.Engine", L"Bootstrap", UNMANAGEDCALLERSONLY_METHOD, nullptr, nullptr, reinterpret_cast<void**>(&Bootstrap));

        if (Result != 0) {
            Error = FormatHostError("Finding Macaw.Host.Bootstrap", Result);
            return false;
        }

        if (Bootstrap(&State->mNative, &State->mManaged) != 1) {
            Error = "Macaw.Engine does not match this engine build. Rebuild the solution.";
            return false;
        }

        mState = std::move(State);

        return true;
    }

    bool Script::FScriptRuntime::IsInitialized() const {
        return mState != nullptr;
    }

    bool Script::FScriptRuntime::LoadScripts(const std::filesystem::path& Assembly, Core::FString& Error) {
        if (!Initialize(Error)) {
            return false;
        }

        std::error_code Code{};

        if (!std::filesystem::is_regular_file(Assembly, Code)) {
            Error = "Cannot find the script assembly " + Core::FilePath::ToUtf8(Assembly);
            return false;
        }

        UnloadScripts();

        if (mState->mManaged.mLoadScripts(Core::FilePath::ToUtf8(Assembly).c_str()) != 1) {
            Error = "Cannot load the script assembly " + Core::FilePath::ToUtf8(Assembly);
            return false;
        }

        mHasScripts = true;

        return true;
    }

    bool Script::FScriptRuntime::UnloadScripts() {
        if (!mHasScripts) {
            return true;
        }

        mHasScripts = false;

        return mState->mManaged.mUnloadScripts() == 1;
    }

    bool Script::FScriptRuntime::HasScripts() const {
        return mHasScripts;
    }

    bool Script::FScriptRuntime::RunOnce(const Core::FString& ClassName, float DeltaTime, Core::FString& Error) {
        if (!mHasScripts) {
            Error = "No C# scripts are loaded";
            return false;
        }

        const std::intptr_t Instance{mState->mManaged.mCreateInstance(ClassName.c_str())};

        if (Instance == 0) {
            Error = "Cannot create the C# script " + ClassName;
            return false;
        }

        mState->mManaged.mTick(Instance, DeltaTime);
        mState->mManaged.mDestroyInstance(Instance);

        return true;
    }

    void Script::FScriptRuntime::WriteLog(const Core::FString& Message) const {
        if (mLogFunction) {
            mLogFunction(Message);
        } else {
            OutputDebugStringA((Message + "\n").c_str());
        }
    }

}
