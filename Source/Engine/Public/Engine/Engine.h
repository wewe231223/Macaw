/**
 * @file Engine.h
 * @brief 현재 월드 컨텍스트와 렌더러 연결을 관리합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/UObject.h"

class UWorld;
class FWorldContext;
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
     * @fn UClass* UEngine::GetClass() const
     * @brief 객체의 실제 클래스 메타데이터를 반환합니다.
     * @return 실제 클래스 메타데이터의 비소유 주소입니다.
     */

    /**
     * @brief 렌더러를 연결하고 초기 월드 컨텍스트와 장면을 준비합니다.
     * @param[in,out] Renderer 뷰포트 및 엔진이 참조할 렌더러입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    virtual bool Initialize(IRendererModule& Renderer);
    /**
     * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
     */
    virtual void Shutdown();

    /**
     * @brief 현재 소속되거나 관리하는 월드의 비소유 주소를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    UWorld* GetWorld() const;
    /**
     * @brief 레지스트리와 월드를 함께 소유한 현재 컨텍스트를 반환합니다.
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

private:
    /**
     * @brief 레지스트리와 월드를 함께 소유한 현재 컨텍스트입니다.
     */
    std::unique_ptr<FWorldContext> mWorldContext{};
    ReflectProperty(mWorldContext, "WorldContext", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
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
};
