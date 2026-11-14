/**
 * @file UPrimitiveComponent.cpp
 * @brief 장면 프록시 생성과 지연된 렌더 상태 및 변환 갱신을 제공하는 기반 클래스입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UPrimitiveComponent.h"
#include "Asset/UStaticMesh.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/UWorld.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FTransform.h"
#include "Core/Math/FBox.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/FSceneInterface.h"
#include "Engine/PrimitiveMesh.h"
#include "CoreUObject/FObjectRegistry.h"

#include <cmath>

namespace Engine {

    /**
     * @brief UPrimitiveComponent의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Engine::UPrimitiveComponent::UPrimitiveComponent() = default;

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Engine::UPrimitiveComponent::~UPrimitiveComponent() {
        UnregisterComponent();
    }

    /**
     * @brief 컴포넌트를 월드에 연결하고 필요한 렌더 상태를 등록합니다.
     */
    void Engine::UPrimitiveComponent::RegisterComponent(Engine::UWorld* World) {
        Engine::UWorld::FDispatchScope Scope{World};

        if (World == nullptr || GetWorld() == World) {
            return;
        }

        Engine::UActorComponent::RegisterComponent(World);

        if (GetWorld() == World) {
            MarkRenderStateDirty();
        }
    }

    /**
     * @brief 등록된 렌더 상태와 월드 연결을 해제합니다.
     */
    void Engine::UPrimitiveComponent::UnregisterComponent() {
        Engine::UWorld::FDispatchScope Scope{GetWorld()};

        if (mRenderStateCreated && GetWorld() != nullptr && GetWorld()->GetScene() != nullptr) {
            GetWorld()->GetScene()->RemovePrimitive(GetGuid());
        }

        mRenderStateCreated = false;
        mQueued = false;
        mRenderStateDirty = true;
        mRenderTransformDirty = false;

        Engine::UActorComponent::UnregisterComponent();
    }

    /**
     * @brief 유한하고 유효한 변환을 검증하여 공간 컴포넌트에 적용합니다.
     */
    bool Engine::UPrimitiveComponent::SetTransform(const Core::FTransform& Transform) {
        if (!Engine::USceneComponent::SetTransform(Transform)) {
            return false;
        }

        MarkRenderTransformDirty();

        return true;
    }

    /**
     * @brief 프리미티브 프록시 재생성이 필요함을 표시하고 갱신을 예약합니다.
     */
    void Engine::UPrimitiveComponent::OnOwnerVisibilityChanged() {
        MarkRenderStateDirty();
    }

    /**
     * @brief 프리미티브 프록시 재생성이 필요함을 표시하고 갱신을 예약합니다.
     */
    void Engine::UPrimitiveComponent::MarkRenderStateDirty() {
        mRenderStateDirty = true;

        QueueUpdate();
    }

    /**
     * @brief 변환 업로드가 필요함을 표시하고 프레임 말 갱신을 예약합니다.
     */
    void Engine::UPrimitiveComponent::MarkRenderTransformDirty() {
        mRenderTransformDirty = true;

        QueueUpdate();
    }

    /**
     * @brief 같은 컴포넌트를 중복 예약하지 않고 월드의 갱신 큐에 넣습니다.
     */
    void Engine::UPrimitiveComponent::QueueUpdate() {
        if (!mQueued && GetWorld() != nullptr) {
            mQueued = true;

            GetWorld()->EnqueueRenderUpdate(CoreUObject::MakeObjectRef(this));
        }
    }

    /**
     * @brief 누적된 렌더 상태 또는 변환 변경을 연결된 장면에 반영합니다.
     */
    void Engine::UPrimitiveComponent::SendRenderUpdates() {
        mQueued = false;

        if (GetWorld() == nullptr || GetWorld()->GetScene() == nullptr) {
            return;
        }

        if (!IsOwnerVisible() || !ShouldCreateRenderState()) {
            if (mRenderStateCreated) {
                GetWorld()->GetScene()->RemovePrimitive(GetGuid());

                mRenderStateCreated = false;
            }

            mRenderStateDirty = false;
            mRenderTransformDirty = false;

            return;
        }

        Engine::FSceneInterface* Scene{GetWorld()->GetScene()};

        if (mRenderStateDirty || !mRenderStateCreated) {
            Scene->AddPrimitive(CreateSceneProxy());

            mRenderStateCreated = true;
        } else if (mRenderTransformDirty) {
            const Engine::FPrimitiveSceneProxy Proxy{CreateSceneProxy()};

            Scene->UpdatePrimitiveTransform(Proxy.mId, Proxy.mWorld, Proxy.mBounds);
        }

        mRenderStateDirty = false;
        mRenderTransformDirty = false;
    }

    /**
     * @brief 컴포넌트의 현재 상태를 복사한 프리미티브 프록시를 만듭니다.
     */
    Engine::FPrimitiveSceneProxy Engine::UPrimitiveComponent::CreateSceneProxy() const {
        const Core::FMatrix World{GetWorldMatrix()};
        Engine::FPrimitiveSceneProxy Proxy{GetGuid(), GetPrimitiveType(), World, GetWorldBounds(), GetStaticMesh()};
        Proxy.mUVScale = mUVScale;
        Proxy.mUVOffset = mUVOffset;
        Proxy.mCastShadow = mCastShadow;
        Proxy.mReceiveShadow = mReceiveShadow;
        Proxy.mReceivesDecals = mReceivesDecals;

        return Proxy;
    }

    /**
     * @brief 프리미티브가 사용하는 정적 메시의 비소유 주소를 반환합니다.
     */
    Asset::UStaticMesh* Engine::UPrimitiveComponent::GetStaticMesh() const {
        return nullptr;
    }

    /**
     * @brief 메시의 CPU 정점 속성·인덱스 데이터 참조를 반환합니다.
     */
    const Asset::FMeshData& Engine::UPrimitiveComponent::GetMeshData() const {
        return Engine::GetPrimitiveMesh(GetPrimitiveType());
    }

    /**
     * @brief 월드 공간의 축 정렬 경계 상자를 반환합니다.
     */
    Core::FBox Engine::UPrimitiveComponent::GetWorldBounds() const {
        return GetMeshData().GetBounds().Transformed(GetWorldMatrix());
    }

    /**
     * @brief 현재 표시·에셋 상태에서 렌더 프록시를 생성할지 판정합니다.
     */
    bool Engine::UPrimitiveComponent::ShouldCreateRenderState() const {
        return true;
    }

    /**
     * @brief 다른 살아 있는 제어자의 변환을 덮어쓰지 않고 UV 배율과 오프셋을 적용합니다.
     */
    bool Engine::UPrimitiveComponent::SetUVTransform(const Core::FVector2& Scale, const Core::FVector2& Offset, CoreUObject::TObjectRef<CoreUObject::UObject> Source) {
        if (!std::isfinite(Scale.x) || !std::isfinite(Scale.y) || !std::isfinite(Offset.x) || !std::isfinite(Offset.y) || Source.Get() == nullptr || (mUVSource != Source && mUVSource.Get() != nullptr)) {
            return false;
        }

        if (mUVScale != Scale || mUVOffset != Offset || mUVSource != Source) {
            mUVScale = Scale;
            mUVOffset = Offset;
            mUVSource = Source;

            MarkRenderStateDirty();
        }

        return true;
    }

    /**
     * @brief 요청한 제어자가 설정한 UV만 기본 변환으로 복원합니다.
     */
    void Engine::UPrimitiveComponent::ClearUVTransform(CoreUObject::TObjectRef<CoreUObject::UObject> Source) {
        if (mUVSource == Source) {
            mUVScale = {1.0f, 1.0f};
            mUVOffset = {};
            mUVSource = {};

            MarkRenderStateDirty();
        }
    }

    /**
     * @brief 기본 UV에 곱할 셀 크기를 반환합니다.
     */
    const Core::FVector2& Engine::UPrimitiveComponent::GetUVScale() const {
        return mUVScale;
    }

    /**
     * @brief 기본 UV에 더할 셀 원점을 반환합니다.
     */
    const Core::FVector2& Engine::UPrimitiveComponent::GetUVOffset() const {
        return mUVOffset;
    }

    /**
     * @brief 프리미티브의 그림자 생성 여부를 반환합니다.
     */
    bool Engine::UPrimitiveComponent::GetCastShadow() const {
        return mCastShadow;
    }

    /**
     * @brief 프리미티브의 그림자 생성 여부를 검증하고 변경합니다.
     */
    bool Engine::UPrimitiveComponent::SetCastShadow(bool CastShadow) {
        mCastShadow = CastShadow;

        MarkRenderStateDirty();

        return true;
    }

    /**
     * @brief 프리미티브의 그림자 수신 여부를 반환합니다.
     */
    bool Engine::UPrimitiveComponent::GetReceiveShadow() const {
        return mReceiveShadow;
    }

    bool Engine::UPrimitiveComponent::GetReceivesDecals() const {
        return mReceivesDecals;
    }

    void Engine::UPrimitiveComponent::SetReceivesDecals(bool ReceivesDecals) {
        mReceivesDecals = ReceivesDecals;
        MarkRenderStateDirty();
    }

    /**
     * @brief 프리미티브의 그림자 수신 여부를 검증하고 변경합니다.
     */
    bool Engine::UPrimitiveComponent::SetReceiveShadow(bool ReceiveShadow) {
        mReceiveShadow = ReceiveShadow;

        MarkRenderStateDirty();

        return true;
    }

}
