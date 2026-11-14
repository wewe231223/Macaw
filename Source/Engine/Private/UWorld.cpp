/**
 * @file UWorld.cpp
 * @brief 영속 레벨, 렌더 장면과 프레임 말 컴포넌트 갱신을 관리합니다.
 * @ingroup Engine
 */

#include "Core/Types.h"
#include "Engine/UWorld.h"
#include "Asset/FAssetManager.h"
#include "Core/FilePath.h"
#include "Engine/Component/UHeightFogComponent.h"
#include "Engine/Component/UDecalComponent.h"
#include "Engine/EngineStats.h"
#include "Engine/Component/ULightComponentBase.h"

#include "Engine/MeshPicking.h"
#include "Engine/Component/UMeshComponent.h"
#include "Engine/Component/UStaticMeshComponent.h"
#include "Engine/Component/UBillboardComponent.h"
#include "Engine/Component/UCameraComponent.h"
#include "Engine/APlayerCameraManager.h"
#include "Core/Input.h"
#include "Core/Math.h"
#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/Component/UShapeComponent.h"
#include "Engine/AStaticMeshActor.h"
#include "Engine/ULevel.h"
#include "Engine/FWorldHit.h"
#include "CoreUObject/TObjectRef.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FRay.h"
#include "Asset/FMeshData.h"
#include "Engine/FSceneInterface.h"
#include "Engine/EPrimitiveType.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Engine/PrimitiveMesh.h"

