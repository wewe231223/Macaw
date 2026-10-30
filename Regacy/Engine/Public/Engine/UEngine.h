/**
 * @file UEngine.h
 * @brief 엔진 소유권, 실행 수명과 GetEngine 전역 접근점을 정의합니다.
 */

#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include <memory>
#include <vector>
#include "CoreUObject/TSubsystemCollection.h"
#include "Engine/FWorldContext.h"
#include "Engine/Subsystem/UEngineSubsystem.h"
#include "World/Tick/ETickType.h"
#include <Windows.h>

class FAssetRegistry;
class FRenderer;

/** @brief 엔진 공용 서비스와 여러 월드를 소유하고 활성 엔진의 전역 접근 수명을 관리합니다. @details 동시에 하나의 엔진만 활성화할 수 있으며 Initialize와 Shutdown은 외부에서 직렬화해야 합니다. */
class UEngine : public UObject {
public:
    /** @brief UObject 기반의 리플렉션 타입 별칭입니다. */
    ReflectTypes(UEngine, UObject);

public:
    /** @brief 전역으로 등록하지 않은 엔진 객체를 생성합니다. */
    UEngine();

    /** @brief 월드와 공용 서비스를 종료하고 이 엔진의 전역 접근점을 해제합니다. */
    ~UEngine() override;

public:
    /** @brief 엔진 클래스의 리플렉션 정보를 반환합니다. */
    ReflectClass(UEngine);

    /** @brief 객체 시스템과 공용 서비스를 초기화하고 GetEngine 및 FAssetRegistry 전역 접근점을 등록합니다. @note 이미 초기화되었으면 무시하며 다른 활성 엔진이 있으면 Critical 오류를 보고합니다. 에셋 파일 로딩은 별도로 수행합니다. */
    virtual void Initialize();

    /** @brief 월드, 서브시스템, 에셋의 순서로 해제한 뒤 전역 엔진 접근점을 비웁니다. @note 월드와 엔진 서브시스템 종료 콜백 동안 엔진과 에셋 접근을 유지하며 중복 종료와 종료 재진입은 안전합니다. Tick 중에는 Critical 오류를 보고합니다. */
    virtual void Shutdown();

    /** @brief 초기화가 완료되어 정상 실행 가능한지 반환합니다. @return 종료가 시작되었거나 초기화 전이면 false입니다. */
    bool IsInitialized() const;

    /** @brief 각 월드의 시간 상태에 맞춰 한 프레임을 진행합니다. @param DeltaTime 프레임 경과 시간(초)입니다. */
    virtual void Tick(float DeltaTime);

    /** @brief 현재 월드 Tick을 처리 중인지 반환합니다. */
    bool IsTicking() const;

    /** @brief Tick 또는 월드 콜백에 의한 컨텍스트 변경 제한이 없는지 반환합니다. */
    bool CanChangeWorldContexts() const;

    /** @brief 월드의 일시 정지 상태에 맞는 Tick 종류를 반환합니다. @param Context 대상 월드 컨텍스트입니다. */
    virtual ETickType GetWorldTickType(const FWorldContext& Context) const;

    /** @brief 공용 에셋 레지스트리를 사용하는 새 월드 컨텍스트를 생성합니다. @param WorldType 생성할 월드 종류입니다. @return 엔진이 소유하는 컨텍스트의 참조입니다. @pre 엔진이 초기화되어 있어야 합니다. */
    FWorldContext& CreateWorldContext(EWorldType WorldType);

    /** @brief 초기화된 월드의 소유권을 엔진으로 이전합니다. @param World 성공하면 비워지는 월드 소유 포인터입니다. @return 생성된 컨텍스트이며 이전이 불가능하면 nullptr입니다. */
    FWorldContext* AdoptWorld(std::unique_ptr<UWorld>& World);

    /** @brief 이 엔진이 소유한 월드 컨텍스트를 제거합니다. @param Context 제거할 컨텍스트입니다. @return 소유권과 실행 상태 검사를 통과해 제거했으면 true입니다. */
    virtual bool DestroyWorldContext(FWorldContext& Context);

    /** @brief 엔진이 소유한 전체 월드 컨텍스트 목록을 읽기 전용으로 반환합니다. */
    const std::vector<std::unique_ptr<FWorldContext>>& GetWorldContexts() const;

    /** @brief 엔진 수명에 귀속된 서브시스템 컬렉션을 반환합니다. */
    TSubsystemCollection<UEngineSubsystem, UEngine>& GetSubsystems();

    bool CreateRenderer(HWND WindowHandle, UINT Width, UINT Height);
    FRenderer* GetRenderer() const;

private:
    /** @brief 엔진 실행에 필요한 모듈의 리플렉션 타입을 등록합니다. */
    void RegisterObjectTypes();

private:
    std::unique_ptr<FRenderer> mRenderer{};

    /** @brief 이 엔진이 소유하며 활성화 시 전역으로 공개되는 공용 에셋 저장소입니다. */
    std::unique_ptr<FAssetRegistry> mAssetRegistry{};

    /** @brief 엔진에 귀속된 서브시스템의 소유 컬렉션입니다. */
    TSubsystemCollection<UEngineSubsystem, UEngine> mSubsystems{};

    /** @brief 에디터와 게임 등 독립된 월드 컨텍스트의 소유 목록입니다. */
    std::vector<std::unique_ptr<FWorldContext>> mWorldContexts{};

    /** @brief 엔진 초기화 완료 여부입니다. 종료를 시작할 때 false로 변경합니다. */
    bool mInitialized{};

    /** @brief 월드 Tick 처리 중 재진입과 컨텍스트 변경을 제한하는 상태입니다. */
    bool mTicking{};

    /** @brief 종료 콜백에서 초기화 또는 종료가 재진입하는 것을 방지하는 상태입니다. */
    bool mShuttingDown{};
};

/**
 * @brief 현재 활성 엔진에 전역으로 접근합니다.
 * @return Initialize가 완료된 엔진의 비소유 포인터입니다. 초기화 전과 종료 후에는 nullptr입니다.
 * @details 엔진 소유권은 호출자에게 이전되지 않습니다. 종료 콜백 중에는 IsInitialized가 false인 엔진이 반환될 수 있습니다.
 * @note 엔진 초기화와 종료 및 이 함수의 호출은 외부에서 동기화해야 합니다. 반환한 포인터를 엔진 종료 후 보관하지 않습니다.
 */
UEngine* GetEngine() noexcept;
