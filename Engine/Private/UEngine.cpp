#include "pch.h"
#include "Core/Base/ErrorHandler.h"
#include "Engine/UEngine.h"
#include "Asset/FAssetRegistry.h"
#include "Core/Spatial/FBVH8.h"
#include "World/UWorld.h"
#include "World/ULevel.h"
#include <algorithm>
#include "CoreUObject/TypeRegistry.h"
#include "Asset/UTexture.h"
#include "Asset/UFont.h"
#include "Asset/UFreeTypeFont.h"
#include "World/Component/UBoxColliderComponent.h"
#include "World/Component/UDirectionalLightComponent.h"
#include "World/Component/UPointLightComponent.h"
#include "World/Component/USpotLightComponent.h"
#include "World/Component/UBillboardComponent.h"
#include "World/Component/UBillboardTextComponent.h"
#include "World/Component/UNameTagComponent.h"
#include "World/Component/UScrollUVComponent.h"
#include "World/Component/USubUVComponent.h"

UEngine::UEngine() = default;

UEngine::~UEngine() {
    Shutdown();
}

void UEngine::Initialize() {
    if (mInitialized) {
        return;
    }

    BVH8::Initialize();
    RegisterObjectTypes();
    ErrorHandler::Report(!UObjectSystem::Register(this).IsValid(), "UEngine", "Cannot register engine", ErrorHandler::EErrorLevel::Critical);
    mAssetRegistry = std::make_unique<FAssetRegistry>();
    mSubsystems.Initialize(*this);
    mInitialized = true;
}

void UEngine::Shutdown() {
    if (mTicking) {
        ErrorHandler::Report("UEngine", "Cannot shut down the engine during a world tick", ErrorHandler::EErrorLevel::Critical);
    }

    mInitialized = false;
    mWorldContexts.clear();
    mSubsystems.Deinitialize();
    mAssetRegistry.reset();
    UObjectSystem::Unregister(this, GetHandle());
}

bool UEngine::IsInitialized() const {
    return mInitialized;
}

void UEngine::Tick(float DeltaTime) {
    if (!mInitialized || mTicking) {
        return;
    }

    mTicking = true;

    const std::size_t Count{mWorldContexts.size()};

    for (std::size_t Index{}; Index < Count; ++Index) {
        mWorldContexts[Index]->GetWorld().Tick(DeltaTime);
    }

    mTicking = false;
}

FWorldContext& UEngine::CreateWorldContext(EWorldType WorldType) {
    if (!mInitialized) {
        ErrorHandler::Report("UEngine", "Engine is not initialized", ErrorHandler::EErrorLevel::Critical);
    }

    std::unique_ptr<FWorldContext> Context{std::make_unique<FWorldContext>(WorldType)};

    Context->GetWorld().SetAssetRegistry(mAssetRegistry.get(), mAssetRegistry.get());

    FWorldContext& Result{*Context};

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

    mWorldContexts.erase(Iterator);

    return true;
}

const std::vector<std::unique_ptr<FWorldContext>>& UEngine::GetWorldContexts() const {
    return mWorldContexts;
}

FAssetRegistry& UEngine::GetAssetRegistry() {
    if (mAssetRegistry == nullptr) {
        ErrorHandler::Report("UEngine", "Engine is not initialized", ErrorHandler::EErrorLevel::Critical);
    }

    return *mAssetRegistry;
}

TSubsystemCollection<UEngineSubsystem, UEngine>& UEngine::GetSubsystems() {
    return mSubsystems;
}

void UEngine::RegisterObjectTypes() {
    TypeRegistry::Register(UObject::StaticTypeInfo());
    TypeRegistry::Register(UAsset::StaticTypeInfo());
    TypeRegistry::Register(UMesh::StaticTypeInfo());
    TypeRegistry::Register(UPipeline::StaticTypeInfo());
    TypeRegistry::Register(UTexture::StaticTypeInfo());
    TypeRegistry::Register(AActor::StaticTypeInfo());
    TypeRegistry::Register(UFont::StaticTypeInfo());
    TypeRegistry::Register(UFreeTypeFont::StaticTypeInfo());
    TypeRegistry::Register(UWorld::StaticTypeInfo());
    TypeRegistry::Register(ULevel::StaticTypeInfo());
    TypeRegistry::Register(UEngine::StaticTypeInfo());
    TypeRegistry::Register(UCameraComponent::StaticTypeInfo());
    TypeRegistry::Register(UStaticMeshComponent::StaticTypeInfo());
    TypeRegistry::Register(UCollisionComponent::StaticTypeInfo());
    TypeRegistry::Register(UBoxColliderComponent::StaticTypeInfo());
    TypeRegistry::Register(UDirectionalLightComponent::StaticTypeInfo());
    TypeRegistry::Register(UPointLightComponent::StaticTypeInfo());
    TypeRegistry::Register(USpotLightComponent::StaticTypeInfo());
    TypeRegistry::Register(UActorComponent::StaticTypeInfo());
    TypeRegistry::Register(USceneComponent::StaticTypeInfo());
    TypeRegistry::Register(UBillboardTextComponent::StaticTypeInfo());
    TypeRegistry::Register(UNameTagComponent::StaticTypeInfo());
    TypeRegistry::Register(UBillboardComponent::StaticTypeInfo());
    TypeRegistry::Register(USubUVComponent::StaticTypeInfo());
    TypeRegistry::Register(UScrollUVComponent::StaticTypeInfo());
}
