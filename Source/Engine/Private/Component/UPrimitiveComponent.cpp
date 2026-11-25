/**
 * @file UPrimitiveComponent.cpp
 * @brief 렌더링과 충돌에 쓰이는 기하 형태를 가진 컴포넌트의 기반 클래스입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UPrimitiveComponent.h"
#include "Asset/UStaticMesh.h"
#include "Engine/AActor.h"
#include "Engine/Collision/FHitResult.h"
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
#include "Engine/PhysicsBodyDrawing.h"
#include "PhysicsRuntime/UBodySetup.h"
#include "Core/Math/FQuat.h"
#include "Core/Literals.h"

#include <algorithm>
#include <cmath>

using namespace Core::Literals;

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
    bool Engine::UPrimitiveComponent::SetTransform(const FTransform& Transform) {
        if (!Engine::USceneComponent::SetTransform(Transform)) {
            return false;
        }

        MarkRenderTransformDirty();

        // 시뮬레이션 결과를 반영하는 중이 아니면 바디를 새 트랜스폼으로 옮깁니다. 스케일은 형상 크기에 반영되어 있으므로 바뀌면 다시 만듭니다.
        if (!mSyncingFromPhysics && mBodyInstance.IsValidBodyInstance()) {
            if (Transform.mScale.Equals(mBodyInstance.GetBodyScale())) {
                mBodyInstance.SetBodyTransform(Transform);
            } else {
                RecreatePhysicsState();
            }
        }

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

        const Engine::EWorldType WorldType{GetWorld()->GetWorldType()};
        const bool HiddenInThisWorld{mHiddenInGame && (WorldType == Engine::EWorldType::Game || WorldType == Engine::EWorldType::PIE)};

        if (!IsOwnerVisible() || HiddenInThisWorld || !ShouldCreateRenderState()) {
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
        const FMatrix World{GetWorldMatrix()};
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
    FBox Engine::UPrimitiveComponent::GetWorldBounds() const {
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
    bool Engine::UPrimitiveComponent::SetUVTransform(const FVector2& Scale, const FVector2& Offset, CoreUObject::TObjectRef<CoreUObject::UObject> Source) {
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
    const FVector2& Engine::UPrimitiveComponent::GetUVScale() const {
        return mUVScale;
    }

    /**
     * @brief 기본 UV에 더할 셀 원점을 반환합니다.
     */
    const FVector2& Engine::UPrimitiveComponent::GetUVOffset() const {
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

    bool Engine::UPrimitiveComponent::IsHiddenInGame() const {
        return mHiddenInGame;
    }

    void Engine::UPrimitiveComponent::SetHiddenInGame(bool HiddenInGame) {
        if (mHiddenInGame != HiddenInGame) {
            mHiddenInGame = HiddenInGame;
            MarkRenderStateDirty();
        }
    }

    /**
     * @brief 프리미티브의 그림자 수신 여부를 검증하고 변경합니다.
     */
    bool Engine::UPrimitiveComponent::SetReceiveShadow(bool ReceiveShadow) {
        mReceiveShadow = ReceiveShadow;

        MarkRenderStateDirty();

        return true;
    }

    /**
     * @brief 컴포넌트 겹침 시작 이벤트를 반환합니다.
     */
    Engine::UPrimitiveComponent::FOverlapDelegate& Engine::UPrimitiveComponent::GetOnComponentBeginOverlap() {
        return mOnComponentBeginOverlap;
    }

    /**
     * @brief 컴포넌트 겹침 종료 이벤트를 반환합니다.
     */
    Engine::UPrimitiveComponent::FOverlapDelegate& Engine::UPrimitiveComponent::GetOnComponentEndOverlap() {
        return mOnComponentEndOverlap;
    }

    /**
     * @brief 컴포넌트 충돌 이벤트를 반환합니다.
     */
    Engine::UPrimitiveComponent::FHitDelegate& Engine::UPrimitiveComponent::GetOnComponentHit() {
        return mOnComponentHit;
    }

    /**
     * @brief 다른 컴포넌트와 겹칠 때 겹침 이벤트를 만드는지 반환합니다.
     */
    bool Engine::UPrimitiveComponent::GetGenerateOverlapEvents() const {
        return mGenerateOverlapEvents;
    }

    /**
     * @brief 다른 컴포넌트와 겹칠 때 겹침 이벤트를 만들지 설정합니다.
     */
    void Engine::UPrimitiveComponent::SetGenerateOverlapEvents(bool Generate) {
        mGenerateOverlapEvents = Generate;
    }

    /**
     * @brief 겹침 시작 델리게이트와 소유 액터의 충돌 이벤트를 호출합니다.
     */
    void Engine::UPrimitiveComponent::OnBeginOverlap(Engine::UPrimitiveComponent* Other) {
        auto* OtherActor{Other != nullptr ? Other->GetOwner() : nullptr};
        mOnComponentBeginOverlap.Broadcast(this, OtherActor, Other);
        if (auto* Owner{GetOwner()}; Owner != nullptr && !Owner->IsPendingDestroy()) {
            Owner->GetOnComponentCollision().Broadcast(Engine::AActor::EComponentCollision::BeginOverlap, this, OtherActor, Other, nullptr);
        }
    }

    /**
     * @brief 겹침 종료 델리게이트와 소유 액터의 충돌 이벤트를 호출합니다.
     */
    void Engine::UPrimitiveComponent::OnEndOverlap(Engine::UPrimitiveComponent* Other) {
        auto* OtherActor{Other != nullptr ? Other->GetOwner() : nullptr};
        mOnComponentEndOverlap.Broadcast(this, OtherActor, Other);
        if (auto* Owner{GetOwner()}; Owner != nullptr && !Owner->IsPendingDestroy()) {
            Owner->GetOnComponentCollision().Broadcast(Engine::AActor::EComponentCollision::EndOverlap, this, OtherActor, Other, nullptr);
        }
    }

    /**
     * @brief 적중 액터·컴포넌트와 충돌 정보를 델리게이트와 소유 액터의 충돌 이벤트로 전달합니다.
     */
    void Engine::UPrimitiveComponent::OnHit(Engine::AActor* OtherActor, Engine::UPrimitiveComponent* OtherComp, const Engine::FHitResult& Hit) {
        mOnComponentHit.Broadcast(this, OtherActor, OtherComp, Hit);
        if (auto* Owner{GetOwner()}; Owner != nullptr && !Owner->IsPendingDestroy()) {
            Owner->GetOnComponentCollision().Broadcast(Engine::AActor::EComponentCollision::Hit, this, OtherActor, OtherComp, &Hit);
        }
    }

    /**
     * @brief 바디를 만들 충돌 형상이 있으면 물리 상태가 필요하다고 반환합니다.
     */
    bool Engine::UPrimitiveComponent::ShouldCreatePhysicsState() const {
        return GetBodySetup() != nullptr;
    }

    /**
     * @brief 물리 바디를 만들 때 사용할 충돌 형상을 반환하는 확장 지점입니다.
     */
    const PhysicsRuntime::UBodySetup* Engine::UPrimitiveComponent::GetBodySetup() const {
        return nullptr;
    }

    /**
     * @brief 이 컴포넌트의 바디가 현재 자세에서 겹치는 다른 바디를 장면 쿼리로 찾습니다.
     */
    bool Engine::UPrimitiveComponent::ComponentOverlapMulti(Core::TArray<PhysicsRuntime::FBodyInstance*>& OutOverlaps) const {
        return mBodyInstance.OverlapMulti(OutOverlaps);
    }

    /**
     * @brief 충돌 프리셋을 적용합니다.
     */
    void Engine::UPrimitiveComponent::SetCollisionPreset(PhysicsRuntime::ECollisionPreset Preset) {
        mBodyInstance.SetCollisionPreset(Preset);
    }

    /**
     * @brief 쿼리·시뮬레이션 참여 방식을 반환합니다.
     */
    PhysicsRuntime::ECollisionEnabled Engine::UPrimitiveComponent::GetCollisionEnabled() const {
        return mBodyInstance.GetCollisionEnabled();
    }

    /**
     * @brief 쿼리·시뮬레이션 참여 방식을 바꿉니다.
     */
    void Engine::UPrimitiveComponent::SetCollisionEnabled(PhysicsRuntime::ECollisionEnabled CollisionEnabled) {
        mBodyInstance.SetCollisionEnabled(CollisionEnabled);
    }

    /**
     * @brief 다른 모든 바디에 대한 응답을 바꿉니다.
     */
    void Engine::UPrimitiveComponent::SetCollisionResponseToAllChannels(PhysicsRuntime::ECollisionResponse Response) {
        mBodyInstance.SetCollisionResponse(Response);
    }

    /**
     * @brief 바디가 있으면 이동 경로를 쓸어 응답이 Block인 바디에 막히는 지점에서 멈추고 양쪽에 Hit 이벤트를 보냅니다.
     */
    bool Engine::UPrimitiveComponent::MoveComponentImpl(const FVector& Delta, bool Sweep, Engine::FHitResult& OutHit) {
        return Sweep ? MoveComponentSwept(Delta, GetTransform().mRotation, EInitialPenetrationMode::Allow, OutHit) : ApplyMove(Delta);
    }

    bool Engine::UPrimitiveComponent::MoveComponentSwept(const FVector& Delta, const FQuat& NewRotation, EInitialPenetrationMode PenetrationMode, Engine::FHitResult& OutHit) {
        const auto ApplyTransform{[this, &NewRotation](const FVector& Offset) {
            FTransform Transform{GetTransform()};
            Transform.mLocation += Offset;
            Transform.mRotation = NewRotation;

            return SetTransform(Transform);
        }};

        if (Delta.IsNearlyZero() || !mBodyInstance.IsValidBodyInstance()) {
            return ApplyTransform(Delta);
        }

        Engine::AActor* const Owner{GetOwner()};
        const PhysicsRuntime::FIgnoreBodyPredicate IgnoreOwnActor{[this, Owner](const PhysicsRuntime::FBodyInstance& Body) {
            const Engine::UPrimitiveComponent* Component{Body.GetOwnerComponent()};

            return Component == this || (Component != nullptr && Owner != nullptr && Component->GetOwner() == Owner);
        }};
        PhysicsRuntime::FBodySweepHit SweepHit{};

        if (!mBodyInstance.Sweep(Delta, SweepHit, IgnoreOwnActor)) {
            return ApplyTransform(Delta);
        }

        constexpr Core::FLength PullBackDistance{1_cm};
        FVector Adjustment{};

        if (PenetrationMode == EInitialPenetrationMode::Resolve && SweepHit.mStartPenetrating && SweepHit.mImpactNormal.IsNormalized() && SweepHit.mPenetrationDepth.IsFinite() && SweepHit.mPenetrationDepth > 0_m) {
            const FVector Candidate{SweepHit.mImpactNormal * (SweepHit.mPenetrationDepth + PullBackDistance).GetBase()};
            const FVector AdjustedStart{OutHit.mTraceStart + Candidate};
            const FVector AdjustedEnd{AdjustedStart + Delta};
            PhysicsRuntime::FBodySweepHit AdjustmentHit{};

            if (AdjustedEnd.IsFinite() && !mBodyInstance.Sweep(Candidate, AdjustmentHit, IgnoreOwnActor)) {
                PhysicsRuntime::FBodySweepHit RetryHit{};
                const bool RetryBlocked{mBodyInstance.Sweep(Delta, RetryHit, IgnoreOwnActor, Candidate)};

                if (!RetryHit.mStartPenetrating) {
                    OutHit.Init(AdjustedStart, AdjustedEnd);

                    if (!RetryBlocked) {
                        return ApplyTransform(Candidate + Delta);
                    }

                    Adjustment = Candidate;
                    SweepHit = RetryHit;
                }
            }
        }

        Engine::UPrimitiveComponent* const OtherComponent{SweepHit.mBody != nullptr ? SweepHit.mBody->GetOwnerComponent() : nullptr};
        const Core::FLength Length{Core::FLength::FromBase(Delta.Length())};

        OutHit.mBlockingHit = true;
        OutHit.mStartPenetrating = SweepHit.mStartPenetrating;
        OutHit.mTime = SweepHit.mDistance / Length;
        OutHit.mDistance = SweepHit.mDistance;
        OutHit.mLocation = OutHit.mTraceStart + Delta * OutHit.mTime;
        OutHit.mImpactPoint = SweepHit.mImpactPoint;
        OutHit.mNormal = SweepHit.mImpactNormal;
        OutHit.mImpactNormal = SweepHit.mImpactNormal;
        OutHit.mPenetrationDepth = SweepHit.mPenetrationDepth;
        OutHit.SetComponent(OtherComponent);
        OutHit.SetActor(OtherComponent != nullptr ? OtherComponent->GetOwner() : nullptr);

        if (PenetrationMode != EInitialPenetrationMode::Allow && OutHit.mStartPenetrating) {
            return false;
        }

        // 막은 표면에 붙어 다음 이동이 시작부터 겹치지 않도록 1 cm 물러난 위치까지만 옮깁니다.
        const Core::FLength MoveDistance{std::max(0_m, SweepHit.mDistance - PullBackDistance)};

        if (!ApplyTransform(Adjustment + Delta * (MoveDistance / Length))) {
            return false;
        }

        // Hit 이벤트 안에서 액터가 파괴될 수 있으므로 지연 삭제 범위 안에서 보내고, 상대에게 보내기 전에 핸들을 다시 확인합니다.
        Engine::UWorld::FDispatchScope Scope{GetWorld()};
        const CoreUObject::TObjectRef<Engine::UPrimitiveComponent> SelfHandle{CoreUObject::MakeObjectRef(this)};
        const CoreUObject::TObjectRef<Engine::UPrimitiveComponent> OtherHandle{CoreUObject::MakeObjectRef(OtherComponent)};

        OnHit(OutHit.GetActor(), OtherComponent, OutHit);

        if (Engine::UPrimitiveComponent* Self{SelfHandle.Get()}, *Other{OtherHandle.Get()}; Self != nullptr && Other != nullptr) {
            Other->OnHit(Self->GetOwner(), Self, Engine::FHitResult::GetReversedHit(OutHit, Self, Self->GetOwner()));
        }

        return true;
    }

    /**
     * @brief 물리 시뮬레이션 설정과 실제 바디를 가진 바디 인스턴스를 반환합니다.
     */
    const PhysicsRuntime::FBodyInstance& Engine::UPrimitiveComponent::GetBodyInstance() const {
        return mBodyInstance;
    }

    /**
     * @brief 바디 인스턴스 설정을 바꾸고 물리 상태를 다시 만듭니다.
     */
    bool Engine::UPrimitiveComponent::SetBodyInstance(const PhysicsRuntime::FBodyInstance& BodyInstance) {
        // 대입은 실제 바디를 해제하므로 물리 상태 표시와 어긋나지 않도록 상태를 먼저 해제하고 다시 만듭니다.
        DestroyPhysicsState();
        mBodyInstance = BodyInstance;
        CreatePhysicsState();

        return true;
    }

    /**
     * @brief 실제 바디가 동적 강체로 시뮬레이션되고 있는지 반환합니다.
     */
    bool Engine::UPrimitiveComponent::IsSimulatingPhysics() const {
        return mBodyInstance.IsDynamic();
    }

    /**
     * @brief 물리 시뮬레이션 여부를 바꾸고 물리 상태를 다시 만듭니다.
     */
    void Engine::UPrimitiveComponent::SetSimulatePhysics(bool Simulate) {
        if (mBodyInstance.ShouldSimulatePhysics() == Simulate) {
            return;
        }

        mBodyInstance.SetSimulatePhysics(Simulate);
        RecreatePhysicsState();
    }

    /**
     * @brief 동적 강체의 질량 중심에 힘을 더합니다.
     */
    bool Engine::UPrimitiveComponent::AddForce(const FVector& Force, bool AccelChange) {
        return mBodyInstance.AddForce(Force, AccelChange);
    }

    /**
     * @brief 동적 강체의 질량 중심에 충격량을 더합니다.
     */
    bool Engine::UPrimitiveComponent::AddImpulse(const FVector& Impulse, bool VelocityChange) {
        return mBodyInstance.AddImpulse(Impulse, VelocityChange);
    }

    /**
     * @brief 동적 강체의 선속도를 반환합니다.
     */
    FVector Engine::UPrimitiveComponent::GetPhysicsLinearVelocity() const {
        return mBodyInstance.GetLinearVelocity();
    }

    /**
     * @brief 동적 강체의 선속도를 설정합니다.
     */
    bool Engine::UPrimitiveComponent::SetPhysicsLinearVelocity(const FVector& Velocity) {
        return mBodyInstance.SetLinearVelocity(Velocity);
    }

    /**
     * @brief 동적 강체의 각속도를 반환합니다.
     */
    FVector Engine::UPrimitiveComponent::GetPhysicsAngularVelocity() const {
        return mBodyInstance.GetAngularVelocity();
    }

    /**
     * @brief 동적 강체의 각속도를 설정합니다.
     */
    bool Engine::UPrimitiveComponent::SetPhysicsAngularVelocity(const FVector& Velocity) {
        return mBodyInstance.SetAngularVelocity(Velocity);
    }

    /**
     * @brief 시뮬레이션한 동적 강체의 자세를 컴포넌트 트랜스폼에 반영합니다.
     */
    void Engine::UPrimitiveComponent::SyncComponentToBody() {
        if (!mBodyInstance.IsDynamic()) {
            return;
        }

        const FTransform Transform{mBodyInstance.GetBodyTransform()};
        Engine::AActor* Owner{GetOwner()};

        mSyncingFromPhysics = true;

        if (Owner != nullptr && Owner->GetRootComponent() == this) {
            Owner->SetActorTransform(Transform);
        } else {
            SetTransform(Transform);
        }

        mSyncingFromPhysics = false;
    }

    /**
     * @brief 바디 셋업의 충돌 형상을 현재 트랜스폼에 맞춰 선분으로 수집합니다.
     */
    void Engine::UPrimitiveComponent::DrawCollision(Engine::FPrimitiveDrawInterface& DrawInterface, bool Highlight, float Thickness) const {
        const FVector4 Color{Highlight ? FVector4{1.0f, 0.65f, 0.15f, 1.0f} : mBodyInstance.ShouldSimulatePhysics() ? FVector4{0.35f, 1.0f, 0.35f, 1.0f} : FVector4{0.35f, 0.6f, 1.0f, 1.0f}};
        DrawCollision(DrawInterface, Color, Thickness);
    }

    void Engine::UPrimitiveComponent::DrawCollision(Engine::FPrimitiveDrawInterface& DrawInterface, const FVector4& CollisionColor, float Thickness) const {
        const PhysicsRuntime::UBodySetup* Setup{GetBodySetup()};

        if (Setup == nullptr) {
            return;
        }

        Engine::PhysicsBodyDrawing::DrawBodySetup(DrawInterface, *Setup, GetTransform(), {CollisionColor.x, CollisionColor.y, CollisionColor.z}, true, Thickness, CollisionColor.w);
    }

    /**
     * @brief 바디 셋업의 형상으로 월드 물리 장면에 바디를 만듭니다.
     */
    void Engine::UPrimitiveComponent::OnCreatePhysicsState() {
        const PhysicsRuntime::UBodySetup* Setup{GetBodySetup()};
        PhysicsRuntime::FPhysicsScene* Scene{GetWorld() != nullptr ? GetWorld()->GetPhysicsScene() : nullptr};

        if (Setup != nullptr && Scene != nullptr && mBodyInstance.InitBody(*Setup, GetTransform(), this, *Scene)) {
            mOnBodyStateChanged.Broadcast(this, true);
        }
    }

    /**
     * @brief 바디를 물리 장면에서 해제합니다.
     */
    void Engine::UPrimitiveComponent::OnDestroyPhysicsState() {
        if (mBodyInstance.IsValidBodyInstance()) {
            mOnBodyStateChanged.Broadcast(this, false);
        }

        mBodyInstance.TermBody();
    }

}