#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
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

    bool Engine::UWorld::OpenScene(const Core::FString& ScenePath) {
        if ((mWorldType != Engine::EWorldType::Game && mWorldType != Engine::EWorldType::PIE) || !mWorldHandle.IsValid() || !CanBeginActor() || mAssets == nullptr || ScenePath.empty() || ScenePath.find('\0') != Core::FString::npos) {
            return false;
        }

        Core::FString Error{};
        const auto Path{mAssets->ResolvePath(Core::FilePath::FromUtf8(ScenePath), Error)};
        std::error_code Code{};

        if (Path.empty() || !std::filesystem::is_regular_file(Path, Code)) {
            return false;
        }

        mPendingScenePath = ScenePath;

        return true;
    }

    /**
     * @brief 월드의 에셋 로드에 사용할 관리자의 비소유 주소를 설정합니다.
     */
    void Engine::UWorld::SetAssets(Asset::FAssetManager* Assets) {
        if (mAssets == Assets) {
            return;
        }

        mAssets = Assets;

        if (mPersistentLevel != nullptr) {
            for (const auto& Actor : mPersistentLevel->GetActors()) {
                for (const auto& Component : Actor->GetComponents()) {
                    if (auto* Mesh{CoreUObject::Cast<Engine::UMeshComponent>(Component.get())}) {
                        Mesh->ResolveMesh();
                        Mesh->ResolveMaterial();
                    }

                    if (auto* Sprite{CoreUObject::Cast<Engine::UBillboardComponent>(Component.get())}) {
                        Sprite->ResolveTexture();
                    }

                    if (auto* Decal{CoreUObject::Cast<Engine::UDecalComponent>(Component.get())}) {
                        Decal->ResolveTexture();
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

    /**
     * @brief 프로젝트의 에셋 관리자를 비소유 주소로 반환합니다.
     */
    Asset::FAssetManager* Engine::UWorld::GetAssets() const {
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

        if (Component != nullptr) {
            return Component->GetOwner() != nullptr && ContainsObject(Component->GetOwner());
        }

        // 컴포넌트가 소유하는 하위 객체(예: 애니메이션 인스턴스)는 Outer 컴포넌트가 월드에 있으면 월드에 속합니다.
        const CoreUObject::UObject* Outer{Object->GetOuter()};

        return Outer != nullptr && CoreUObject::Cast<Engine::UActorComponent>(Outer) != nullptr && ContainsObject(Outer);
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

                    if (auto* Decal{CoreUObject::Cast<Engine::UDecalComponent>(Component.get())}) {
                        Decal->SynchronizeDecal();
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

        const CoreUObject::TObjectRef<Engine::AActor> Handle{CoreUObject::MakeObjectRef(Actor)};

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

        const CoreUObject::TObjectRef<Engine::AActor> Handle{CoreUObject::MakeObjectRef(Actor)};

        return mPersistentLevel->AddActor(std::move(Actor)) ? CoreUObject::Cast<Engine::AStaticMeshActor>(ResolveActor(Handle)) : nullptr;
    }

    /**
     * @brief 핸들에 해당하는 액터의 컴포넌트를 등록 해제하고 액터를 제거합니다.
     */
    bool Engine::UWorld::DestroyActor(CoreUObject::TObjectRef<Engine::AActor> Handle) {
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
    void Engine::UWorld::EnqueueRenderUpdate(CoreUObject::TObjectRef<Engine::UPrimitiveComponent> Handle) {
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

        for (Core::Index_t Index{}; Index < Count; ++Index) {
            const CoreUObject::TObjectRef<Engine::UPrimitiveComponent> Handle{mDirtyComponents[Index]};

            if (auto* Component{CoreUObject::Cast<Engine::UPrimitiveComponent>(Handle.Get())}; Component != nullptr && Component->GetWorld() == this) {
                Component->SendRenderUpdates();
            }
        }

        mDirtyComponents.erase(mDirtyComponents.begin(), mDirtyComponents.begin() + static_cast<std::ptrdiff_t>(Count));
    }

    /**
     * @brief 월드 프리미티브의 메시를 검사하여 광선에 가장 가까운 충돌을 찾습니다.
     */
    bool Engine::UWorld::Raycast(const Core::FRay& Ray, Engine::FWorldHit& Hit, const Engine::AActor* Ignored) const {
        const Core::FScopedStatTimer StatTimer{Engine::EngineStats::Raycast};
        Core::FRay NormalizedRay{Ray};

        if (mPersistentLevel == nullptr || !NormalizedRay.mOrigin.IsFinite() || !NormalizedRay.mDirection.Normalize()) {
            return false;
        }

        bool Found{};
        float Closest{std::numeric_limits<float>::max()};

        for (const auto& Actor : mPersistentLevel->GetActors()) {
            if (Actor->IsPendingDestroy() || Actor.get() == Ignored) {
                continue;
            }

            for (const auto& Component : Actor->GetComponents()) {
                auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(Component.get())};

                if (Primitive != nullptr && Engine::RaycastMesh(&Primitive->GetMeshData(), Primitive->GetWorldMatrix(), NormalizedRay, Closest)) {
                    Hit = {CoreUObject::MakeObjectRef(Actor), CoreUObject::MakeObjectRef(Primitive), NormalizedRay.mOrigin + NormalizedRay.mDirection * Closest, Closest};
                    Found = true;
                }
            }
        }

        return Found;
    }

    /**
     * @brief 광선과 처음 만나는 메시를 찾아 값으로 반환합니다.
     */
    Engine::FWorldHit Engine::UWorld::Raycast(const Core::FVector& Origin, const Core::FVector& Direction, const Engine::AActor* Ignored) const {
        Engine::FWorldHit Hit{};

        Raycast(Core::FRay{Origin, Direction}, Hit, Ignored);

        return Hit;
    }

    /**
     * @brief 이름으로 살아 있는 액터를 찾습니다.
     */
    Engine::AActor* Engine::UWorld::FindActor(const Core::FString& Name) const {
        if (mPersistentLevel != nullptr) {
            for (const auto& Actor : mPersistentLevel->GetActors()) {
                if (!Actor->IsPendingDestroy() && Actor->GetName() == Name) {
                    return Actor.get();
                }
            }
        }

        return nullptr;
    }

    /**
     * @brief 지정한 클래스의 살아 있는 액터를 모두 찾습니다.
     */
    Core::TArray<Engine::AActor*> Engine::UWorld::GetActorsByClassName(const Core::FString& ClassName) const {
        const CoreUObject::UClass* Class{CoreUObject::FReflectionRegistry::FindClass(ClassName)};
        Core::TArray<Engine::AActor*> Actors{};

        if (mPersistentLevel != nullptr && Class != nullptr) {
            for (const auto& Actor : mPersistentLevel->GetActors()) {
                if (!Actor->IsPendingDestroy() && Actor->IsA(Class)) {
                    Actors.push_back(Actor.get());
                }
            }
        }

        return Actors;
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
        mPlayerCameraManager = {};
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

        // 스크립트가 BeginPlay에서 카메라 관리자를 찾을 수 있도록 액터보다 먼저 만듭니다.
        if (auto CameraManager{CoreUObject::NewObject<Engine::APlayerCameraManager>()}; CameraManager != nullptr && CameraManager->InitializeDefaultComponents() && mPersistentLevel != nullptr) {
            CameraManager->SetName("PlayerCameraManager");

            const CoreUObject::TObjectRef<Engine::APlayerCameraManager> Handle{CoreUObject::MakeObjectRef(CameraManager)};

            if (mPersistentLevel->AddActor(std::move(CameraManager))) {
                mPlayerCameraManager = Handle;
            }
        }

        const auto Handles{GetActorRefs()};

        for (const CoreUObject::TObjectRef<Engine::AActor> Handle : Handles) {
            if (!CanBeginActor()) {
                break;
            }

            if (Engine::AActor* Actor{ResolveActor(Handle)}) {
                Actor->DispatchBeginPlay();
            }
        }

        // 스크립트가 BeginPlay에서 뷰 타깃을 정하지 않았으면 편집기에서 지정한 활성 카메라를 씁니다.
        if (Engine::APlayerCameraManager* CameraManager{GetPlayerCameraManager()}) {
            if (CameraManager->GetViewTarget() == nullptr) {
                if (const Engine::UCameraComponent* Camera{GetActiveCamera()}) {
                    CameraManager->SetViewTarget(Camera->GetOwner());
                }
            }

            CameraManager->UpdateCamera(0.0f);
        }

        mPlayState = Engine::EWorldPlayState::Playing;

        return true;
    }

    /**
     * @brief 시작된 액터와 컴포넌트의 실행을 종료합니다.
     */
    void Engine::UWorld::EndPlay() {
        mPendingScenePath.clear();

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

        const auto Handles{GetActorRefs()};

        for (const CoreUObject::TObjectRef<Engine::AActor> Handle : Handles) {
            if (Engine::AActor* Actor{ResolveActor(Handle)}) {
                Actor->DispatchEndPlay();
            }
        }

        DestroyActor(mPlayerCameraManager);
        mPlayerCameraManager = {};
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

    /**
     * @brief 월드 소속을 검증하여 활성 카메라를 선택하거나 해제합니다.
     */
    bool Engine::UWorld::SetActiveCamera(Engine::UCameraComponent* Camera) {
        if (!CanAddActor() || (Camera != nullptr && (Camera->GetWorld() != this || !ContainsObject(Camera)))) {
            return false;
        }

        mActiveCamera = Camera;

        return true;
    }

    /**
     * @brief 월드에 등록된 활성 카메라를 핸들로 검증하여 조회합니다.
     */
    Engine::UCameraComponent* Engine::UWorld::GetActiveCamera() const {
        auto* Camera{CoreUObject::Cast<Engine::UCameraComponent>(mActiveCamera.Get())};

        return Camera != nullptr && Camera->GetWorld() == this && ContainsObject(Camera) ? Camera : nullptr;
    }

    /**
     * @brief 월드 소속을 검증하여 입력 대상 액터를 선택하거나 해제합니다.
     */
    bool Engine::UWorld::SetInputTarget(Engine::AActor* Actor) {
        if (!CanAddActor() || (Actor != nullptr && !ContainsObject(Actor))) {
            return false;
        }

        mInputTarget = Actor;

        return true;
    }

    /**
     * @brief 월드의 뷰포트 입력을 받을 액터를 핸들로 검증하여 조회합니다.
     */
    Engine::AActor* Engine::UWorld::GetInputTarget() const {
        Engine::AActor* Actor{ResolveActor(mInputTarget)};

        return Actor != nullptr && !Actor->IsPendingDestroy() ? Actor : nullptr;
    }

    Engine::APlayerCameraManager* Engine::UWorld::GetPlayerCameraManager() const {
        return CoreUObject::Cast<Engine::APlayerCameraManager>(ResolveActor(mPlayerCameraManager));
    }

    /**
     * @brief 뷰포트 입력을 받아 프레임 시간에 따라 액터의 동작을 갱신합니다.
     */
    bool Engine::UWorld::Input(const Core::FInput& Input, float DeltaTime) {
        Engine::AActor* Actor{GetInputTarget()};

        if ((mWorldType != Engine::EWorldType::Game && mWorldType != Engine::EWorldType::PIE) || mPlayState != Engine::EWorldPlayState::Playing || IsDispatching() || !CanBeginActor() || !Input.IsGameInputEnabled() || !std::isfinite(DeltaTime) || DeltaTime < 0.0f || Actor == nullptr || !Actor->HasBegunPlay()) {
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

        const auto Handles{GetActorRefs()};

        for (const CoreUObject::TObjectRef<Engine::AActor> Handle : Handles) {
            if (mEndPlayRequested || mShutdownRequested) {
                break;
            }

            if (Engine::AActor* Actor{ResolveActor(Handle)}; Actor != nullptr && !Actor->IsPendingDestroy()) {
                Actor->DispatchTick(DeltaTime, EditorTick);
            }
        }

        if (!EditorTick && !mEndPlayRequested && !mShutdownRequested) {
            UpdateOverlaps();
        }

        // 카메라는 이번 프레임의 이동과 겹침 처리가 모두 끝난 뒤에 갱신합니다.
        if (Engine::APlayerCameraManager* CameraManager{GetPlayerCameraManager()}; !EditorTick && !mEndPlayRequested && !mShutdownRequested && CameraManager != nullptr) {
            CameraManager->UpdateCamera(DeltaTime);
        }

        return true;
    }

    /**
     * @brief 충돌 도형의 겹침을 갱신하고 시작과 끝 이벤트를 전달합니다.
     */
    void Engine::UWorld::UpdateOverlaps() {
        Core::TArray<Engine::UShapeComponent*> Shapes{};

        if (mPersistentLevel != nullptr) {
            for (const auto& Actor : mPersistentLevel->GetActors()) {
                if (!Actor->IsPendingDestroy()) {
                    for (const auto& Component : Actor->GetComponents()) {
                        if (auto* Shape{CoreUObject::Cast<Engine::UShapeComponent>(Component.get())}) {
                            Shapes.push_back(Shape);
                        }
                    }
                }
            }
        }

        Core::TArray<std::pair<CoreUObject::TObjectRef<Engine::UShapeComponent>, CoreUObject::TObjectRef<Engine::UShapeComponent>>> Current{};

        for (Core::Index_t First{}; First < Shapes.size(); ++First) {
            for (Core::Index_t Second{static_cast<Core::Index_t>(First + 1)}; Second < Shapes.size(); ++Second) {
                if (Shapes[First]->GetOwner() != Shapes[Second]->GetOwner() && Shapes[First]->IsOverlapping(*Shapes[Second])) {
                    Current.emplace_back(Shapes[First], Shapes[Second]);
                }
            }
        }

        const auto Contains{[](const auto& Pairs, const auto& Pair) {
            return std::find(Pairs.begin(), Pairs.end(), Pair) != Pairs.end();
        }};
        const auto Resolve{[](CoreUObject::TObjectRef<Engine::UShapeComponent> Handle) {
            return CoreUObject::Cast<Engine::UShapeComponent>(Handle.Get());
        }};

        // 이벤트 안에서 액터가 파괴될 수 있으므로 호출할 때마다 핸들을 다시 해석합니다.
        const auto Previous{std::exchange(mOverlaps, Current)};

        for (const auto& Pair : Previous) {
            if (!Contains(Current, Pair)) {
                if (Engine::UShapeComponent* Shape{Resolve(Pair.first)}) {
                    Shape->OnEndOverlap(Resolve(Pair.second));
                }

                if (Engine::UShapeComponent* Shape{Resolve(Pair.second)}) {
                    Shape->OnEndOverlap(Resolve(Pair.first));
                }
            }
        }

        for (const auto& Pair : Current) {
            if (!Contains(Previous, Pair)) {
                Engine::UShapeComponent* First{Resolve(Pair.first)};
                Engine::UShapeComponent* Second{Resolve(Pair.second)};

                if (First != nullptr && Second != nullptr) {
                    First->OnBeginOverlap(Second);
                }

                First = Resolve(Pair.first);
                Second = Resolve(Pair.second);

                if (First != nullptr && Second != nullptr) {
                    Second->OnBeginOverlap(First);
                }
            }
        }
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

            for (const CoreUObject::TObjectRef<Engine::AActor> Handle : Pending) {
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
    Core::TArray<CoreUObject::TObjectRef<Engine::AActor>> Engine::UWorld::GetActorRefs() const {
        Core::TArray<CoreUObject::TObjectRef<Engine::AActor>> Handles{};

        if (mPersistentLevel != nullptr) {
            Handles.reserve(mPersistentLevel->GetActors().size());

            for (const auto& Actor : mPersistentLevel->GetActors()) {
                Handles.push_back(CoreUObject::MakeObjectRef(Actor));
            }
        }

        return Handles;
    }

    /**
     * @brief 전역 핸들을 해석하고 이 월드의 액터인지 확인합니다.
     */
    Engine::AActor* Engine::UWorld::ResolveActor(CoreUObject::TObjectRef<Engine::AActor> Handle) const {
        Engine::AActor* Actor{CoreUObject::Cast<Engine::AActor>(Handle.Get())};

        return Actor != nullptr && Actor->GetWorld() == this ? Actor : nullptr;
    }

}
