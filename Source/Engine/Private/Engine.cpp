/**
 * @file Engine.cpp
 * @brief 현재 월드 컨텍스트와 렌더러 연결을 관리합니다.
 * @ingroup Engine
 */

#include "Engine/Engine.h"
#include "Engine/EngineStats.h"
#include "Engine/ULevel.h"
#include "Engine/FProject.h"
#include "Engine/FSceneInterface.h"
#include "Engine/UWorld.h"
#include "Engine/FWorldContext.h"
#include "Engine/FWorldCloner.h"
#include "Engine/EPrimitiveType.h"
#include "CoreUObject/FObjectDuplicationContext.h"
#include "RenderCore/IRendererModule.h"
#include "Engine/Reflection/RegisterEngine.h"
#include "Engine/PrimitiveMesh.h"
#include "Engine/AActor.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"

#include <atomic>
#include <cmath>
#include <utility>

namespace Engine {

    /**
     * @brief 엔진 인스턴스 사이에서 공유하는 월드 세대 발급 상태입니다.
     */
    namespace {
        /**
         * @brief 프로세스 내 엔진 및 월드 슬롯 재사용을 구분하는 다음 세대입니다.
         */
        std::atomic<Core::Uint64> NextWorldGeneration{1};
    }

    /**
     * @brief UEngine의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Engine::UEngine::UEngine() = default;

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Engine::UEngine::~UEngine() {
        Shutdown();
    }

    void Engine::UEngine::SetProject(Engine::FProject* Project) {
        if (!mInitialized && !IsDispatching()) {
            mProject = Project;
        }
    }

    Engine::FProject* Engine::UEngine::GetProject() const {
        return mProject;
    }

    /**
     * @brief 렌더러 없이 초기 편집 월드와 리플렉션 기반을 준비합니다.
     */
    bool Engine::UEngine::Initialize() {
        if (mInitialized || IsDispatching()) {
            return false;
        }

        Engine::ReflectionModules::RegisterEngine();
        StaticClass();
        mInitialized = true;
        mShutdownRequested = false;
        mCurrentWorld = CreateWorld(Engine::EWorldType::Editor);

        if (!mCurrentWorld.IsValid()) {
            Shutdown();
            return false;
        }

        ++mWorldGeneration;

        return true;
    }

    /**
     * @brief 렌더러를 연결하고 초기 월드 컨텍스트와 장면을 준비합니다.
     */
    bool Engine::UEngine::Initialize(RenderCore::IRendererModule* Renderer) {
        if (Renderer == nullptr || mInitialized) {
            return false;
        }

        for (Core::Uint32 Index{}; Index < static_cast<Core::Uint32>(Engine::EPrimitiveType::Count); ++Index) {
            if (!Renderer->UploadMesh(static_cast<Engine::EPrimitiveType>(Index), Engine::GetPrimitiveMesh(static_cast<Engine::EPrimitiveType>(Index)))) {
                return false;
            }
        }

        mRenderer = Renderer;

        std::size_t GeometryBytes{};

        for (Core::Uint32 Index{}; Index < static_cast<Core::Uint32>(Engine::EPrimitiveType::Count); ++Index) {
            GeometryBytes += Engine::GetPrimitiveMesh(static_cast<Engine::EPrimitiveType>(Index)).GetAllocatedBytes();
        }

        Engine::EngineStats::SharedGeometry.Set(static_cast<double>(GeometryBytes));

        if (!Initialize()) {
            mRenderer = nullptr;
            return false;
        }

        return true;
    }

    /**
     * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
     */
    void Engine::UEngine::Shutdown() {
        if (IsDispatching()) {
            mShutdownRequested = true;
            return;
        }

        mInitialized = false;
        mShutdownRequested = false;
        mUpdating = true;

        for (const auto& Context : mWorldContexts) {
            if (Context != nullptr) {
                Context->GetWorld()->Shutdown();
            }
        }

        mWorldContexts.clear();
        mUpdating = false;
        mCurrentWorld = {};
        mRenderer = nullptr;
    }

