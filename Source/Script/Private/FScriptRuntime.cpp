#include "Script/FScriptRuntime.h"
#include "Core/FilePath.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Engine/AActor.h"
#include "Engine/Component/USceneComponent.h"

#include "ThirdParty/hostfxr.h"
#include "ThirdParty/coreclr_delegates.h"

#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <system_error>
#include <utility>

namespace Script {

    namespace {
        // 엔진 수학 타입은 alignas(16)이므로 경계에서는 정렬 없는 float 배치만 사용합니다.
        // Source/Managed/Macaw.Engine/Interop.cs의 Transform과 순서가 같아야 합니다.
        struct FAbiTransform {
            float mLocation[3]{};
            float mRotation[4]{};
            float mScale[3]{};
        };

        // Source/Managed/Macaw.Engine/Interop.cs의 NativeApi와 순서가 같아야 합니다.
        struct FNativeApi {
            std::intptr_t mSize{};
            void(__stdcall* mLog)(const char*){};
            int(__stdcall* mActorGetTransform)(CoreUObject::FObjectHandle, FAbiTransform*){};
            int(__stdcall* mActorSetTransform)(CoreUObject::FObjectHandle, const FAbiTransform*){};
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
        };

        void __stdcall NativeLog(const char* Message) {
            Script::FScriptRuntime::Get()->WriteLog(Message);
        }

        // 파괴 중인 액터와 루트가 없는 액터는 Transform을 다룰 수 없으므로 찾지 못한 것으로 처리합니다.
        Engine::AActor* ResolveActor(CoreUObject::FObjectHandle Handle) {
            auto* Actor{CoreUObject::Cast<Engine::AActor>(CoreUObject::FObjectRegistry::Get()->Resolve(Handle))};

            return Actor != nullptr && !Actor->IsPendingDestroy() && Actor->GetRootComponent() != nullptr ? Actor : nullptr;
        }

        int __stdcall NativeActorGetTransform(CoreUObject::FObjectHandle Handle, FAbiTransform* Output) {
            const Engine::AActor* Actor{ResolveActor(Handle)};

            if (Actor == nullptr) {
                return 0;
            }

            const Core::FTransform& Transform{Actor->GetRootComponent()->GetTransform()};
            const Core::FVector& Location{Transform.mLocation};
            const Core::FQuat& Rotation{Transform.mRotation};
            const Core::FVector& Scale{Transform.mScale};
            *Output = {{Location.x, Location.y, Location.z}, {Rotation.x, Rotation.y, Rotation.z, Rotation.w}, {Scale.x, Scale.y, Scale.z}};

            return 1;
        }

        int __stdcall NativeActorSetTransform(CoreUObject::FObjectHandle Handle, const FAbiTransform* Input) {
            Engine::AActor* Actor{ResolveActor(Handle)};

            if (Actor == nullptr) {
                return 0;
            }

            Core::FTransform Transform{Actor->GetRootComponent()->GetTransform()};
            Transform.mLocation = Core::FVector{Input->mLocation[0], Input->mLocation[1], Input->mLocation[2]};
            Transform.mRotation = Core::FQuat{Input->mRotation[0], Input->mRotation[1], Input->mRotation[2], Input->mRotation[3]};
            Transform.mScale = Core::FVector{Input->mScale[0], Input->mScale[1], Input->mScale[2]};

            return Actor->SetActorTransform(Transform) ? 1 : 0;
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
        State->mNative.mActorGetTransform = &NativeActorGetTransform;
        State->mNative.mActorSetTransform = &NativeActorSetTransform;
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
        mAssembly = ProjectRoot / "Scripts/bin/GameScripts.dll";
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
