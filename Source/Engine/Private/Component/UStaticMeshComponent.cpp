/**
 * @file UStaticMeshComponent.cpp
 * @brief Plane, Cube 또는 Sphere 프리미티브를 표시하는 메시 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UStaticMeshComponent.h"
#include "Core/Reflection/EPropertyFlags.h"
#include "Engine/EPrimitiveType.h"

/**
 * @brief UStaticMeshComponent의 초기 상태와 전달받은 의존성을 구성합니다.
 */
UStaticMeshComponent::UStaticMeshComponent() = default;

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
UStaticMeshComponent::~UStaticMeshComponent() = default;

/**
 * @brief 표시할 프리미티브 메시 종류를 반환합니다.
 */
EPrimitiveType UStaticMeshComponent::GetPrimitiveType() const {
    return mPrimitive;
}

/**
 * @brief 지원되는 프리미티브 종류를 검증하고 렌더 상태 갱신을 예약합니다.
 */
bool UStaticMeshComponent::SetPrimitiveType(EPrimitiveType Type) {
    if (Type >= EPrimitiveType::Count) {
        return false;
    }

    if (mPrimitive != Type) {
        mPrimitive = Type;
        MarkRenderStateDirty();
    }

    return true;
}
