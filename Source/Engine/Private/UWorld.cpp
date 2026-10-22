/**
 * @file UWorld.cpp
 * @brief 영속 레벨, 렌더 장면과 프레임 말 컴포넌트 갱신을 관리합니다.
 * @ingroup Engine
 */

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

#include <algorithm>
#include <limits>
#include <utility>

/**
 * @brief UWorld의 초기 상태와 전달받은 의존성을 구성합니다.
 */
UWorld::UWorld() = default;

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
UWorld::~UWorld() {
    Shutdown();
}

/**
 * @brief 아직 레벨이 없는 월드에 영속 레벨의 소유권을 연결합니다.
 */
bool UWorld::SetPersistentLevel(std::unique_ptr<ULevel> Level) {
    if (mPersistentLevel != nullptr || Level == nullptr || Level->GetRegistry() != GetRegistry()) {
        return false;
    }

    mPersistentLevel = std::move(Level);
    mPersistentLevel->SetWorld(this);

    return true;
}

/**
 * @brief 월드가 소유한 영속 레벨을 반환합니다.
 */
ULevel* UWorld::GetPersistentLevel() const {
    return mPersistentLevel.get();
}

/**
 * @brief 월드에 렌더 장면을 연결하고 기존 프리미티브를 동기화합니다.
 */
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

/**
 * @brief 연결된 렌더 장면을 반환합니다.
 */
FSceneInterface* UWorld::GetScene() const {
    return mScene.get();
}

/**
 * @brief 지정한 기본 메시 종류의 액터를 생성하여 영속 레벨에 추가합니다.
 */
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

/**
 * @brief 핸들에 해당하는 액터의 컴포넌트를 등록 해제하고 액터를 제거합니다.
 */
bool UWorld::DestroyActor(FObjectHandle Handle) {
    return mPersistentLevel != nullptr && mPersistentLevel->DestroyActor(Handle);
}

/**
 * @brief 컴포넌트 핸들을 프레임 말 렌더 갱신 큐에 추가합니다.
 */
void UWorld::EnqueueRenderUpdate(FObjectHandle Handle) {
    mDirtyComponents.push_back(Handle);
}

/**
 * @brief 큐에 쌓인 컴포넌트 핸들을 해석하여 지연된 렌더 갱신을 반영합니다.
 */
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

/**
 * @brief 월드 프리미티브의 메시를 검사하여 광선에 가장 가까운 충돌을 찾습니다.
 */
bool UWorld::Raycast(const FRay& Ray, FWorldHit& Hit) const {
    FRay NormalizedRay{Ray};

    if (mPersistentLevel == nullptr || !NormalizedRay.mOrigin.IsFinite() || !NormalizedRay.mDirection.Normalize()) {
        return false;
    }

    bool Found{};
    float Closest{std::numeric_limits<float>::max()};

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

/**
 * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
 */
void UWorld::Shutdown() {
    if (mPersistentLevel != nullptr) {
        mPersistentLevel->SetWorld(nullptr);
    }

    mDirtyComponents.clear();
    mPersistentLevel.reset();
    mScene.reset();
}
