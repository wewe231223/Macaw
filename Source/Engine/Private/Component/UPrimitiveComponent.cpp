/**
 * @file UPrimitiveComponent.cpp
 * @brief 장면 프록시 생성과 지연된 렌더 상태 및 변환 갱신을 제공하는 기반 클래스입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/UWorld.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FTransform.h"
#include "Core/Math/FBox.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/FSceneInterface.h"
#include "Engine/PrimitiveMesh.h"

/**
 * @brief UPrimitiveComponent의 초기 상태와 전달받은 의존성을 구성합니다.
 */
UPrimitiveComponent::UPrimitiveComponent() = default;

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
UPrimitiveComponent::~UPrimitiveComponent() {
    UnregisterComponent();
}

/**
 * @brief 컴포넌트를 월드에 연결하고 필요한 렌더 상태를 등록합니다.
 */
void UPrimitiveComponent::RegisterComponent(UWorld& World) {
    if (GetWorld() == &World) {
        return;
    }

    UnregisterComponent();
    UActorComponent::RegisterComponent(World);
    MarkRenderStateDirty();
}

/**
 * @brief 등록된 렌더 상태와 월드 연결을 해제합니다.
 */
void UPrimitiveComponent::UnregisterComponent() {
    if (mRenderStateCreated && GetWorld() != nullptr && GetWorld()->GetScene() != nullptr) {
        GetWorld()->GetScene()->RemovePrimitive(GetGuid());
    }

    mRenderStateCreated = false;
    mQueued = false;
    mRenderStateDirty = true;
    mRenderTransformDirty = false;

    UActorComponent::UnregisterComponent();
}

/**
 * @brief 유한하고 유효한 변환을 검증하여 공간 컴포넌트에 적용합니다.
 */
bool UPrimitiveComponent::SetTransform(const FTransform& Transform) {
    if (!USceneComponent::SetTransform(Transform)) {
        return false;
    }

    MarkRenderTransformDirty();

    return true;
}

/**
 * @brief 프리미티브 프록시 재생성이 필요함을 표시하고 갱신을 예약합니다.
 */
void UPrimitiveComponent::MarkRenderStateDirty() {
    mRenderStateDirty = true;
    QueueUpdate();
}

/**
 * @brief 변환 업로드가 필요함을 표시하고 프레임 말 갱신을 예약합니다.
 */
void UPrimitiveComponent::MarkRenderTransformDirty() {
    mRenderTransformDirty = true;
    QueueUpdate();
}

/**
 * @brief 같은 컴포넌트를 중복 예약하지 않고 월드의 갱신 큐에 넣습니다.
 */
void UPrimitiveComponent::QueueUpdate() {
    if (!mQueued && GetWorld() != nullptr) {
        mQueued = true;
        GetWorld()->EnqueueRenderUpdate(GetHandle());
    }
}

/**
 * @brief 누적된 렌더 상태 또는 변환 변경을 연결된 장면에 반영합니다.
 */
void UPrimitiveComponent::SendRenderUpdates() {
    mQueued = false;

    if (GetWorld() == nullptr || GetWorld()->GetScene() == nullptr) {
        return;
    }

    FSceneInterface& Scene{*GetWorld()->GetScene()};

    if (mRenderStateDirty || !mRenderStateCreated) {
        Scene.AddPrimitive(CreateSceneProxy());
        mRenderStateCreated = true;
    } else if (mRenderTransformDirty) {
        const FPrimitiveSceneProxy Proxy{CreateSceneProxy()};

        Scene.UpdatePrimitiveTransform(Proxy.mId, Proxy.mWorld, Proxy.mBounds);
    }

    mRenderStateDirty = false;
    mRenderTransformDirty = false;
}

/**
 * @brief 컴포넌트의 현재 상태를 복사한 프리미티브 프록시를 만듭니다.
 */
FPrimitiveSceneProxy UPrimitiveComponent::CreateSceneProxy() const {
    const FMatrix World{GetWorldMatrix()};
    return {GetGuid(), GetPrimitiveType(), World, GetPrimitiveMesh(GetPrimitiveType()).mBounds.Transformed(World)};
}

/**
 * @brief 월드 공간의 축 정렬 경계 상자를 반환합니다.
 */
FBox UPrimitiveComponent::GetWorldBounds() const {
    return GetPrimitiveMesh(GetPrimitiveType()).mBounds.Transformed(GetWorldMatrix());
}
