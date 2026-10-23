#include "pch.h"
#include "Core/Base/ErrorHandler.h"
#include "Core/Base/FScopedFlag.h"
#include "Engine/UEngine.h"
#include "Asset/FAssetRegistry.h"
#include "Render/Renderer.h"
#include "Core/Spatial/FBVH8.h"
#include "World/UWorld.h"
#include "World/ULevel.h"
#include <algorithm>
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "Asset/UTexture.h"
#include "Asset/UFont.h"
#include "Asset/UFreeTypeFont.h"
#include "World/Component/UBoxColliderComponent.h"
#include "World/Component/UDirectionalLightComponent.h"
#include "World/Component/UPointLightComponent.h"
#include "World/Component/USpotLightComponent.h"
#include "World/Component/UBillboardComponent.h"
#include "World/Component/UBillboardTextComponent.h"
#include "World/Component/UUVScrollComponent.h"
#include "World/Component/UFlipBookComponent.h"
#include "World/Component/USubUVComponent.h"

namespace {
    UEngine* EngineInstance{};
}

UEngine* GetEngine() noexcept {
    return EngineInstance;
}

UEngine::UEngine() {
    InitializeClass(StaticClass());
}

UEngine::~UEngine() {
    Shutdown();
}

void UEngine::Initialize() {
    if (mInitialized || mShuttingDown) {
        return;
    }

    ErrorHandler::Report(EngineInstance != nullptr && EngineInstance != this, "UEngine", "Another engine is already active", ErrorHandler::EErrorLevel::Critical);

    BVH8::Initialize();
    RegisterObjectTypes();
    ErrorHandler::Report(!UObjectSystem::Register(this).IsValid(), "UEngine", "Cannot register engine", ErrorHandler::EErrorLevel::Critical);
    mAssetRegistry = std::make_unique<FAssetRegistry>();
    mSubsystems.Initialize(*this);
    mAssetRegistry->ActivateGlobal();
    EngineInstance = this;
    mInitialized = true;
}

void UEngine::Shutdown() {
    if (mShuttingDown) {
        return;
    }

    if (mTicking) {
        ErrorHandler::Report("UEngine", "Cannot shut down the engine during a world tick", ErrorHandler::EErrorLevel::Critical);
    }

    const FScopedFlag ShutdownScope{mShuttingDown};

    mInitialized = false;
    mWorldContexts.clear();
    mSubsystems.Deinitialize();
    mAssetRegistry.reset();
    mRenderer.reset();
    UObjectSystem::Unregister(this, GetHandle());

    if (EngineInstance == this) {
        EngineInstance = nullptr;
    }
}

bool UEngine::IsInitialized() const {
    return mInitialized;
}

void UEngine::Tick(float DeltaTime) {
    if (!mInitialized || mTicking) {
        return;
    }

    const FScopedFlag TickScope{mTicking};

    const std::size_t Count{mWorldContexts.size()};

    for (std::size_t Index{}; Index < Count; ++Index) {
        mWorldContexts[Index]->GetWorld().Tick(GetWorldTickType(*mWorldContexts[Index]), DeltaTime);
    }
}

bool UEngine::IsTicking() const {
    return mTicking;
}

bool UEngine::CanChangeWorldContexts() const {
    if (mTicking) {
        return false;
    }

    return std::ranges::none_of(mWorldContexts, [](const std::unique_ptr<FWorldContext>& Context) {
        return Context->GetWorld().IsBusy();
    });
}

ETickType UEngine::GetWorldTickType(const FWorldContext& Context) const {
    const UWorld& World{Context.GetWorld()};

    return World.GetTime().IsPaused() ? ETickType::Paused : World.GetDefaultTickType();
}

FWorldContext* UEngine::AdoptWorld(std::unique_ptr<UWorld>& World) {
    if (!mInitialized || mTicking || World == nullptr || !World->IsInitialized() || World->IsBusy()) {
        return nullptr;
    }

    std::unique_ptr<FWorldContext> Context{std::make_unique<FWorldContext>(std::move(World))};
    FWorldContext* Result{Context.get()};

    if (mRenderer != nullptr && !Result->GetWorld().GetSceneHandle().IsValid()) {
        Result->GetWorld().BindScene(mRenderer->CreateScene());
    }

    mWorldContexts.push_back(std::move(Context));

    return Result;
}

FWorldContext& UEngine::CreateWorldContext(EWorldType WorldType) {
    if (!mInitialized) {
        ErrorHandler::Report("UEngine", "Engine is not initialized", ErrorHandler::EErrorLevel::Critical);
    }

    std::unique_ptr<FWorldContext> Context{std::make_unique<FWorldContext>(WorldType)};

    FWorldContext& Result{*Context};

    if (mRenderer != nullptr) {
        Result.GetWorld().BindScene(mRenderer->CreateScene());
    }

    mWorldContexts.push_back(std::move(Context));

    return Result;
}

bool UEngine::DestroyWorldContext(FWorldContext& Context) {
    if (mTicking) {
        return false;
    }

    const auto Iterator{std::ranges::find_if(mWorldContexts, [&Context](const std::unique_ptr<FWorldContext>& Candidate) {
        return Candidate.get() == &Context;
    })};

    if (Iterator == mWorldContexts.end()) {
        return false;
    }

    if (Context.GetWorld().IsBusy()) {
        return false;
    }

    std::unique_ptr<FWorldContext> RemovedContext{std::move(*Iterator)};

    mWorldContexts.erase(Iterator);

    return true;
}

const std::vector<std::unique_ptr<FWorldContext>>& UEngine::GetWorldContexts() const {
    return mWorldContexts;
}

TSubsystemCollection<UEngineSubsystem, UEngine>& UEngine::GetSubsystems() {
    return mSubsystems;
}

bool UEngine::CreateRenderer(HWND WindowHandle, UINT Width, UINT Height) {
    if (mRenderer != nullptr || mInitialized || mShuttingDown || WindowHandle == nullptr || Width == 0 || Height == 0) {
        return false;
    }

    std::unique_ptr<FRenderer> Renderer{std::make_unique<FRenderer>()};

    if (!Renderer->Create(WindowHandle, Width, Height)) {
        return false;
    }

    mRenderer = std::move(Renderer);

    return true;
}

FRenderer* UEngine::GetRenderer() const {
    return mRenderer.get();
}

void UEngine::RegisterObjectTypes() {
    ReflectionModules::RegisterCoreUObject();
    ReflectionModules::RegisterRenderCore();
    ReflectionModules::RegisterAsset();
    ReflectionModules::RegisterWorld();
    ReflectionModules::RegisterEngine();
}
