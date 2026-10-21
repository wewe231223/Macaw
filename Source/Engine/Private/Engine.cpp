#include "Engine/Engine.h"
#include "Engine/FSceneInterface.h"
#include "Engine/UWorld.h"
#include "Engine/FWorldContext.h"
#include "Engine/EPrimitiveType.h"
#include "RenderCore/IRendererModule.h"
#include "Engine/Reflection/RegisterEngine.h"
#include "Engine/PrimitiveMesh.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"

UEngine::UEngine() = default;

UEngine::~UEngine() {
    Shutdown();
}

ImplementReflection(UEngine, EClassFlags::NoFactory) {
    static_cast<void>(Information);
}

bool UEngine::Initialize(IRendererModule& Renderer) {
    if (mWorldContext != nullptr) {
        return false;
    }

    ReflectionModules::RegisterEngine();
    StaticClass();

    for (Uint32 Index{}; Index < static_cast<Uint32>(EPrimitiveType::Count); ++Index) {
        const auto Type{static_cast<EPrimitiveType>(Index)};

        if (!Renderer.UploadMesh(Type, GetPrimitiveMesh(Type))) {
            return false;
        }
    }

    mRenderer = &Renderer;

    return ReplaceWorld(std::make_unique<FWorldContext>());
}

void UEngine::Shutdown() {
    mWorldContext.reset();
    mRenderer = nullptr;
}

UWorld* UEngine::GetWorld() const {
    return mWorldContext != nullptr ? mWorldContext->GetWorld() : nullptr;
}

FWorldContext* UEngine::GetWorldContext() const {
    return mWorldContext.get();
}

IRendererModule* UEngine::GetRenderer() const {
    return mRenderer;
}

bool UEngine::ReplaceWorld(std::unique_ptr<FWorldContext> Context) {
    if (Context == nullptr || Context->GetWorld() == nullptr || mRenderer == nullptr) {
        return false;
    }

    if (!Context->GetWorld()->AttachScene(mRenderer->AllocateScene())) {
        return false;
    }

    mWorldContext = std::move(Context);
    ++mWorldGeneration;

    return true;
}

Uint64 UEngine::GetWorldGeneration() const {
    return mWorldGeneration;
}

void UEngine::SynchronizeWorld() {
    if (GetWorld() != nullptr) {
        GetWorld()->SendAllEndOfFrameUpdates();
    }
}
