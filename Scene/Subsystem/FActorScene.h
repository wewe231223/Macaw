#pragma once

#include "Common.h"
#include "Core/Asset/FAssetHandle.h"
#include "Core/Asset/UMesh.h"
#include "Core/Base/FGuid.h"
#include "Scene/AActor.h"
#include "Serialize/FArchive.h"

class UWorld;

class FActorScene {
  public:
	explicit FActorScene(UWorld *InWorld);
	~FActorScene();

	AActor *AddActor(std::unique_ptr<AActor> InActor);
	AActor *AddLoadedActor(std::unique_ptr<AActor> InActor, const FGuid &Guid, FArchive &Archive);
	bool SpawnActor(const FAssetHandle &MeshHandle, const FAssetHandle &PipelineHandle, const FAssetHandle &MaterialHandle, const FVector3 &Position, UMesh *Mesh);
	bool DestroyActor(AActor *Actor);
	void FlushPendingDestroyActors();
	void Reset();
	void Tick(float DeltaTime);

	const TArray<std::unique_ptr<AActor>> &GetActors() const noexcept;

  private:
	UWorld *World = nullptr;
	TArray<std::unique_ptr<AActor>> Actors;
	TArray<AActor *> PendingDestroyActors;
};
