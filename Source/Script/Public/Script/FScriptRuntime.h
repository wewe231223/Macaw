/**
 * @file FScriptRuntime.h
 * @brief 프로세스에 CoreCLR을 호스팅하고 C# 스크립트 어셈블리와 인스턴스를 관리하는 런타임의 선언입니다.
 *
 * @details
 * C# 스크립트는 별도 프로세스나 인터프리터가 아니라 엔진 프로세스 안에 기동한 .NET 런타임(CoreCLR)에서
 * 실행됩니다. 엔진과 C#은 같은 주소 공간에 있으며, 서로의 함수 포인터를 직접 호출합니다.
 *
 * @verbatim
 *   Macaw.exe
 *    +- UScriptComponent -> FScriptRuntime (C++)
 *    |                          |  함수 포인터 테이블 (FNativeApi / FManagedApi)
 *    |                          v
 *    +- CoreCLR
 *         +- 기본 로드 컨텍스트:  Macaw.Engine.dll   (Host, Log, Actor, ScriptComponent), 프로세스 수명 동안 유지
 *         +- 수집형 로드 컨텍스트: GameScripts.dll    (ScriptComponent 파생 클래스), 실행 세션마다 로드와 언로드
 * @endverbatim
 *
 * @par 파일 배치
 * 빌드 후 단계의 Scripts/StageManagedRuntime.ps1이 Macaw.Engine을 빌드하고 실행 파일 옆에 다음을 배치합니다.
 * .NET이 설치되지 않은 PC에서도 동작하도록 시스템 설치본을 찾지 않고 이 사본만 사용합니다.
 * 사용자 스크립트는 프로젝트의 Scripts/bin/GameScripts.dll에서 읽습니다.
 * @verbatim
 *   <실행 파일 폴더>
 *    +- dotnet/host/fxr/<버전>/hostfxr.dll                  런타임을 찾아 기동하는 호스트 라이브러리
 *    +- dotnet/shared/Microsoft.NETCore.App/<버전>/         CoreCLR과 기본 클래스 라이브러리
 *    +- Managed/Macaw.Engine.dll                            엔진 쪽 C# 어셈블리
 *    +- Managed/Macaw.Engine.runtimeconfig.json             사용할 런타임 버전과 GC 설정
 * @endverbatim
 *
 * @par 실행 세션
 * PIE와 게임은 BeginPlay 전에 BeginSession을, 월드를 파괴한 뒤 EndSession을 호출합니다.
 * 세션을 시작해도 바로 아무것도 로드하지 않습니다. 첫 UScriptComponent가 BeginPlay에서 인스턴스를 요청할 때
 * CoreCLR 기동과 스크립트 로드를 수행하므로, 스크립트를 쓰지 않는 장면은 비용을 치르지 않습니다.
 * 세션이 끝나면 스크립트를 언로드하므로, 다시 빌드한 DLL은 다음 세션에 반영됩니다.
 * 실행 중에는 리로드하지 않습니다.
 *
 * @par 기동
 * CoreCLR은 프로세스당 한 번만 기동할 수 있어 이 클래스는 단일 인스턴스입니다.
 * -# hostfxr.dll을 LoadLibraryW로 열고 호스팅 함수를 GetProcAddress로 얻습니다. 링크 시점 의존성은 없습니다.
 * -# hostfxr_initialize_for_runtime_config에 사본 경로를 dotnet_root로 넘겨 런타임을 초기화합니다.
 * -# hdt_load_assembly로 Macaw.Engine을 기본 로드 컨텍스트에 올립니다.
 *    hdt_load_assembly_and_get_function_pointer는 격리된 컨텍스트에 올리므로 쓰지 않습니다.
 *    기본 컨텍스트에 있어야 스크립트와 ScriptComponent 타입을 공유할 수 있습니다.
 * -# hdt_get_function_pointer로 Macaw.Host.Bootstrap의 주소를 얻어 호출합니다.
 *
 * @par 경계 호출 (FNativeApi / FManagedApi)
 * 양쪽 호출은 Bootstrap에서 교환한 함수 포인터 테이블만 사용합니다.
 * - FNativeApi는 C++이 채우는 C# -> C++ 함수 목록입니다. Log, 객체 생존 확인, 클래스 목록 수집과 생성된 바인딩 테이블을 담습니다.
 * - FManagedApi는 C#이 채우는 C++ -> C# 함수 목록입니다. 대상은 Host의 [UnmanagedCallersOnly] 메서드입니다.
 * - 두 구조체는 Source/Managed/Macaw.Engine/Interop.cs의 NativeApi, ManagedApi와 멤버 순서가 같아야 합니다.
 *   Bootstrap은 크기만 비교하므로, 크기가 같고 순서만 다른 불일치는 잡지 못합니다.
 * - 문자열은 UTF-8 const char*로 전달하며, 수신 측이 호출 중에 필요한 만큼 복사합니다.
 * - 엔진 수학 타입은 alignas(16)이라 직접 넘기지 않고, ScriptAbi.h의 정렬 없는 타입으로 변환해 전달합니다.
 * - C# 객체는 GC가 옮길 수 있으므로 GCHandle을 정수로 바꿔 C++에 전달합니다.
 * - 엔진 객체는 포인터 대신 FObjectHandle로 C#에 전달합니다. 파괴된 객체의 핸들은 해석에 실패하므로,
 *   C#에서는 크래시 대신 예외가 발생합니다.
 *
 * @par 스크립트 로드와 언로드
 * -# 출력 폴더 전체를 <DLL 폴더>/.Loaded/<프로세스 ID>_<세대>/로 복사합니다. 복사본을 로드하므로 에디터가
 *    켜진 상태에서도 원본을 다시 빌드할 수 있습니다.
 * -# 복사본을 새 수집형 컨텍스트(ScriptLoadContext)에 로드합니다. NuGet 의존성은 deps.json을 읽는
 *    AssemblyDependencyResolver로 찾습니다. Macaw.Engine과 프레임워크는 기본 컨텍스트로 해석되므로,
 *    스크립트 클래스를 ScriptComponent로 다룰 수 있습니다.
 * -# 언로드할 때는 컨텍스트에 Unload를 요청한 뒤, WeakReference가 끊어질 때까지 GC를 최대 10회 수행합니다.
 *    static 이벤트나 스레드가 스크립트 객체를 붙잡고 있으면 수집되지 않습니다. 이때는 경고만 남기며,
 *    옛 코드와 복사본 폴더는 메모리와 디스크에 남습니다.
 *
 * @par 인스턴스 수명과 예외
 * UScriptComponent가 다음 순서로 호출합니다. 살아 있는 인스턴스의 GCHandle은 언로드를 막으므로,
 * 세션을 끝내기 전에 월드를 파괴해 인스턴스를 모두 해제해야 합니다.
 * -# CreateInstance: 클래스 이름으로 타입을 찾아 ScriptComponent 파생인지 확인하고, 인스턴스를 만들어 GCHandle을 반환합니다.
 * -# BeginPlayInstance, TickInstance, EndPlayInstance: 핸들에서 인스턴스를 꺼내 C# 가상 함수를 호출합니다.
 * -# DestroyInstance: GCHandle을 해제합니다. 이후 인스턴스는 GC의 수집 대상이 됩니다.
 * .
 * C# 예외가 네이티브 프레임으로 넘어가면 프로세스가 종료되므로, Host의 진입점에서 모두 잡습니다.
 * 예외가 발생한 인스턴스는 로그를 남긴 뒤 비활성화되며, 이후 호출은 실행되지 않습니다.
 *
 * @par 필드와 클래스 목록
 * [Field]를 붙인 필드는 Details 패널에서 편집하고 씬에 저장합니다.
 * RefreshCatalog는 편집 중에 스크립트를 잠시 로드해 클래스와 필드를 읽은 뒤 바로 언로드합니다.
 * 기본값은 임시 인스턴스에서 읽으므로, 이때도 생성자는 엔진 API를 쓸 수 없습니다.
 * 씬에는 기본값과 달라진 필드만 FScriptField로 저장하고, CreateInstance 후 BeginPlay 전에 넣습니다.
 *
 * @par 바인딩 생성
 * 멤버 함수 선언 앞에 UFUNCTION()을 쓰면 MacawHeaderTool이 C++ 썽크(ScriptBindings.cpp)와
 * C# 래퍼(ScriptBindings.g.cs)를 만듭니다. 썽크는 핸들을 객체로 해석하고, 객체가 없으면 0을 반환해 C#에서 예외가 발생합니다.
 * 함수 순서는 생성기가 양쪽에 같게 만들고, 시그니처 해시를 Bootstrap에서 비교합니다.
 * 프로퍼티는 FScriptInterop이 리플렉션으로 읽고 씁니다.
 *
 * @par 현재 범위
 * 충돌은 UShapeComponent의 겹침 시작과 끝만 전달합니다. 엔진에 막는 물리가 없으므로 Hit 이벤트는 없습니다.
 * 내보낸 게임(MacawGame)에는 아직 런타임을 배치하지 않으므로, 그곳의 스크립트는 오류 로그만 남기고 실행되지 않습니다.
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/FObjectHandle.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>

namespace Script {

    struct FScriptField;

    /**
     * @brief 스크립트의 [Field] 필드 하나의 정보입니다.
     */
    struct FScriptFieldInfo {
        /**
         * @brief C# 필드 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 타입 태그이며 Bool, Int, Float, String, Vector3 중 하나입니다.
         */
        Core::FString mType{};

        /**
         * @brief 임시 인스턴스에서 읽은 기본값입니다.
         */
        Core::FString mDefault{};
    };

    /**
     * @brief 생성할 수 있는 스크립트 클래스 하나의 정보입니다.
     */
    struct FScriptClassInfo {
        /**
         * @brief 네임스페이스를 포함한 클래스 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 파생 클래스 필드부터 나열한 [Field] 목록입니다.
         */
        Core::TArray<Script::FScriptFieldInfo> mFields{};
    };

    /**
     * @brief 실행 파일 옆에 배치된 .NET 런타임을 기동하고 C# 스크립트와 인스턴스의 수명을 담당합니다.
     * @note CoreCLR은 프로세스당 한 번만 기동할 수 있고 종료 후 다시 기동할 수 없으므로 단일 인스턴스로 관리합니다.
     */
    class FScriptRuntime final {
    public:
        /**
         * @brief C# 로그 메시지를 전달받는 함수 타입입니다.
         */
        using FLogFunction = std::function<void(const Core::FString&)>;

    private:
        /**
         * @brief 프로세스 수명 동안 유지되는 런타임 상태를 구성합니다.
         */
        FScriptRuntime();

        /**
         * @brief 런타임 상태를 정리합니다.
         */
        ~FScriptRuntime();

        /**
         * @brief 소유 상태의 중복을 막기 위해 복사를 금지합니다.
         */
        FScriptRuntime(const Script::FScriptRuntime&) = delete;

        /**
         * @brief 소유 상태의 중복을 막기 위해 복사를 금지합니다.
         */
        Script::FScriptRuntime& operator=(const Script::FScriptRuntime&) = delete;

    public:
        /**
         * @brief 프로세스에 하나뿐인 런타임을 반환합니다.
         * @return 런타임의 비소유 주소입니다.
         */
        static Script::FScriptRuntime* Get();

        /**
         * @brief C# 로그 메시지를 받을 콜백을 설정합니다.
         * @param[in] Function 메시지를 받을 콜백이며, 비어 있으면 디버그 출력으로 보냅니다.
         */
        void SetLogFunction(FLogFunction Function);

        /**
         * @brief 실행 세션을 시작하고 프로젝트의 스크립트 어셈블리 경로를 정합니다.
         * @param[in] ProjectRoot 프로젝트 루트 폴더입니다.
         * @note 런타임 기동과 로드는 첫 CreateInstance까지 미룹니다.
         */
        void BeginSession(const std::filesystem::path& ProjectRoot);

        /**
         * @brief 실행 세션을 끝내고 스크립트를 언로드합니다.
         * @note 세션이 없으면 아무 일도 하지 않습니다. 모든 인스턴스를 해제한 뒤 호출해야 합니다.
         */
        void EndSession();

        /**
         * @brief 스크립트 클래스의 인스턴스를 만듭니다.
         * @param[in] ClassName 네임스페이스를 포함한 클래스 이름입니다.
         * @param[in] Owner 인스턴스를 소유한 액터의 핸들입니다.
         * @return 인스턴스 핸들이며, 실패하면 0입니다. 실패 원인은 로그로 보고합니다.
         */
        std::intptr_t CreateInstance(const Core::FString& ClassName, CoreUObject::FObjectHandle Owner);

        /**
         * @brief 인스턴스의 BeginPlay를 호출합니다.
         * @param[in] Instance CreateInstance가 반환한 유효한 핸들입니다.
         */
        void BeginPlayInstance(std::intptr_t Instance);

        /**
         * @brief 인스턴스의 Tick을 호출합니다.
         * @param[in] Instance CreateInstance가 반환한 유효한 핸들입니다.
         * @param[in] DeltaTime 이번 갱신의 초 단위 시간입니다.
         */
        void TickInstance(std::intptr_t Instance, float DeltaTime);

        /**
         * @brief 인스턴스의 EndPlay를 호출합니다.
         * @param[in] Instance CreateInstance가 반환한 유효한 핸들입니다.
         */
        void EndPlayInstance(std::intptr_t Instance);

        /**
         * @brief 인스턴스 핸들을 해제합니다.
         * @param[in] Instance CreateInstance가 반환한 유효한 핸들입니다.
         */
        void DestroyInstance(std::intptr_t Instance);

        /**
         * @brief 겹침 시작이나 끝을 C# 인스턴스에 전달합니다.
         * @param[in] Instance CreateInstance가 반환한 값입니다.
         * @param[in] Begin 겹침 시작이면 true, 끝이면 false입니다.
         * @param[in] Other 상대 액터이며, 파괴되어 없으면 유효하지 않은 핸들입니다.
         */
        void OverlapInstance(std::intptr_t Instance, bool Begin, CoreUObject::FObjectHandle Other);

        /**
         * @brief 씬에 저장된 값을 인스턴스의 필드에 넣습니다.
         * @param[in] Instance CreateInstance가 반환한 유효한 핸들입니다.
         * @param[in] Field 넣을 필드입니다. 스크립트와 맞지 않으면 경고만 남기고 건너뜁니다.
         */
        void SetInstanceField(std::intptr_t Instance, const Script::FScriptField& Field);

        /**
         * @brief 프로젝트의 스크립트를 잠시 로드해 클래스와 필드 목록을 다시 읽습니다.
         * @param[in] ProjectRoot 프로젝트 루트 폴더입니다.
         * @return 목록을 다시 읽었으면 true, 실행 세션 중이거나 로드에 실패하면 false입니다.
         * @note 어셈블리가 없으면 CoreCLR을 기동하지 않고 목록만 비웁니다.
         */
        bool RefreshCatalog(const std::filesystem::path& ProjectRoot);

        /**
         * @brief 마지막으로 읽은 스크립트 클래스 목록을 반환합니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        const Core::TArray<Script::FScriptClassInfo>& GetCatalog() const;

        /**
         * @brief 목록에서 스크립트 클래스를 찾습니다.
         * @param[in] ClassName 네임스페이스를 포함한 클래스 이름입니다.
         * @return 클래스 정보의 비소유 주소이며, 없으면 nullptr입니다.
         */
        const Script::FScriptClassInfo* FindClass(const Core::FString& ClassName) const;

        /**
         * @brief 프로젝트의 스크립트 어셈블리 경로를 반환합니다.
         * @param[in] ProjectRoot 프로젝트 루트 폴더입니다.
         * @return 어셈블리 경로입니다.
         */
        static std::filesystem::path GetAssemblyPath(const std::filesystem::path& ProjectRoot);

        /**
         * @brief 등록된 로그 콜백에 메시지를 전달합니다.
         * @param[in] Message 전달할 메시지입니다.
         */
        void WriteLog(const Core::FString& Message) const;

    private:
        /**
         * @brief 실행 파일 옆의 dotnet 폴더에서 CoreCLR을 기동하고 Macaw.Engine을 로드합니다.
         * @param[out] Error 실패 원인입니다.
         * @return 이미 기동되었거나 기동에 성공하면 true, 실패하면 false입니다.
         */
        bool Initialize(Core::FString& Error);

        /**
         * @brief 세션의 스크립트 어셈블리를 로드합니다.
         * @return 스크립트가 로드되어 있으면 true입니다.
         * @note 실패하면 한 번만 로그를 남기고, 세션이 끝날 때까지 다시 시도하지 않습니다.
         */
        bool EnsureScripts();

    private:
        /**
         * @brief hostfxr와 C# 사이의 함수 테이블을 담는 내부 상태입니다.
         */
        struct FState;

        /**
         * @brief 기동에 성공한 뒤 유효한 내부 상태입니다.
         */
        std::unique_ptr<FState> mState{};

        /**
         * @brief C# 메시지를 받는 선택적 콜백입니다.
         */
        FLogFunction mLogFunction{};

        /**
         * @brief 현재 세션의 스크립트 어셈블리 경로이며, 세션이 없으면 비어 있습니다.
         */
        std::filesystem::path mAssembly{};

        /**
         * @brief 마지막으로 읽은 스크립트 클래스 목록입니다.
         */
        Core::TArray<Script::FScriptClassInfo> mCatalog{};

        /**
         * @brief 스크립트 어셈블리가 로드되어 있는지 나타냅니다.
         */
        bool mHasScripts{};

        /**
         * @brief 현재 세션에서 로드에 실패했는지 나타냅니다.
         */
        bool mLoadFailed{};
    };

}