    /**
     * @brief 등록된 월드의 핸들 스냅샷을 순회하여 실행을 갱신합니다.
     */
    void Engine::UEngine::Tick(float DeltaTime, bool TickEditorWorlds) {
        if (!mInitialized || IsDispatching() || !std::isfinite(DeltaTime) || DeltaTime < 0.0f) {
            return;
        }

        if (mShutdownRequested) {
            Shutdown();
            return;
        }

        const Core::FScopedStatTimer StatTimer{Engine::EngineStats::EngineTick};

        mUpdating = true;

        const auto Handles{GetWorldHandles()};

        for (const Engine::FWorldHandle Handle : Handles) {
            if (mShutdownRequested) {
                break;
            }

            if (Engine::UWorld* World{GetWorld(Handle)}) {
                if (!TickEditorWorlds && (World->GetWorldType() == Engine::EWorldType::Editor || World->GetWorldType() == Engine::EWorldType::EditorPreview)) {
                    continue;
                }

                World->Tick(DeltaTime);
            }
        }

        mUpdating = false;

        std::size_t WorldCount{};
        std::size_t ActorCount{};
        std::size_t ComponentCount{};

        for (const Engine::FWorldHandle Handle : GetWorldHandles()) {
            const Engine::UWorld* World{GetWorld(Handle)};

            if (World == nullptr) {
                continue;
            }

            ++WorldCount;

            if (World->GetPersistentLevel() != nullptr) {
                for (const auto& Actor : World->GetPersistentLevel()->GetActors()) {
                    ++ActorCount;
                    ComponentCount += Actor->GetComponents().size();
                }
            }
        }

        Engine::EngineStats::Worlds.Set(static_cast<double>(WorldCount));
        Engine::EngineStats::Actors.Set(static_cast<double>(ActorCount));
        Engine::EngineStats::Components.Set(static_cast<double>(ComponentCount));

        if (mShutdownRequested) {
            Shutdown();
        }
    }

    /**
     * @brief 지정한 용도의 월드 컨텍스트를 생성하여 엔진이 소유합니다.
     */
    Engine::FWorldHandle Engine::UEngine::CreateWorld(Engine::EWorldType Type) {
        if (!mInitialized || IsDispatching() || Type > Engine::EWorldType::EditorPreview) {
            return {};
        }

        return AdoptWorld(std::make_unique<Engine::FWorldContext>(Type));
    }

    /**
     * @brief 현재 월드 선택을 유지하면서 준비한 컨텍스트의 소유권을 인수합니다.
     */
    Engine::FWorldHandle Engine::UEngine::RegisterWorld(std::unique_ptr<Engine::FWorldContext> Context) {
        return IsDispatching() ? Engine::FWorldHandle{} : AdoptWorld(std::move(Context));
    }

    /**
     * @brief 명시적인 세대 핸들로 엔진 소유 컨텍스트를 조회합니다.
     */
    Engine::FWorldContext* Engine::UEngine::GetWorldContext(Engine::FWorldHandle Handle) const {
        return FindContext(Handle);
    }

    /**
     * @brief 필요한 렌더 장면을 준비하고 컨텍스트를 빈 슬롯에 인수합니다.
     */
    Engine::FWorldHandle Engine::UEngine::AdoptWorld(std::unique_ptr<Engine::FWorldContext> Context) {
        if (!mInitialized || Context == nullptr || Context->GetWorld() == nullptr || Context->GetHandle().IsValid() || Context->GetWorld()->IsDispatching()) {
            return {};
        }

        Context->GetWorld()->SetAssets(mProject != nullptr ? mProject->GetAssets() : nullptr);

        if (mRenderer != nullptr && !Context->GetWorld()->AttachScene(mRenderer->AllocateScene())) {
            return {};
        }

        Core::Uint32 Index{};

        while (Index < mWorldContexts.size() && mWorldContexts[Index] != nullptr) {
            ++Index;
        }

        const Core::Uint64 Generation{NextWorldGeneration.fetch_add(1, std::memory_order_relaxed)};
        const Engine::FWorldHandle Handle{Index, Generation};
        Context->SetHandle(Handle);

        if (Index == mWorldContexts.size()) {
            mWorldContexts.push_back(std::move(Context));
        } else {
            mWorldContexts[Index] = std::move(Context);
        }

        return Handle;
    }

