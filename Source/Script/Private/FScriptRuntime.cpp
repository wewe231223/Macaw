/**
 * @file FScriptRuntime.cpp
 * @brief CoreCLR 호스팅과 스크립트 어셈블리·인스턴스 수명을 관리합니다.
 * @ingroup Script
 */

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

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {

        // Source/Managed/Macaw.Engine/Interop.cs의 NativeApi와 순서가 같아야 합니다.

        /**
         * @brief 관리 코드가 호출할 네이티브 로그·객체·카탈로그·바인딩 함수 테이블입니다.
         */
        struct FNativeApi {
            /**
             * @brief ABI 호환성 검사에 쓰는 테이블 바이트 크기입니다.
             */
            std::intptr_t mSize{};

            /**
             * @brief 관리 UTF-8 로그를 네이티브 로그에 전달하는 콜백입니다.
             */
            void(__stdcall* mLog)(const char*){};

            /**
             * @brief 엔진 객체 핸들 생존 조회 콜백입니다.
             */
            int(__stdcall* mObjectIsAlive)(CoreUObject::FObjectHandle){};

            /**
             * @brief 관리 클래스의 네이티브 카탈로그 추가 콜백입니다.
             */
            void(__stdcall* mCatalogAddClass)(const char*){};

            /**
             * @brief 관리 노출 필드의 네이티브 카탈로그 추가 콜백입니다.
             */
            void(__stdcall* mCatalogAddField)(const char*, const char*, const char*){};

            /**
             * @brief 생성한 네이티브 바인딩 함수 주소입니다.
             */
            void* const* mBindings{};

            /**
             * @brief 바인딩 함수 테이블의 원소 수입니다.
             */
            std::intptr_t mBindingCount{};

            /**
             * @brief 네이티브·관리 바인딩 서명 검사 해시입니다.
             */
            std::intptr_t mBindingHash{};
        };

        // Source/Managed/Macaw.Engine/Interop.cs의 ManagedApi와 순서가 같아야 합니다.

        /**
         * @brief 네이티브가 호출할 관리 코드의 스크립트 수명·필드·이벤트 함수 테이블입니다.
         */
        struct FManagedApi {
            /**
             * @brief ABI 호환성 검사에 쓰는 테이블 바이트 크기입니다.
             */
            std::intptr_t mSize{};

            /**
             * @brief 관리 코드의 스크립트 어셈블리 로드 콜백입니다.
             */
            int(__stdcall* mLoadScripts)(const char*, int){};

            /**
             * @brief 관리 스크립트 컨텍스트의 언로드 콜백입니다.
             */
            int(__stdcall* mUnloadScripts)(){};

            /**
             * @brief 클래스 이름·엔진 소유자로 스크립트 인스턴스를 만드는 콜백입니다.
             */
            std::intptr_t(__stdcall* mCreateInstance)(const char*, CoreUObject::FObjectHandle){};

            /**
             * @brief 관리 인스턴스 BeginPlay 호출입니다.
             */
            void(__stdcall* mBeginPlay)(std::intptr_t){};

            /**
             * @brief 관리 인스턴스 Tick 호출입니다.
             */
            void(__stdcall* mTick)(std::intptr_t, float){};

            /**
             * @brief 관리 인스턴스 EndPlay 호출입니다.
             */
            void(__stdcall* mEndPlay)(std::intptr_t){};

            /**
             * @brief 관리 인스턴스 GCHandle 해제 호출입니다.
             */
            void(__stdcall* mDestroyInstance)(std::intptr_t){};

            /**
             * @brief 관리 클래스·노출 필드 수집 호출입니다.
             */
            void(__stdcall* mDescribeScripts)(){};

            /**
             * @brief 관리 인스턴스 노출 필드 설정 호출입니다.
             */
            int(__stdcall* mSetField)(std::intptr_t, const char*, const char*, const char*){};

            /**
             * @brief 관리 인스턴스 겹침 이벤트 전달 호출입니다.
             */
            void(__stdcall* mOverlap)(std::intptr_t, int, CoreUObject::FObjectHandle){};
        };

        // DescribeScripts 호출 동안만 유효한 출력 목록입니다.

        /**
         * @brief 현재 관리 코드의 클래스·필드 수집 결과를 받을 카탈로그입니다.
         */
        Core::TArray<Script::FScriptClassInfo>* sCatalogOutput{};

        /**
         * @brief 관리 코드의 UTF-8 로그를 네이티브 로그 경로로 전달합니다.
         * @param[in] Message 전달할 UTF-8 관리 로그 문자열입니다.
         */
        void __stdcall NativeLog(const char* Message) {
            Script::FScriptRuntime::Get()->WriteLog(Message);
        }

        /**
         * @brief 관리 코드가 보고한 클래스를 수집 중인 목록에 추가합니다.
         * @param[in] Name 조회·등록·출력할 대상 이름입니다.
         */
        void __stdcall NativeCatalogAddClass(const char* Name) {
            sCatalogOutput->push_back({Name, {}});
        }

        /**
         * @brief 관리 코드가 보고한 필드를 마지막 수집 클래스에 추가합니다.
         * @param[in] Name 조회·등록·출력할 대상 이름입니다.
         * @param[in] Type 생성·조회·변환할 타입입니다.
         * @param[in] Default 관리 코드가 보고한 필드 기본값입니다.
         */
        void __stdcall NativeCatalogAddField(const char* Name, const char* Type, const char* Default) {
            sCatalogOutput->back().mFields.push_back({Name, Type, Default});
        }

        /**
         * @brief 관리 코드가 가진 엔진 객체 핸들의 생존 여부를 검사합니다.
         * @param[in] Handle 생존 여부를 검사할 엔진 객체 핸들입니다.
         * @return 핸들이 살아 있는 객체이면 1, 아니면 0입니다.
         */
        int __stdcall NativeObjectIsAlive(CoreUObject::FObjectHandle Handle) {
            return Script::Abi::Resolve<CoreUObject::UObject>(Handle) != nullptr ? 1 : 0;
        }

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

        // 배치 스크립트가 버전 폴더를 하나만 유지하므로 처음 찾은 폴더를 사용합니다.

        /**
         * @brief 배포된 .NET 디렉터리에서 사용할 hostfxr 경로를 찾습니다.
         * @param[in] DotnetRoot 배포된 .NET 런타임 루트입니다.
         * @return 배포된 hostfxr 라이브러리 경로이며 찾지 못하면 빈 경로입니다.
         */
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

        /**
         * @brief CoreCLR 호스팅 단계와 결과 코드를 오류 문자열로 구성합니다.
         * @param[in] Step 실패한 CoreCLR 호스팅 단계 이름입니다.
         * @param[in] Result COM 호출 결과 또는 호스팅 결과 코드입니다.
         * @return 해석·변환한 문자열입니다.
         */
        Core::FString FormatHostError(const char* Step, int Result) {
            char Buffer[16]{};

            snprintf(Buffer, sizeof(Buffer), "0x%08X", static_cast<unsigned int>(Result));

            return Core::FString{Step} + " failed with " + Buffer;
        }
    }

    /**
     * @brief CoreCLR 경계에서 교환하는 네이티브·관리 함수 테이블입니다.
     */
    struct Script::FScriptRuntime::FState {
        /**
         * @brief 관리 코드에 전달할 로그·객체·바인딩 호출 테이블입니다.
         */
        FNativeApi mNative{};

        /**
         * @brief 관리 코드에서 받은 수명·필드·이벤트 호출 테이블입니다.
         */
        FManagedApi mManaged{};
    };

    /**
     * @brief 프로세스 수명 동안 유지되는 런타임 상태를 구성합니다.
     */
    Script::FScriptRuntime::FScriptRuntime() = default;

    /**
     * @brief 런타임 상태를 정리합니다.
     */
    Script::FScriptRuntime::~FScriptRuntime() = default;

    /**
     * @brief 프로세스에 하나뿐인 런타임을 반환합니다.
     */
    Script::FScriptRuntime* Script::FScriptRuntime::Get() {
        static Script::FScriptRuntime Runtime{};

        return &Runtime;
    }

    /**
     * @brief C# 로그 메시지를 받을 콜백을 설정합니다.
     */
    void Script::FScriptRuntime::SetLogFunction(FLogFunction Function) {
        mLogFunction = std::move(Function);
    }

    /**
     * @brief 실행 파일 옆의 dotnet 폴더에서 CoreCLR을 기동하고 Macaw.Engine을 로드합니다.
     */
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

    /**
     * @brief 실행 세션을 시작하고 프로젝트의 스크립트 어셈블리 경로를 정합니다.
     */
    void Script::FScriptRuntime::BeginSession(const std::filesystem::path& ProjectRoot, bool ShadowCopy) {
        EndSession();

        mAssembly = GetAssemblyPath(ProjectRoot);
        mShadowCopy = ShadowCopy;
    }

    /**
     * @brief 프로젝트의 스크립트 어셈블리 경로를 반환합니다.
     */
    std::filesystem::path Script::FScriptRuntime::GetAssemblyPath(const std::filesystem::path& ProjectRoot) {
        return ProjectRoot / "Scripts/bin/GameScripts.dll";
    }

    /**
     * @brief 프로젝트의 스크립트를 잠시 로드해 클래스와 필드 목록을 다시 읽습니다.
     */
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

        if (mState->mManaged.mLoadScripts(Utf8.c_str(), 1) != 1) {
            WriteLog("Cannot load the script assembly " + Utf8);

            return false;
        }

        sCatalogOutput = &mCatalog;

        mState->mManaged.mDescribeScripts();

        sCatalogOutput = nullptr;

        mState->mManaged.mUnloadScripts();

        return true;
    }

    /**
     * @brief 마지막으로 읽은 스크립트 클래스 목록을 반환합니다.
     */
    const Core::TArray<Script::FScriptClassInfo>& Script::FScriptRuntime::GetCatalog() const {
        return mCatalog;
    }

    /**
     * @brief 목록에서 스크립트 클래스를 찾습니다.
     */
    const Script::FScriptClassInfo* Script::FScriptRuntime::FindClass(const Core::FString& ClassName) const {
        const auto Position{std::find_if(mCatalog.begin(), mCatalog.end(), [&ClassName](const Script::FScriptClassInfo& Class) {
            return Class.mName == ClassName;
        })};

        return Position != mCatalog.end() ? &*Position : nullptr;
    }

    /**
     * @brief 씬에 저장된 값을 인스턴스의 필드에 넣습니다.
     */
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

    /**
     * @brief 실행 세션을 끝내고 스크립트를 언로드합니다.
     */
    void Script::FScriptRuntime::EndSession() {
        // 수집되지 않으면 C# 쪽에서 경고를 남기므로 결과를 따로 보고하지 않습니다.
        if (mHasScripts) {
            mState->mManaged.mUnloadScripts();
        }

        mHasScripts = false;
        mLoadFailed = false;
        mAssembly.clear();
    }

    /**
     * @brief 세션의 스크립트 어셈블리를 로드합니다.
     */
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
        } else if (Initialize(Error) && mState->mManaged.mLoadScripts(Core::FilePath::ToUtf8(mAssembly).c_str(), mShadowCopy ? 1 : 0) != 1) {
            Error = "Cannot load the script assembly " + Core::FilePath::ToUtf8(mAssembly);
        }

        mHasScripts = Error.empty();
        mLoadFailed = !mHasScripts;

        if (mLoadFailed) {
            WriteLog(Error);
        }

        return mHasScripts;
    }

    /**
     * @brief 스크립트 클래스의 인스턴스를 만듭니다.
     */
    std::intptr_t Script::FScriptRuntime::CreateInstance(const Core::FString& ClassName, CoreUObject::FObjectHandle Owner) {
        return EnsureScripts() ? mState->mManaged.mCreateInstance(ClassName.c_str(), Owner) : 0;
    }

    /**
     * @brief 인스턴스의 BeginPlay를 호출합니다.
     */
    void Script::FScriptRuntime::BeginPlayInstance(std::intptr_t Instance) {
        mState->mManaged.mBeginPlay(Instance);
    }

    /**
     * @brief 인스턴스의 Tick을 호출합니다.
     */
    void Script::FScriptRuntime::TickInstance(std::intptr_t Instance, float DeltaTime) {
        mState->mManaged.mTick(Instance, DeltaTime);
    }

    /**
     * @brief 인스턴스의 EndPlay를 호출합니다.
     */
    void Script::FScriptRuntime::EndPlayInstance(std::intptr_t Instance) {
        mState->mManaged.mEndPlay(Instance);
    }

    /**
     * @brief 겹침 시작이나 끝을 C# 인스턴스에 전달합니다.
     */
    void Script::FScriptRuntime::OverlapInstance(std::intptr_t Instance, bool Begin, CoreUObject::FObjectHandle Other) {
        mState->mManaged.mOverlap(Instance, Begin ? 1 : 0, Other);
    }

    /**
     * @brief 인스턴스 핸들을 해제합니다.
     */
    void Script::FScriptRuntime::DestroyInstance(std::intptr_t Instance) {
        mState->mManaged.mDestroyInstance(Instance);
    }

    /**
     * @brief 등록된 로그 콜백에 메시지를 전달합니다.
     */
    void Script::FScriptRuntime::WriteLog(const Core::FString& Message) const {
        if (mLogFunction) {
            mLogFunction(Message);
        } else {
            OutputDebugStringA((Message + "\n").c_str());
        }
    }

}
