/**
 * @file UWorld.cpp
 * @brief 영속 레벨, 렌더 장면과 프레임 말 컴포넌트 갱신을 관리합니다.
 * @ingroup Engine
 */

#include "Engine/UWorld.h"
#include "Engine/Component/UHeightFogComponent.h"
#include "Engine/EngineStats.h"
#include "Engine/Component/ULightComponentBase.h"

#include "Engine/MeshPicking.h"
#include "Engine/Component/UStaticMeshComponent.h"
#include "Engine/Component/UBillboardComponent.h"
#include "Engine/Component/UCameraComponent.h"
#include "Engine/FViewportInput.h"
#include "Core/Math.h"
#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/AStaticMeshActor.h"
#include "Engine/ULevel.h"
#include "Engine/FWorldHit.h"
#include "CoreUObject/FObjectHandle.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FRay.h"
#include "Engine/FMeshData.h"
#include "Engine/FSceneInterface.h"
#include "Engine/EPrimitiveType.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Engine/PrimitiveMesh.h"

#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/UClass.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace Engine {

    /**
     * @brief UWorld의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Engine::UWorld::UWorld() = default;

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Engine::UWorld::~UWorld() {
        Shutdown();
    }

    void Engine::UWorld::SetAssets(Engine::FAssetManager* Assets) {
        if (mAssets == Assets) {
            return;
        }

        mAssets = Assets;

        if (mPersistentLevel != nullptr) {
            for (const auto& Actor : mPersistentLevel->GetActors()) {
                for (const auto& Component : Actor->GetComponents()) {
                    if (auto* Mesh{CoreUObject::Cast<Engine::UStaticMeshComponent>(Component.get())}) {
                        Mesh->ResolveMesh();
                        Mesh->ResolveMaterial();
                    }

                    if (auto* Sprite{CoreUObject::Cast<Engine::UBillboardComponent>(Component.get())}) {
                        Sprite->ResolveTexture();
                    }

                    if (auto* Light{CoreUObject::Cast<Engine::ULightComponentBase>(Component.get())}) {
                        Light->SynchronizeLight();
                    }

                    if (auto* Fog{CoreUObject::Cast<Engine::UHeightFogComponent>(Component.get())}) {
                        Fog->SynchronizeFog();
                    }
                }
            }
        }
    }

    Engine::FAssetManager* Engine::UWorld::GetAssets() const {
        return mAssets;
    }

    /**
     * @brief 아직 레벨이 없는 월드에 영속 레벨의 소유권을 연결합니다.
     */
    bool Engine::UWorld::SetPersistentLevel(std::unique_ptr<Engine::ULevel> Level) {
        if (mPersistentLevel != nullptr || Level == nullptr || Level->mWorld != nullptr || mShuttingDown || IsDispatching() || mPlayState != Engine::EWorldPlayState::Stopped) {
            return false;
        }

        mPersistentLevel = std::move(Level);
        mPersistentLevel->SetOuter(this);
        mPersistentLevel->SetWorld(this);

        return true;
    }

    /**
     * @brief 월드가 소유한 영속 레벨을 반환합니다.
     */
    Engine::ULevel* Engine::UWorld::GetPersistentLevel() const {
        return mPersistentLevel.get();
    }

    /**
     * @brief 객체의 실제 소유 관계가 이 월드에 속하는지 검사합니다.
     */
    bool Engine::UWorld::ContainsObject(const CoreUObject::UObject* Object) const {
        if (Object == nullptr) {
            return false;
        }

        if (Object == this || Object == mPersistentLevel.get()) {
            return true;
        }

        if (const auto* Actor{CoreUObject::Cast<Engine::AActor>(Object)}) {
            return Actor->GetWorld() == this && !Actor->IsPendingDestroy();
        }

        const auto* Component{CoreUObject::Cast<Engine::UActorComponent>(Object)};

        return Component != nullptr && Component->GetOwner() != nullptr && ContainsObject(Component->GetOwner());
    }

    /**
     * @brief 월드에 렌더 장면을 연결하고 기존 프리미티브를 동기화합니다.
     */
    bool Engine::UWorld::AttachScene(std::unique_ptr<Engine::FSceneInterface> Scene) {
        if (mScene != nullptr || Scene == nullptr || mShuttingDown || IsDispatching()) {
            return false;
        }

        mScene = std::move(Scene);

        if (mPersistentLevel != nullptr) {
            for (const auto& Actor : mPersistentLevel->GetActors()) {
                for (const auto& Component : Actor->GetComponents()) {
                    if (auto* Light{CoreUObject::Cast<Engine::ULightComponentBase>(Component.get())}) {
                        Light->SynchronizeLight();
                    }

                    if (auto* Fog{CoreUObject::Cast<Engine::UHeightFogComponent>(Component.get())}) {
                        Fog->SynchronizeFog();
                    }

                    if (auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(Component.get())}) {
                        Primitive->MarkRenderStateDirty();
                    }
                }
            }
        }

        SendAllEndOfFrameUpdates();

        return true;
    }

    /**
     * @brief 연결된 렌더 장면을 반환합니다.
     */
    Engine::FSceneInterface* Engine::UWorld::GetScene() const {
        return mScene.get();
    }

    /**
     * @brief 지정한 액터 클래스의 기본 컴포넌트를 생성하고 영속 레벨에 추가합니다.
     */
    Engine::AActor* Engine::UWorld::SpawnActor(const CoreUObject::UClass* Class) {
        if (Class == nullptr || !Class->IsChildOf(Engine::AActor::StaticClass()) || !Class->CanCreate() || mPersistentLevel == nullptr || !CanAddActor()) {
            return nullptr;
        }

        auto Object{Class->CreateInstance()};

        if (Object == nullptr || Object->GetClass() != Class) {
            return nullptr;
        }

        std::unique_ptr<Engine::AActor> Actor{static_cast<Engine::AActor*>(Object.release())};
        Core::FString Name{};

        do {
            Name = Class->GetName() + "_" + std::to_string(++mSpawnSerial);
        } while (CoreUObject::FObjectRegistry::Get()->FindObject(Core::FName{Name}, mPersistentLevel.get()) != nullptr);
        Actor->SetName(Name);

        if (!Actor->InitializeDefaultComponents()) {
            return nullptr;
        }

        const CoreUObject::FObjectHandle Handle{Actor->GetHandle()};

        return mPersistentLevel->AddActor(std::move(Actor)) ? ResolveActor(Handle) : nullptr;
    }

    /**
     * @brief 지정한 기본 메시 종류의 액터를 생성하여 영속 레벨에 추가합니다.
     */
    Engine::AStaticMeshActor* Engine::UWorld::SpawnPrimitive(Engine::EPrimitiveType Type) {
        if (Type >= Engine::EPrimitiveType::Count || mPersistentLevel == nullptr || !CanAddActor()) {
            return nullptr;
        }

        auto Actor{CoreUObject::NewObject<Engine::AStaticMeshActor>()};

        if (Actor == nullptr || !Actor->InitializeDefaultComponents()) {
            return nullptr;
        }

        Core::FString Name{};

        do {
            Name = Engine::PrimitiveName(Type) + "_" + std::to_string(++mSpawnSerial);
        } while (CoreUObject::FObjectRegistry::Get()->FindObject(Core::FName{Name}, mPersistentLevel.get()) != nullptr);

        Actor->SetName(Name);
        Actor->GetStaticMeshComponent()->SetPrimitiveType(Type);

        const CoreUObject::FObjectHandle Handle{Actor->GetHandle()};

        return mPersistentLevel->AddActor(std::move(Actor)) ? CoreUObject::Cast<Engine::AStaticMeshActor>(ResolveActor(Handle)) : nullptr;
    }

    /**
     * @brief 핸들에 해당하는 액터의 컴포넌트를 등록 해제하고 액터를 제거합니다.
     */
    bool Engine::UWorld::DestroyActor(CoreUObject::FObjectHandle Handle) {
        Engine::AActor* Actor{ResolveActor(Handle)};

        if (Actor == nullptr || Actor->IsPendingDestroy()) {
            return false;
        }

        Actor->mPendingDestroy = true;
        mPendingActors.push_back(Handle);
        FlushPendingActors();

        return true;
    }

    /**
     * @brief 컴포넌트 핸들을 프레임 말 렌더 갱신 큐에 추가합니다.
     */
    void Engine::UWorld::EnqueueRenderUpdate(CoreUObject::FObjectHandle Handle) {
        mDirtyComponents.push_back(Handle);
    }

    /**
     * @brief 큐에 쌓인 컴포넌트 핸들을 해석하여 지연된 렌더 갱신을 반영합니다.
     */
    void Engine::UWorld::SendAllEndOfFrameUpdates() {
        const Core::FScopedStatTimer StatTimer{Engine::EngineStats::RenderUpdates};

        if (mScene == nullptr) {
            return;
        }

        const std::size_t Count{mDirtyComponents.size()};

        for (std::size_t Index{}; Index < Count; ++Index) {
            const CoreUObject::FObjectHandle Handle{mDirtyComponents[Index]};

            if (auto* Component{CoreUObject::Cast<Engine::UPrimitiveComponent>(CoreUObject::FObjectRegistry::Get()->Resolve(Handle))}; Component != nullptr && Component->GetWorld() == this) {
                Component->SendRenderUpdates();
            }
        }

        mDirtyComponents.erase(mDirtyComponents.begin(), mDirtyComponents.begin() + static_cast<std::ptrdiff_t>(Count));
    }

    /**
     * @brief 월드 프리미티브의 메시를 검사하여 광선에 가장 가까운 충돌을 찾습니다.
     */
    bool Engine::UWorld::Raycast(const Core::FRay& Ray, Engine::FWorldHit& Hit) const {
        const Core::FScopedStatTimer StatTimer{Engine::EngineStats::Raycast};
        Core::FRay NormalizedRay{Ray};

        if (mPersistentLevel == nullptr || !NormalizedRay.mOrigin.IsFinite() || !NormalizedRay.mDirection.Normalize()) {
            return false;
        }

        bool Found{};
        float Closest{std::numeric_limits<float>::max()};

        for (const auto& Actor : mPersistentLevel->GetActors()) {
            if (Actor->IsPendingDestroy()) {
                continue;
            }

            for (const auto& Component : Actor->GetComponents()) {
                const auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(Component.get())};

                if (Primitive != nullptr && Engine::RaycastMesh(&Primitive->GetMeshData(), Primitive->GetWorldMatrix(), NormalizedRay, Closest)) {
                    Hit = {Actor->GetHandle(), Primitive->GetHandle(), NormalizedRay.mOrigin + NormalizedRay.mDirection * Closest, Closest};
                    Found = true;
                }
            }
        }

        return Found;
    }

    /**
     * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
     */
    void Engine::UWorld::Shutdown() {
        if (IsDispatching()) {
            mShutdownRequested = true;
            return;
        }

        mShutdownRequested = false;
        EndPlay();
        mShuttingDown = true;

        if (mPersistentLevel != nullptr) {
            mPersistentLevel->SetWorld(nullptr);
        }

        mPendingActors.clear();
        mDirtyComponents.clear();
        mActiveCamera = {};
        mInputTarget = {};
        mPersistentLevel.reset();
        mScene.reset();
    }

    /**
     * @brief 월드의 콜백 전달 깊이를 증가시킵니다.
     */
    Engine::UWorld::FDispatchScope::FDispatchScope(Engine::UWorld* World)
		: mWorld{World} {
        if (mWorld != nullptr) {
            ++mWorld->mDispatchDepth;
        }
    }

    /**
     * @brief 전달 깊이를 감소시키고 안전한 경계에서 지연 작업을 처리합니다.
     */
    Engine::UWorld::FDispatchScope::~FDispatchScope() {
        if (mWorld != nullptr) {
            mWorld->FinishDispatch();
        }
    }

    /**
     * @brief 이 월드를 엔진에 등록할 때 발급한 핸들을 반환합니다.
     */
    Engine::FWorldHandle Engine::UWorld::GetWorldHandle() const {
        return mWorldHandle;
    }

    /**
     * @brief 월드의 편집 또는 실행 용도를 반환합니다.
     */
    Engine::EWorldType Engine::UWorld::GetWorldType() const {
        return mWorldType;
    }

    /**
     * @brief 현재 게임 실행 생명주기 상태를 반환합니다.
     */
    Engine::EWorldPlayState Engine::UWorld::GetPlayState() const {
        return mPlayState;
    }

    /**
     * @brief 정지 상태의 Game 또는 PIE 월드에서 게임 생명주기를 시작합니다.
     */
    bool Engine::UWorld::BeginPlay() {
        if (mPlayState != Engine::EWorldPlayState::Stopped || IsDispatching() || mShuttingDown || (mWorldType != Engine::EWorldType::Game && mWorldType != Engine::EWorldType::PIE)) {
            return false;
        }

        FDispatchScope Scope{this};

        mPlayState = Engine::EWorldPlayState::Starting;
        mTimeSeconds = 0.0f;
        mTickCount = 0;

        const auto Handles{GetActorHandles()};

        for (const CoreUObject::FObjectHandle Handle : Handles) {
            if (!CanBeginActor()) {
                break;
            }

            if (Engine::AActor* Actor{ResolveActor(Handle)}) {
                Actor->DispatchBeginPlay();
            }
        }

        mPlayState = Engine::EWorldPlayState::Playing;

        return true;
    }

    /**
     * @brief 시작된 액터와 컴포넌트의 실행을 종료합니다.
     */
    void Engine::UWorld::EndPlay() {
        if (mDispatchDepth != 0) {
            mEndPlayRequested = true;
            return;
        }

        mEndPlayRequested = false;

        if (mPlayState == Engine::EWorldPlayState::Stopped || mPlayState == Engine::EWorldPlayState::Stopping) {
            return;
        }

        FDispatchScope Scope{this};

        mPlayState = Engine::EWorldPlayState::Stopping;

        const auto Handles{GetActorHandles()};

        for (const CoreUObject::FObjectHandle Handle : Handles) {
            if (Engine::AActor* Actor{ResolveActor(Handle)}) {
                Actor->DispatchEndPlay();
            }
        }

        mPlayState = Engine::EWorldPlayState::Stopped;
    }

    /**
     * @brief Playing 상태의 게임 갱신 또는 정지한 편집 월드의 선택적 갱신을 수행합니다.
     */
    bool Engine::UWorld::Tick(float DeltaTime) {
        const bool EditorTick{mPlayState == Engine::EWorldPlayState::Stopped && (mWorldType == Engine::EWorldType::Editor || mWorldType == Engine::EWorldType::EditorPreview)};
        return (mPlayState == Engine::EWorldPlayState::Playing || EditorTick) && TickActors(DeltaTime, EditorTick);
    }

    /**
     * @brief Playing과 Paused 사이의 상태를 전환합니다.
     */
    bool Engine::UWorld::SetPaused(bool Paused) {
        if (mPlayState != Engine::EWorldPlayState::Playing && mPlayState != Engine::EWorldPlayState::Paused) {
            return false;
        }

        mPlayState = Paused ? Engine::EWorldPlayState::Paused : Engine::EWorldPlayState::Playing;

        return true;
    }

    /**
     * @brief Paused 상태를 유지하면서 게임 갱신을 한 번 수행합니다.
     */
    bool Engine::UWorld::Step(float DeltaTime) {
        return mPlayState == Engine::EWorldPlayState::Paused && DeltaTime > 0.0f && TickActors(DeltaTime, false);
    }

    /**
     * @brief 마지막 실행 시작 이후 누적 게임 시간을 반환합니다.
     */
    float Engine::UWorld::GetTimeSeconds() const {
        return mTimeSeconds;
    }

    /**
     * @brief 마지막 실행 시작 이후 수행한 게임 갱신 횟수를 반환합니다.
     */
    Core::Uint64 Engine::UWorld::GetTickCount() const {
        return mTickCount;
    }

    /**
     * @brief 콜백 전달 또는 지연된 액터 정리를 처리 중인지 검사합니다.
     */
    bool Engine::UWorld::IsDispatching() const {
        return mDispatchDepth != 0 || mFlushing;
    }

    bool Engine::UWorld::SetActiveCamera(Engine::UCameraComponent* Camera) {
        if (!CanAddActor() || (Camera != nullptr && (Camera->GetWorld() != this || !ContainsObject(Camera)))) {
            return false;
        }

        mActiveCamera = Camera != nullptr ? Camera->GetHandle() : CoreUObject::FObjectHandle{};

        return true;
    }

    Engine::UCameraComponent* Engine::UWorld::GetActiveCamera() const {
        auto* Camera{CoreUObject::Cast<Engine::UCameraComponent>(CoreUObject::FObjectRegistry::Get()->Resolve(mActiveCamera))};
        return Camera != nullptr && Camera->GetWorld() == this && ContainsObject(Camera) ? Camera : nullptr;
    }

    bool Engine::UWorld::SetInputTarget(Engine::AActor* Actor) {
        if (!CanAddActor() || (Actor != nullptr && !ContainsObject(Actor))) {
            return false;
        }

        mInputTarget = Actor != nullptr ? Actor->GetHandle() : CoreUObject::FObjectHandle{};

        return true;
    }

    Engine::AActor* Engine::UWorld::GetInputTarget() const {
        Engine::AActor* Actor{ResolveActor(mInputTarget)};
        return Actor != nullptr && !Actor->IsPendingDestroy() ? Actor : nullptr;
    }

    bool Engine::UWorld::Input(const Engine::FViewportInput& Input, float DeltaTime) {
        Engine::AActor* Actor{GetInputTarget()};

        if ((mWorldType != Engine::EWorldType::Game && mWorldType != Engine::EWorldType::PIE) || mPlayState != Engine::EWorldPlayState::Playing || IsDispatching() || !CanBeginActor() || !Input.mFocused || !std::isfinite(DeltaTime) || DeltaTime < 0.0f || Actor == nullptr || !Actor->HasBegunPlay()) {
            return false;
        }

        FDispatchScope Scope{this};

        Actor->Input(Input, DeltaTime);

        return true;
    }

    /**
     * @brief 시간을 검증하고 액터 핸들 스냅샷으로 갱신을 전달합니다.
     */
    bool Engine::UWorld::TickActors(float DeltaTime, bool EditorTick) {
        if (!std::isfinite(DeltaTime) || DeltaTime < 0.0f || (!EditorTick && !std::isfinite(mTimeSeconds + DeltaTime)) || IsDispatching() || mShuttingDown) {
            return false;
        }

        const Core::FScopedStatTimer StatTimer{Engine::EngineStats::WorldTick};

        Engine::EngineStats::WorldTicks.Add();

        FDispatchScope Scope{this};

        if (!EditorTick) {
            mTimeSeconds += DeltaTime;
            ++mTickCount;
        }

        const auto Handles{GetActorHandles()};

        for (const CoreUObject::FObjectHandle Handle : Handles) {
            if (mEndPlayRequested || mShutdownRequested) {
                break;
            }

            if (Engine::AActor* Actor{ResolveActor(Handle)}; Actor != nullptr && !Actor->IsPendingDestroy()) {
                Actor->DispatchTick(DeltaTime, EditorTick);
            }
        }

        return true;
    }

    /**
     * @brief 전달 깊이를 줄이고 안전한 경계에서 지연 작업을 처리합니다.
     */
    void Engine::UWorld::FinishDispatch() {
        --mDispatchDepth;
        FlushPendingActors();
    }

    /**
     * @brief 가장 바깥 전달 범위 이후 종료 요청과 액터 삭제 및 월드 종료를 처리합니다.
     */
    void Engine::UWorld::FlushPendingActors() {
        if (IsDispatching()) {
            return;
        }

        mFlushing = true;

        if (mEndPlayRequested) {
            EndPlay();
        }

        while (!mPendingActors.empty() && mPersistentLevel != nullptr) {
            auto Pending{std::move(mPendingActors)};

            mPendingActors.clear();

            for (const CoreUObject::FObjectHandle Handle : Pending) {
                mPersistentLevel->RemoveActor(Handle);
            }
        }

        mFlushing = false;

        if (mShutdownRequested) {
            Shutdown();
        }
    }

    /**
     * @brief 월드 상태에 맞추어 새 액터의 시작 콜백을 전달합니다.
     */
    void Engine::UWorld::NotifyActorAdded(Engine::AActor* Actor) {
        FDispatchScope Scope{this};

        if (CanBeginActor()) {
            Actor->DispatchBeginPlay();
        }
    }

    /**
     * @brief 월드가 새 액터나 컴포넌트 등록을 수락할 수 있는지 검사합니다.
     */
    bool Engine::UWorld::CanAddActor() const {
        return !mShuttingDown && !mShutdownRequested && mPlayState != Engine::EWorldPlayState::Stopping;
    }

    /**
     * @brief 월드가 액터 시작 콜백을 전달할 수 있는지 검사합니다.
     */
    bool Engine::UWorld::CanBeginActor() const {
        return !mEndPlayRequested && !mShutdownRequested && (mPlayState == Engine::EWorldPlayState::Starting || mPlayState == Engine::EWorldPlayState::Playing || mPlayState == Engine::EWorldPlayState::Paused);
    }

    /**
     * @brief 컨테이너 변경에 영향받지 않는 액터 핸들 스냅샷을 만듭니다.
     */
    Core::TArray<CoreUObject::FObjectHandle> Engine::UWorld::GetActorHandles() const {
        Core::TArray<CoreUObject::FObjectHandle> Handles{};

        if (mPersistentLevel != nullptr) {
            Handles.reserve(mPersistentLevel->GetActors().size());

            for (const auto& Actor : mPersistentLevel->GetActors()) {
                Handles.push_back(Actor->GetHandle());
            }
        }

        return Handles;
    }

    /**
     * @brief 전역 핸들을 해석하고 이 월드의 액터인지 확인합니다.
     */
    Engine::AActor* Engine::UWorld::ResolveActor(CoreUObject::FObjectHandle Handle) const {
        Engine::AActor* Actor{CoreUObject::Cast<Engine::AActor>(CoreUObject::FObjectRegistry::Get()->Resolve(Handle))};
        return Actor != nullptr && Actor->GetWorld() == this ? Actor : nullptr;
    }

}
