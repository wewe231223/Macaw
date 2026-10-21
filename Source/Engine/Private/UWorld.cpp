#include "Engine/UWorld.h"
#include "Engine/Component/UStaticMeshComponent.h"
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
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include <algorithm>
#include <limits>
#include <utility>

UWorld::UWorld() = default;

UWorld::~UWorld() {
    Shutdown();
}

ImplementReflection(UWorld, EClassFlags::NoFactory) {
    static_cast<void>(Information);
}

bool UWorld::SetPersistentLevel(std::unique_ptr<ULevel> Level) {
    if (mPersistentLevel != nullptr || Level == nullptr || Level->GetRegistry() != GetRegistry()) {
        return false;
    }

    mPersistentLevel = std::move(Level);
    mPersistentLevel->SetWorld(this);

    return true;
}

ULevel* UWorld::GetPersistentLevel() const {
    return mPersistentLevel.get();
}

bool UWorld::AttachScene(std::unique_ptr<FSceneInterface> Scene) {
    if (mScene != nullptr || Scene == nullptr) {
        return false;
    }

    mScene = std::move(Scene);

    if (mPersistentLevel != nullptr) {
        for (const auto& Actor : mPersistentLevel->GetActors()) {
            for (const auto& Component : Actor->GetComponents()) {
                if (auto* Primitive{Cast<UPrimitiveComponent>(Component.get())}) {
                    Primitive->MarkRenderStateDirty();
                }
            }
        }
    }

    SendAllEndOfFrameUpdates();

    return true;
}

FSceneInterface* UWorld::GetScene() const {
    return mScene.get();
}

AStaticMeshActor* UWorld::SpawnPrimitive(EPrimitiveType Type) {
    if (Type >= EPrimitiveType::Count || GetRegistry() == nullptr || mPersistentLevel == nullptr) {
        return nullptr;
    }

    auto Actor{NewObject<AStaticMeshActor>(*GetRegistry())};

    if (Actor == nullptr || !Actor->InitializeDefaultComponents()) {
        return nullptr;
    }

    Actor->SetName(PrimitiveName(Type) + "_" + std::to_string(++mSpawnSerial));
    Actor->GetStaticMeshComponent()->SetPrimitiveType(Type);
    AStaticMeshActor* Result{Actor.get()};

    return mPersistentLevel->AddActor(std::move(Actor)) ? Result : nullptr;
}

bool UWorld::DestroyActor(FObjectHandle Handle) {
    return mPersistentLevel != nullptr && mPersistentLevel->DestroyActor(Handle);
}

void UWorld::EnqueueRenderUpdate(FObjectHandle Handle) {
    mDirtyComponents.push_back(Handle);
}

void UWorld::SendAllEndOfFrameUpdates() {
    if (mScene == nullptr || GetRegistry() == nullptr) {
        return;
    }

    const std::size_t Count{mDirtyComponents.size()};

    for (std::size_t Index{}; Index < Count; ++Index) {
        const FObjectHandle Handle{mDirtyComponents[Index]};

        if (auto* Component{Cast<UPrimitiveComponent>(GetRegistry()->Resolve(Handle))}; Component != nullptr && Component->GetWorld() == this) {
            Component->SendRenderUpdates();
        }
    }

    mDirtyComponents.erase(mDirtyComponents.begin(), mDirtyComponents.begin() + static_cast<std::ptrdiff_t>(Count));
}

bool UWorld::Raycast(const FRay& Ray, FWorldHit& Hit) const {
    if (mPersistentLevel == nullptr || !Ray.mOrigin.IsFinite() || !Ray.mDirection.IsFinite() || Ray.mDirection.Length() < 0.000001f) {
        return false;
    }

    bool Found{};
    float Closest{std::numeric_limits<float>::max()};
    const FRay NormalizedRay{Ray.mOrigin, Ray.mDirection.Normalized()};

    for (const auto& Actor : mPersistentLevel->GetActors()) {
        for (const auto& Component : Actor->GetComponents()) {
            const auto* Primitive{Cast<UPrimitiveComponent>(Component.get())};

            if (Primitive == nullptr || !Primitive->GetWorldBounds().Intersects(NormalizedRay, Closest)) {
                continue;
            }

            FMatrix Inverse{};

            if (!Primitive->GetWorldMatrix().TryInverse(Inverse)) {
                continue;
            }

            const FRay Local{Inverse.TransformPosition(NormalizedRay.mOrigin), Inverse.TransformVector(NormalizedRay.mDirection)};
            const FMeshData& Mesh{GetPrimitiveMesh(Primitive->GetPrimitiveType())};

            for (std::size_t Index{}; Index + 2 < Mesh.mIndices.size(); Index += 3) {
                float Distance{};

                if (Math::IntersectTriangle(Local, Mesh.mVertices[Mesh.mIndices[Index]].mPosition, Mesh.mVertices[Mesh.mIndices[Index + 1]].mPosition, Mesh.mVertices[Mesh.mIndices[Index + 2]].mPosition, Distance) && Distance < Closest) {
                    Closest = Distance;
                    Hit = {Actor->GetHandle(), Primitive->GetHandle(), NormalizedRay.mOrigin + NormalizedRay.mDirection * Distance, Distance};
                    Found = true;
                }
            }
        }
    }

    return Found;
}

void UWorld::Shutdown() {
    if (mPersistentLevel != nullptr) {
        mPersistentLevel->SetWorld(nullptr);
    }

    mDirtyComponents.clear();
    mPersistentLevel.reset();
    mScene.reset();
}
