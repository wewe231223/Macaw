#include "PCH.h"
#include "FActorScene.h"

#include <ranges>

#include "Core/Base/UObjectSystem.h"
#include "Scene/Component/UCollisionComponent.h"
#include "Scene/Component/UStaticMeshComponent.h"
#include "Scene/UWorld.h"

FActorScene::FActorScene(UWorld *InWorld)
	: World(InWorld) {
}

FActorScene::~FActorScene() {
	for (const std::unique_ptr<AActor> &Actor : Actors) {
		UObjectSystem::Unregister(Actor.get(), Actor->GetHandle());
	}

	Actors.clear();
}

AActor *FActorScene::AddActor(std::unique_ptr<AActor> InActor) {
	if (!InActor) {
		return nullptr;
	}

	AActor *Actor = InActor.get();
	if (UObjectSystem::Resolve(Actor->GetHandle()) != Actor) {
		UObjectSystem::Register(Actor);
	}

	Actors.push_back(std::move(InActor));
	Actor->SetWorld(World);
	return Actor;
}

AActor *FActorScene::AddLoadedActor(std::unique_ptr<AActor> InActor, const FGuid &Guid, FArchive &Archive) {
	if (!InActor) {
		return nullptr;
	}

	AActor *Actor = InActor.get();
	UObjectSystem::RegisterWithGuid(Actor, Guid);
	Actor->PreLoadComponents(Archive);
	Actor->SetWorld(World);
	Actors.push_back(std::move(InActor));
	return Actor;
}

bool FActorScene::SpawnActor(const FAssetHandle &MeshHandle, const FAssetHandle &PipelineHandle, const FAssetHandle &MaterialHandle, const FVector3 &Position, UMesh *Mesh) {
	AActor *Actor = AddActor(std::make_unique<AActor>());

	UStaticMeshComponent *MeshComponent = Actor->AddComponent<UStaticMeshComponent>();
	UCollisionComponent *CollisionComponent = Actor->AddComponent<UCollisionComponent>();

	Actor->SetRootComponent(MeshComponent);
	CollisionComponent->AttachTo(MeshComponent);

	MeshComponent->SetMeshHandle(MeshHandle);
	MeshComponent->SetPipelineHandle(PipelineHandle);
	MeshComponent->SetMaterialHandle(MaterialHandle);
	MeshComponent->GetTransform().SetPosition(FVector3{Position.x, Position.y, Position.z});

	CollisionComponent->SetBounds(Mesh->GetLocalBoundingBox());
	return true;
}

bool FActorScene::DestroyActor(AActor *Actor) {
	if (Actor == nullptr) {
		return false;
	}

	auto It = std::ranges::find_if(Actors, [Actor](const std::unique_ptr<AActor> &Ptr) {
		return Ptr.get() == Actor;
	});
	if (It == Actors.end()) {
		return false;
	}

	PendingDestroyActors.push_back(Actor);
	return true;
}

void FActorScene::FlushPendingDestroyActors() {
	for (AActor *Actor : PendingDestroyActors) {
		if (Actor == nullptr) {
			continue;
		}

		auto It = std::ranges::find_if(Actors, [Actor](const std::unique_ptr<AActor> &Ptr) {
			return Ptr.get() == Actor;
		});
		if (It == Actors.end()) {
			continue;
		}

		UObjectSystem::Unregister(Actor, Actor->GetHandle());
		Actors.erase(It);
	}

	PendingDestroyActors.clear();
}

void FActorScene::Reset() {
	for (const std::unique_ptr<AActor> &Actor : Actors) {
		DestroyActor(Actor.get());
	}

	FlushPendingDestroyActors();
}

void FActorScene::Tick(float DeltaTime) {
	for (const std::unique_ptr<AActor> &Actor : Actors) {
		Actor->Tick(DeltaTime);
	}
}

const TArray<std::unique_ptr<AActor>> &FActorScene::GetActors() const noexcept {
	return Actors;
}