    /**
     * @brief 핸들을 검증하고 해당 월드의 실행과 소유 리소스를 종료합니다.
     */
    bool Engine::UEngine::DestroyWorld(Engine::FWorldHandle Handle) {
        Engine::FWorldContext* Context{FindContext(Handle)};

        if (Context == nullptr || IsDispatching()) {
            return false;
        }

        mUpdating = true;
        Context->GetWorld()->Shutdown();
        mWorldContexts[Handle.GetIndex()].reset();
        mUpdating = false;

        if (mCurrentWorld == Handle) {
            mCurrentWorld = {};
            ++mWorldGeneration;
        }

        return true;
    }

    /**
     * @brief 원본과 독립된 정지 상태의 월드를 복제하여 엔진에 등록합니다.
     */
    Engine::FWorldHandle Engine::UEngine::DuplicateWorld(Engine::FWorldHandle Source, Engine::EWorldType Type, Core::FString& Error, CoreUObject::FObjectDuplicationContext* Duplication) {
        Error.clear();

        if (Duplication != nullptr) {
            Duplication->Clear();
        }

        const Engine::FWorldContext* Context{FindContext(Source)};

        if (Context == nullptr || IsDispatching()) {
            Error = "Source world is invalid or dispatching";
            return {};
        }

        CoreUObject::FObjectDuplicationContext Objects{};

        mUpdating = true;

        auto Duplicate{Engine::FWorldCloner::Clone(Context, Type, &Objects, Error)};

        mUpdating = false;

        if (mShutdownRequested) {
            Shutdown();
            Error = "World duplication was cancelled by shutdown";
            return {};
        }

        if (Duplicate == nullptr) {
            return {};
        }

        const Engine::FWorldHandle Handle{AdoptWorld(std::move(Duplicate))};

        if (!Handle.IsValid()) {
            Error = "Could not attach the duplicated world";
            return {};
        }

        if (Duplication != nullptr) {
            *Duplication = std::move(Objects);
        }

        return Handle;
    }

    /**
     * @brief 슬롯 범위와 세대가 일치하는 소유 컨텍스트를 찾습니다.
     */
    Engine::FWorldContext* Engine::UEngine::FindContext(Engine::FWorldHandle Handle) const {
        if (!Handle.IsValid() || Handle.GetIndex() >= mWorldContexts.size()) {
            return nullptr;
        }

        Engine::FWorldContext* Context{mWorldContexts[Handle.GetIndex()].get()};

        return Context != nullptr && Context->GetHandle() == Handle ? Context : nullptr;
    }

    /**
     * @brief 현재 소속되거나 관리하는 월드의 비소유 주소를 반환합니다.
     */
    Engine::UWorld* Engine::UEngine::GetWorld(Engine::FWorldHandle Handle) const {
        const Engine::FWorldContext* Context{FindContext(Handle)};
        return Context != nullptr ? Context->GetWorld() : nullptr;
    }

    /**
     * @brief 현재 등록된 모든 월드의 핸들 스냅샷을 반환합니다.
     */
    Core::TArray<Engine::FWorldHandle> Engine::UEngine::GetWorldHandles() const {
        Core::TArray<Engine::FWorldHandle> Handles{};

        for (const auto& Context : mWorldContexts) {
            if (Context != nullptr) {
                Handles.push_back(Context->GetHandle());
            }
        }

        return Handles;
    }

