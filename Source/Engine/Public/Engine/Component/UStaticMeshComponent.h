/**
 * @file UStaticMeshComponent.h
 * @brief Plane, Cube 또는 Sphere 프리미티브를 표시하는 메시 컴포넌트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/EPrimitiveType.h"

/**
 * @brief Plane, Cube 또는 Sphere 프리미티브를 표시하는 메시 컴포넌트입니다.
 * @ingroup Engine
 */
class UStaticMeshComponent final : public UPrimitiveComponent {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(UStaticMeshComponent, UPrimitiveComponent);

public:
    /**
     * @brief UStaticMeshComponent의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    UStaticMeshComponent();
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~UStaticMeshComponent() override;

public:
    /**
     * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectClass(UStaticMeshComponent);
    /**
     * @fn UClass* UStaticMeshComponent::GetClass() const
     * @brief 객체의 실제 클래스 메타데이터를 반환합니다.
     * @return 실제 클래스 메타데이터의 비소유 주소입니다.
     */

    /**
     * @brief 표시할 프리미티브 메시 종류를 반환합니다.
     * @return 표시할 프리미티브 메시 종류입니다.
     */
    EPrimitiveType GetPrimitiveType() const override;
    /**
     * @brief 지원되는 프리미티브 종류를 검증하고 렌더 상태 갱신을 예약합니다.
     * @param[in] Type 생성, 조회 또는 표시할 타입입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool SetPrimitiveType(EPrimitiveType Type);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 프리미티브 메시 종류입니다.
     */
    EPrimitiveType mPrimitive{EPrimitiveType::Cube};
    ReflectAccessor(mPrimitive, GetPrimitiveType, SetPrimitiveType, "Primitive", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
