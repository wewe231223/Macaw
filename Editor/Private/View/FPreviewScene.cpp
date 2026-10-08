#include "pch.h"
#include "Editor/View/FPreviewScene.h"
#include "Asset/FAssetRegistry.h"
#include "Render/Renderer.h"
#include "World/Component/UDirectionalLightComponent.h"

FPreviewScene::~FPreviewScene() {
    Reset();
}

bool FPreviewScene::Initialize(FRenderer& Renderer, FAssetRegistry& Registry) {
    if (mRenderer == &Renderer && mRegistry == &Registry && mWorld.IsInitialized()) {
        return true;
    }

    Reset();
    mRenderer = &Renderer;
    mRegistry = &Registry;
    mWorld.Initialize(EWorldType::Preview);
    mWorld.SetAssetRegistry(&Registry, &Registry);
    mWorld.BindScene(Renderer.CreateScene());

    AActor* MeshActor{mWorld.SpawnActorDeferred<AActor>()};
    AActor* LightActor{mWorld.SpawnActorDeferred<AActor>()};

    if (MeshActor == nullptr || LightActor == nullptr) {
        Reset();
        return false;
    }

    mMesh = MeshActor->AddComponent<UStaticMeshComponent>();
    mLight = LightActor->AddComponent<UDirectionalLightComponent>();

    if (mMesh == nullptr || mLight == nullptr) {
        Reset();
        return false;
    }

    mMesh->SetVisible(false);
    MeshActor->SetRootComponent(mMesh);
    LightActor->SetRootComponent(mLight);

    if (!mWorld.FinishSpawningActor(MeshActor, FTransform{}) || !mWorld.FinishSpawningActor(LightActor, FTransform{})) {
        Reset();
        return false;
    }

    return true;
}

void FPreviewScene::SetMesh(const FMeshSceneData& Mesh, const FMatrix& World) {
    if (mMesh == nullptr) {
        return;
    }

    if (mMesh->GetMeshHandle() != Mesh.mMeshHandle) {
        mMesh->SetMeshHandle(Mesh.mMeshHandle);
    }

    mMesh->SetMaterialHandle(Mesh.mMaterialHandle);
    mMesh->SetPipelineHandle(Mesh.mPipelineHandle);
    mMesh->SetWorldTransform(World);
    mMesh->SetVisible(static_cast<bool>(Mesh.mMeshHandle) && static_cast<bool>(Mesh.mMaterialHandle) && static_cast<bool>(Mesh.mPipelineHandle));
}

void FPreviewScene::SetLight(const FVector3& Direction, const FVector3& Color, float Intensity) {
    if (mLight == nullptr || Direction.LengthSquared() <= 1e-8f) {
        return;
    }

    FVector3 Forward{Direction};

    Forward.Normalize();

    const FVector3 Reference{std::abs(Forward.mZ) < 0.99f ? FVector::UnitZ : FVector::UnitY};
    FVector3 Right{Reference.Cross(Forward)};

    Right.Normalize();

    const FVector3 Up{Forward.Cross(Right)};
    FMatrix World{};

    World.SetAxis(0, Forward);
    World.SetAxis(1, Right);
    World.SetAxis(2, Up);
    mLight->SetWorldTransform(World);
    mLight->SetLightColor(Color);
    mLight->SetIntensity(Intensity);
}

const FRenderScene* FPreviewScene::Synchronize() {
    if (mRenderer == nullptr || !mWorld.IsInitialized()) {
        return nullptr;
    }

    if (!mWorld.GetSceneHandle().IsValid()) {
        mWorld.BindScene(mRenderer->CreateScene());
    }

    if (!mWorld.SendSceneUpdates()) {
        return nullptr;
    }

    return mRenderer->FindScene(mWorld.GetSceneHandle());
}

void FPreviewScene::Reset() {
    mWorld.CleanupWorld();
    mMesh = nullptr;
    mLight = nullptr;
    mRenderer = nullptr;
    mRegistry = nullptr;
}