    /**
     * @brief 월드 경계를 검증한 뒤 해당 월드에 액터 삭제를 요청합니다.
     */
    bool Engine::UEngine::DestroyActor(CoreUObject::FObjectHandle Handle) {
        Engine::AActor* Actor{CoreUObject::Cast<Engine::AActor>(CoreUObject::FObjectRegistry::Get()->Resolve(Handle))};
        Engine::UWorld* World{Actor != nullptr ? Actor->GetWorld() : nullptr};
        return World != nullptr && GetWorld(World->GetWorldHandle()) == World && World->DestroyActor(Handle);
    }

    /**
     * @brief 기존 편집 경로에서 사용하는 현재 월드 선택을 반환합니다.
     */
    Engine::FWorldHandle Engine::UEngine::GetWorldHandle() const {
        return mCurrentWorld;
    }

    /**
     * @brief 이 엔진이 소유한 월드를 현재 월드로 선택합니다.
     */
    bool Engine::UEngine::SetCurrentWorld(Engine::FWorldHandle Handle) {
        if (FindContext(Handle) == nullptr || IsDispatching()) {
            return false;
        }

        if (!(mCurrentWorld == Handle)) {
            mCurrentWorld = Handle;
            ++mWorldGeneration;
        }

        return true;
    }

    /**
     * @brief 현재 소속되거나 관리하는 월드의 비소유 주소를 반환합니다.
     */
    Engine::UWorld* Engine::UEngine::GetWorld() const {
        return GetWorld(mCurrentWorld);
    }

    /**
     * @brief 현재 월드를 소유한 컨텍스트를 반환합니다.
     */
    Engine::FWorldContext* Engine::UEngine::GetWorldContext() const {
        return FindContext(mCurrentWorld);
    }

    /**
     * @brief 연결된 렌더러의 비소유 주소를 반환합니다.
     */
    RenderCore::IRendererModule* Engine::UEngine::GetRenderer() const {
        return mRenderer;
    }

    /**
     * @brief 새 월드 컨텍스트를 검증하고 장면을 연결한 뒤 현재 월드를 교체합니다.
     */
    bool Engine::UEngine::ReplaceWorld(std::unique_ptr<Engine::FWorldContext> Context) {
        if (IsDispatching()) {
            return false;
        }

        const Engine::FWorldHandle Previous{mCurrentWorld};
        const Engine::FWorldHandle Replacement{AdoptWorld(std::move(Context))};

        if (!Replacement.IsValid()) {
            return false;
        }

        if (Previous.IsValid()) {
            DestroyWorld(Previous);
        }

        mCurrentWorld = Replacement;
        ++mWorldGeneration;

        return true;
    }

    /**
     * @brief 월드 교체 시 증가하는 컨텍스트 세대를 반환합니다.
     */
    Core::Uint64 Engine::UEngine::GetWorldGeneration() const {
        return mWorldGeneration;
    }

    /**
     * @brief 월드의 프레임 말 렌더 갱신을 모두 반영합니다.
     */
    void Engine::UEngine::SynchronizeWorld() {
        const Core::FScopedStatTimer StatTimer{Engine::EngineStats::Synchronize};

        for (const Engine::FWorldHandle Handle : GetWorldHandles()) {
            if (Engine::UWorld* World{GetWorld(Handle)}) {
                World->SendAllEndOfFrameUpdates();
            }
        }
    }

    /**
     * @brief 엔진 갱신 또는 소유 월드의 콜백 처리가 진행 중인지 검사합니다.
     */
    bool Engine::UEngine::IsDispatching() const {
        if (mUpdating) {
            return true;
        }

        for (const auto& Context : mWorldContexts) {
            if (Context != nullptr && Context->GetWorld()->IsDispatching()) {
                return true;
            }
        }

        return false;
    }

}
