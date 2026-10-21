/**
 * @file UEngineSubsystem.h
 * @brief 엔진 서브시스템의 소유 관계와 실행 수명을 정의합니다.
 */
#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include "CoreUObject/UObject.h"

class UEngine;

class UEngineSubsystem : public UObject {
public:
    ReflectTypes(UEngineSubsystem, UObject);

public:
    UEngineSubsystem();
    ~UEngineSubsystem() override = default;

public:
    ReflectClass(UEngineSubsystem, EClassFlags::Abstract);

    /**
     * @brief 소유 엔진과 Outer의 일치를 검사하고 서브시스템을 초기화합니다.
     * @param Engine 컬렉션이 전달하는 실제 소유 엔진입니다.
     * @details 이 인자는 소유 관계 검증에만 사용하며 엔진 포인터를 별도 멤버로 보관하지 않습니다.
     */
    void Initialize(UEngine* Engine);
    void Deinitialize();
    /**
     * @brief 초기화된 서브시스템의 Outer에서 실제 소유 엔진을 조회합니다.
     * @return 초기화 전과 종료 후에는 nullptr입니다.
     */
    UEngine* GetEngine() const;
    bool IsInitialized() const;

private:
    bool CanChangeOuter(const UObject* NewOuter) const override;
    virtual void OnInitialize();
    virtual void OnDeinitialize();

private:
    /**
     * @brief 소유 엔진에 연결되어 초기화된 상태인지 나타냅니다.
     */
    bool mInitialized{};
};
