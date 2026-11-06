#include "Script/FScriptRuntime.h"
#include "Core/FilePath.h"
#include "CoreUObject/FObjectRegistry.h"
#include "Script/FScriptField.h"
#include "ScriptAbi.h"

#include "ThirdParty/hostfxr.h"
#include "ThirdParty/coreclr_delegates.h"

#include <Windows.h>

#include <algorithm>
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
            int(__stdcall* mObjectIsAlive)(CoreUObject::FObjectHandle){};
            void(__stdcall* mCatalogAddClass)(const char*){};
            void(__stdcall* mCatalogAddField)(const char*, const char*, const char*){};
            void* const* mBindings{};
            std::intptr_t mBindingCount{};
            std::intptr_t mBindingHash{};
        };

        // Source/Managed/Macaw.Engine/Interop.cs의 ManagedApi와 순서가 같아야 합니다.
        struct FManagedApi {
            std::intptr_t mSize{};
            int(__stdcall* mLoadScripts)(const char*){};
            int(__stdcall* mUnloadScripts)(){};
            std::intptr_t(__stdcall* mCreateInstance)(const char*, CoreUObject::FObjectHandle){};
            void(__stdcall* mBeginPlay)(std::intptr_t){};
            void(__stdcall* mTick)(std::intptr_t, float){};
            void(__stdcall* mEndPlay)(std::intptr_t){};
            void(__stdcall* mDestroyInstance)(std::intptr_t){};
            void(__stdcall* mDescribeScripts)(){};
            int(__stdcall* mSetField)(std::intptr_t, const char*, const char*, const char*){};
            void(__stdcall* mOverlap)(std::intptr_t, int, CoreUObject::FObjectHandle){};
        };

        // DescribeScripts 호출 동안만 유효한 출력 목록입니다.
        Core::TArray<Script::FScriptClassInfo>* sCatalogOutput{};

        void __stdcall NativeLog(const char* Message) {
            Script::FScriptRuntime::Get()->WriteLog(Message);
        }

        void __stdcall NativeCatalogAddClass(const char* Name) {
            sCatalogOutput->push_back({Name, {}});
        }

        void __stdcall NativeCatalogAddField(const char* Name, const char* Type, const char* Default) {
            sCatalogOutput->back().mFields.push_back({Name, Type, Default});
        }

        int __stdcall NativeObjectIsAlive(CoreUObject::FObjectHandle Handle) {
            return Script::Abi::Resolve<CoreUObject::UObject>(Handle) != nullptr ? 1 : 0;
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
        if (mState != nullptr) {
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
        State->mNative.mObjectIsAlive = &NativeObjectIsAlive;
        State->mNative.mCatalogAddClass = &NativeCatalogAddClass;
        State->mNative.mCatalogAddField = &NativeCatalogAddField;
        const Script::Abi::FBindingTable Bindings{Script::Abi::GetBindingTable()};
        State->mNative.mBindings = Bindings.mThunks;
        State->mNative.mBindingCount = Bindings.mCount;
        State->mNative.mBindingHash = Bindings.mHash;
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

    void Script::FScriptRuntime::BeginSession(const std::filesystem::path& ProjectRoot) {
        EndSession();
        mAssembly = GetAssemblyPath(ProjectRoot);
    }

    std::filesystem::path Script::FScriptRuntime::GetAssemblyPath(const std::filesystem::path& ProjectRoot) {
        return ProjectRoot / "Scripts/bin/GameScripts.dll";
    }

    bool Script::FScriptRuntime::RefreshCatalog(const std::filesystem::path& ProjectRoot) {
        if (!mAssembly.empty()) {
            return false;
        }

        mCatalog.clear();

        const std::filesystem::path Assembly{GetAssemblyPath(ProjectRoot)};
        const Core::FString Utf8{Core::FilePath::ToUtf8(Assembly)};
        std::error_code Code{};

        if (!std::filesystem::is_regular_file(Assembly, Code)) {
            return true;
        }

        Core::FString Error{};

        if (!Initialize(Error)) {
            WriteLog(Error);
            return false;
        }

        if (mState->mManaged.mLoadScripts(Utf8.c_str()) != 1) {
            WriteLog("Cannot load the script assembly " + Utf8);
            return false;
        }

        sCatalogOutput = &mCatalog;
        mState->mManaged.mDescribeScripts();
        sCatalogOutput = nullptr;
        mState->mManaged.mUnloadScripts();

        return true;
    }

    const Core::TArray<Script::FScriptClassInfo>& Script::FScriptRuntime::GetCatalog() const {
        return mCatalog;
    }

    const Script::FScriptClassInfo* Script::FScriptRuntime::FindClass(const Core::FString& ClassName) const {
        const auto Position{std::find_if(mCatalog.begin(), mCatalog.end(), [&ClassName](const Script::FScriptClassInfo& Class) {
            return Class.mName == ClassName;
        })};

        return Position != mCatalog.end() ? &*Position : nullptr;
    }

    void Script::FScriptRuntime::SetInstanceField(std::intptr_t Instance, const Script::FScriptField& Field) {
        Core::FString Value{Field.mValue};

        // 액터 참조는 씬에 GUID로 저장되므로, 복제 월드에서 해석한 핸들을 문자열로 넘깁니다.
        if (Field.mType == "Actor") {
            const CoreUObject::UObject* Object{Field.mObject.Resolve()};

            if (Object == nullptr) {
                return;
            }

            const CoreUObject::FObjectHandle Handle{Object->GetHandle()};
            Value = std::to_string(Handle.mIndex) + ":" + std::to_string(Handle.mGeneration);
        }

        mState->mManaged.mSetField(Instance, Field.mName.c_str(), Field.mType.c_str(), Value.c_str());
    }

    void Script::FScriptRuntime::EndSession() {
        // 수집되지 않으면 C# 쪽에서 경고를 남기므로 결과를 따로 보고하지 않습니다.
        if (mHasScripts) {
            mState->mManaged.mUnloadScripts();
        }

        mHasScripts = false;
        mLoadFailed = false;
        mAssembly.clear();
    }

    bool Script::FScriptRuntime::EnsureScripts() {
        if (mHasScripts) {
            return true;
        }

        if (mAssembly.empty() || mLoadFailed) {
            return false;
        }

        Core::FString Error{};
        std::error_code Code{};

        if (!std::filesystem::is_regular_file(mAssembly, Code)) {
            Error = "Cannot find the script assembly " + Core::FilePath::ToUtf8(mAssembly);
        } else if (Initialize(Error) && mState->mManaged.mLoadScripts(Core::FilePath::ToUtf8(mAssembly).c_str()) != 1) {
            Error = "Cannot load the script assembly " + Core::FilePath::ToUtf8(mAssembly);
        }

        mHasScripts = Error.empty();
        mLoadFailed = !mHasScripts;

        if (mLoadFailed) {
            WriteLog(Error);
        }

        return mHasScripts;
    }

    std::intptr_t Script::FScriptRuntime::CreateInstance(const Core::FString& ClassName, CoreUObject::FObjectHandle Owner) {
        return EnsureScripts() ? mState->mManaged.mCreateInstance(ClassName.c_str(), Owner) : 0;
    }

    void Script::FScriptRuntime::BeginPlayInstance(std::intptr_t Instance) {
        mState->mManaged.mBeginPlay(Instance);
    }

    void Script::FScriptRuntime::TickInstance(std::intptr_t Instance, float DeltaTime) {
        mState->mManaged.mTick(Instance, DeltaTime);
    }

    void Script::FScriptRuntime::EndPlayInstance(std::intptr_t Instance) {
        mState->mManaged.mEndPlay(Instance);
    }

    void Script::FScriptRuntime::OverlapInstance(std::intptr_t Instance, bool Begin, CoreUObject::FObjectHandle Other) {
        mState->mManaged.mOverlap(Instance, Begin ? 1 : 0, Other);
    }

    void Script::FScriptRuntime::DestroyInstance(std::intptr_t Instance) {
        mState->mManaged.mDestroyInstance(Instance);
    }

    void Script::FScriptRuntime::WriteLog(const Core::FString& Message) const {
        if (mLogFunction) {
            mLogFunction(Message);
        } else {
            OutputDebugStringA((Message + "\n").c_str());
        }
    }

}
