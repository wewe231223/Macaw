#include "pch.h"
#include "Editor/View/FPreviewScene.h"
#include "Asset/FAssetRegistry.h"
#include "Render/Renderer.h"
#include "World/ADirectionalLightActor.h"
#include "World/AStaticMeshActor.h"

FPreviewScene::~FPreviewScene() {
    Reset();
}

bool FPreviewScene::Initialize(FRenderer& Renderer) {
    const Uint64 Generation{FAssetRegistry::GetGeneration()};

    if (Generation == 0) {
        return false;
    }

    if (mRenderer == &Renderer && mRegistryGeneration == Generation && mWorld.IsInitialized()) {
        return true;
    }

    Reset();
    mRenderer = &Renderer;
    mRegistryGeneration = Generation;
    mWorld.Initialize(EWorldType::Preview);
    mWorld.BindScene(Renderer.CreateScene());

    AStaticMeshActor* MeshActor{mWorld.SpawnActorDeferred<AStaticMeshActor>()};
    ADirectionalLightActor* LightActor{mWorld.SpawnActorDeferred<ADirectionalLightActor>()};

    if (MeshActor == nullptr || LightActor == nullptr) {
        Reset();
        return false;
    }

    mMesh = MeshActor->GetStaticMeshComponent();
    mLight = LightActor->GetLightComponent();

    if (mMesh == nullptr || mLight == nullptr) {
        Reset();
        return false;
    }

    mMesh->SetVisible(false);

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
        mMesh->ClearMaterialOverrides();
    }

    if (const UMesh* Asset{mMesh->ResolveMesh()}; Asset != nullptr) {
        for (const FMeshMaterialSlot& Slot : Asset->GetMaterialSlots()) {
            const auto Position{std::ranges::find(Mesh.mMaterialOverrides, Slot.mName, &FMeshMaterialBinding::mSlot)};

            mMesh->SetMaterialOverride(Slot.mName, Position != Mesh.mMaterialOverrides.end() ? Position->mMaterial : FAssetHandle{});
        }
    }
    mMesh->SetWorldTransform(World);
    mMesh->SetVisible(static_cast<bool>(Mesh.mMeshHandle));
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
    mRegistryGeneration = 0;
}
