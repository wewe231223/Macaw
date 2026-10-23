/**
 * @file Engine.h
 * @brief 현재 월드 컨텍스트와 렌더러 연결을 관리합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/UObject.h"
#include "Engine/EWorldType.h"
#include "Engine/FWorldHandle.h"

class UWorld;
class FWorldContext;
class FObjectDuplicationContext;
class IRendererModule;

/**
 * @brief 현재 월드 컨텍스트와 렌더러 연결 및 월드 교체를 관리합니다.
 * @ingroup Engine
 */
class UEngine : public UObject {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(UEngine, UObject);

public:
    /**
     * @brief UEngine의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    UEngine();
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~UEngine() override;

public:
    /**
     * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectClass(UEngine, EClassFlags::NoFactory);
    /**
     * @brief 렌더러 없이 초기 편집 월드와 리플렉션 기반을 준비합니다.
     * @return 초기 월드 준비에 성공하면 true이며 이미 초기화되었거나 콜백 처리 중이면 false입니다.
     */
    bool Initialize();
    /**
     * @fn UClass* UEngine::GetClass() const
     * @brief 객체의 실제 클래스 메타데이터를 반환합니다.
     * @return 실제 클래스 메타데이터의 비소유 주소입니다.
     */
    /**
     * @brief 렌더러를 연결하고 초기 월드 컨텍스트와 장면을 준비합니다.
     * @param[in,out] Renderer 뷰포트 및 엔진이 참조할 렌더러입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    virtual bool Initialize(IRendererModule* Renderer);
    /**
     * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
     */
    virtual void Shutdown();
    /**
     * @brief 등록된 월드의 핸들 스냅샷을 순회하여 실행을 갱신합니다.
     * @param[in] DeltaTime 진행할 초 단위 시간이며 유한한 0 이상의 값이어야 합니다.
     * @note 실행 중인 월드만 일반 Tick을 수행합니다. 콜백에서 요청한 엔진 종료는 순회를 마친 뒤 처리합니다.
     */
    void Tick(float DeltaTime);
    /**
     * @brief 지정한 용도의 월드 컨텍스트를 생성하여 엔진이 소유합니다.
     * @param[in] Type 새 월드의 편집 또는 실행 용도입니다.
     * @return 등록된 월드 핸들이며 생성할 수 없으면 무효 핸들입니다.
     */
    FWorldHandle CreateWorld(EWorldType Type);
    /**
     * @brief 핸들을 검증하고 해당 월드의 실행과 소유 리소스를 종료합니다.
     * @param[in] Handle 제거할 월드의 슬롯과 세대입니다.
     * @return 월드를 제거하면 true입니다.
     * @note 갱신 또는 월드 콜백 처리 중에는 제거를 거부합니다. 현재 월드를 제거하면 현재 월드 선택도 해제합니다.
     */
    bool DestroyWorld(FWorldHandle Handle);
    /**
     * @brief 원본과 독립된 정지 상태의 월드를 복제하여 엔진에 등록합니다.
     * @param[in] Source 이 엔진이 소유한 원본 월드 핸들입니다.
     * @param[in] Type 복제 월드의 용도입니다.
     * @param[out] Error 실패 원인이며 성공하면 비워집니다.
     * @param[out] Duplication 선택적인 비소유 대응표이며 실패하면 비워집니다.
     * @return 새 월드 핸들이며 복제나 장면 할당이 실패하면 무효 핸들입니다.
     * @note 현재 월드 선택은 바꾸지 않습니다. 대응표는 원본과 복제본이 살아 있는 동안만 사용할 수 있습니다.
     */
    FWorldHandle DuplicateWorld(FWorldHandle Source, EWorldType Type, FString& Error, FObjectDuplicationContext* Duplication = nullptr);
    /**
     * @brief 슬롯과 세대를 검증하여 엔진 소유 월드를 조회합니다.
     * @param[in] Handle 조회할 월드 핸들입니다.
     * @return 살아 있는 월드의 비소유 주소이며 오래되었거나 다른 엔진의 핸들이면 nullptr입니다.
     */
    UWorld* GetWorld(FWorldHandle Handle) const;
    /**
     * @brief 현재 등록된 모든 월드의 핸들 스냅샷을 반환합니다.
     * @return 비어 있는 슬롯을 제외한 월드 핸들 목록입니다.
     */
    TArray<FWorldHandle> GetWorldHandles() const;
    /**
     * @brief 월드 경계를 검증한 뒤 해당 월드에 액터 삭제를 요청합니다.
     * @param[in] Handle 삭제할 액터의 전역 객체 핸들입니다.
     * @return 액터 삭제 요청을 수락하면 true입니다.
     * @note 월드 콜백 중 실제 해제는 가장 바깥 전달 범위가 끝날 때까지 지연됩니다.
     */
    bool DestroyActor(FObjectHandle Handle);
    /**
     * @brief 기존 편집 경로에서 사용하는 현재 월드 선택을 반환합니다.
     * @return 현재 월드 핸들이며 선택되지 않았다면 무효 핸들입니다.
     */
    FWorldHandle GetWorldHandle() const;
    /**
     * @brief 이 엔진이 소유한 월드를 현재 월드로 선택합니다.
     * @param[in] Handle 선택할 살아 있는 월드 핸들입니다.
     * @return 유효한 월드를 선택하면 true입니다.
     */
    bool SetCurrentWorld(FWorldHandle Handle);
    /**
     * @brief 현재 소속되거나 관리하는 월드의 비소유 주소를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    UWorld* GetWorld() const;
    /**
     * @brief 현재 월드를 소유한 컨텍스트를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    FWorldContext* GetWorldContext() const;
    /**
     * @brief 연결된 렌더러의 비소유 주소를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    IRendererModule* GetRenderer() const;
    /**
     * @brief 새 월드 컨텍스트를 검증하고 장면을 연결한 뒤 현재 월드를 교체합니다.
     * @param[in] Context 월드 컨텍스트 또는 Direct3D 11 컨텍스트입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool ReplaceWorld(std::unique_ptr<FWorldContext> Context);
    /**
     * @brief 월드 교체 시 증가하는 컨텍스트 세대를 반환합니다.
     * @return 월드 교체 시 증가하는 컨텍스트 세대입니다.
     */
    Uint64 GetWorldGeneration() const;
    /**
     * @brief 월드의 프레임 말 렌더 갱신을 모두 반영합니다.
     */
    void SynchronizeWorld();

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();
    /**
     * @brief 슬롯 범위와 세대가 일치하는 소유 컨텍스트를 찾습니다.
     * @param[in] Handle 검증할 월드 핸들입니다.
     * @return 컨텍스트의 비소유 주소이며 검증에 실패하면 nullptr입니다.
     */
    FWorldContext* FindContext(FWorldHandle Handle) const;
    /**
     * @brief 필요한 렌더 장면을 준비하고 컨텍스트를 빈 슬롯에 인수합니다.
     * @param[in] Context 엔진이 단독 소유권을 넘겨받을 컨텍스트입니다.
     * @return 새 세대를 발급한 월드 핸들이며 검증이나 장면 준비 실패 시 무효 핸들입니다.
     */
    FWorldHandle AdoptWorld(std::unique_ptr<FWorldContext> Context);
    /**
     * @brief 엔진 갱신 또는 소유 월드의 콜백 처리가 진행 중인지 검사합니다.
     * @return 안전한 월드 제거 경계가 아니면 true입니다.
     */
    bool IsDispatching() const;

private:
    /**
     * @brief 월드를 각각 소유하는 컨텍스트 슬롯입니다.
     * @details 복수 컨텍스트를 슬롯별로 소유하며 현재 편집 대상은 mCurrentWorld 핸들로 선택합니다.
     */
    TArray<std::unique_ptr<FWorldContext>> mWorldContexts{};
    ReflectProperty(mWorldContexts, "WorldContexts", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 기존 편집 경로가 참조하는 현재 월드의 비소유 핸들입니다.
     */
    FWorldHandle mCurrentWorld{};
    ReflectProperty(mCurrentWorld, "CurrentWorld", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 연결된 렌더러의 비소유 주소입니다.
     */
    IRendererModule* mRenderer{};
    ReflectProperty(mRenderer, "Renderer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 월드 교체 시 증가하는 컨텍스트 세대입니다.
     */
    Uint64 mWorldGeneration{};
    ReflectProperty(mWorldGeneration, "WorldGeneration", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 초기화가 완료되어 월드를 생성할 수 있는지 나타냅니다.
     */
    bool mInitialized{};
    ReflectProperty(mInitialized, "Initialized", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 갱신이나 복제 및 정리 도중 엔진 변경의 재진입을 막습니다.
     */
    bool mUpdating{};
    ReflectProperty(mUpdating, "Updating", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 안전한 갱신 경계에서 처리할 엔진 종료 요청입니다.
     */
    bool mShutdownRequested{};
    ReflectProperty(mShutdownRequested, "ShutdownRequested", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
