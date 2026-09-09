#pragma once

#include <filesystem>
#include <optional>
#include <d3d11.h>

#include "Common.h"
#include "Core/Asset/FAssetHandle.h"
#include "Core/Asset/FAssetRegistry.h"
#include "Core/Asset/UMesh.h"
#include "Core/Base/FRenderProbe.h"
#include "Core/Base/UObject.h"
#include "FKeyboardCameraMoveRequestMessage.h"
#include "FMouseCameraRotateRequestMessage.h"
#include "FMousePickRequestMessage.h"
#include "FTransformEditRequestMessage.h"
#include "Render/Panel/FEditorInfo.h"
#include "Scene/Component/UCameraComponent.h"
#include "Scene/Component/UCollisionComponent.h"
#include "Scene/Component/UStaticMeshComponent.h"
#include "Scene/Subsystem/FCollisionScene.h"
#include "Scene/Subsystem/FEditorCameraSubsystem.h"
#include "Scene/Subsystem/FEditorSceneController.h"
#include "Scene/Subsystem/FEditorWorldCommandRouter.h"
#include "Scene/Subsystem/FRenderScene.h"
#include "Scene/Subsystem/FSceneSerializer.h"
#include "Scene/Subsystem/FActorScene.h"
#include "Scene/Subsystem/FWorldComponentRegistry.h"
#include "Scene/Subsystem/FWorldContext.h"
#include "Scene/Subsystem/IWorldSceneAccess.h"

class UWorld : public UObject, public IWorldSceneAccess {
  public:
	UWorld();
	~UWorld() override;

	UWorld(const UWorld&) = delete;
	UWorld& operator=(const UWorld&) = delete;

	UWorld(UWorld&&) = delete;
	UWorld& operator=(UWorld&&) = delete;

public:
	AActor *AddActor(std::unique_ptr<AActor> InActor);

	template <typename T>
		requires std::is_base_of_v<AActor, T>
	T *AdoptActor() {
		std::unique_ptr<T> NewActor = std::make_unique<T>();
		T *ActorPtr = NewActor.get();

		if (AddActor(std::move(NewActor)) == nullptr) {
			return nullptr;
		}

		return ActorPtr;
	}

	bool SpawnActor(const FAssetHandle &MeshHandle, const FAssetHandle &PipelineHandle, const FAssetHandle &MaterialHandle, const FVector3 &Position, UMesh *Mesh, FAssetRegistry *AssetRegistry);
	bool DestroyActor(AActor *Actor);
	void FlushPendingDestroyActors();

	const TArray<std::unique_ptr<AActor>> &GetActors() const;
	FRenderProbe &BuildRenderProbe();
	FStateChannel<FEditorSelectionState>::FReader GetEditorSelectionStateReader() const noexcept;

	void Tick(float DeltaTime);

	void RegisterRenderable(UStaticMeshComponent *Component);
	void UnregisterRenderable(UStaticMeshComponent *Component);
	void SetMainCamera(UCameraComponent *InCamera);
	void ClearMainCamera(UCameraComponent *InCamera);

	bool SaveScene(const FString &SceneName, FAssetRegistry *AssetRegistry);
	bool LoadScene(const std::filesystem::path &ScenePath, ID3D11Device *Device, FAssetRegistry *AssetRegistry);

	JG_DECLARE_DERIVED_TYPEINFO(UWorld, UObject);

	void InitializeEditorEventSender(FMessageChannel::FSender &&InSender);
	void InitializeEditorCameraState(FStateChannel<FMessageEditorCameraState>::FWriter InWriter, FStateChannel<FMessageEditorCameraState>::FReader InReader);

	void HandleMousePickRequest(const FMousePickRequestMessage &Message);
	void HandleMousePickReleaseRequest(const FMousePickReleaseRequestMessage &Message);
	void HandleTransformEditRequest(const FTransformEditRequestMessage &Message);
	void HandleMouseCameraRotateRequest(const FMouseCameraRotateRequestMessage &Message);
	void HandleKeyboardCameraMoveRequest(const FKeyboardCameraMoveRequestMessage &Message);

	void RegisterCollision(UCollisionComponent *Component);
	void UnregisterCollision(UCollisionComponent *Component);

	void HandleSpawnPrimitive(const FMessageSpawnPrimitive &Message, FAssetRegistry &AssetRegistry);
	void HandleNewScene(const FMessageNewScene &Message);
	void HandleLoadScene(const FMessageLoadScene &Message);
	void HandleChangeGizmoMode(const FMessageChangeGizmoMode &Message);

	std::optional<FStateChannel<FMessageEditorCameraState>::FWriter> &EditorCameraWriter;
	std::optional<FStateChannel<FMessageEditorCameraState>::FReader> &EditorCameraReader;

	void UpdateEditorCameraState();
	void SetAssetRegistry(FAssetRegistry *InAssetRegistry);
	void SetWindowInfoReader(FStateChannel<RenderWindowInfo>::FReader InReader);
	FAssetRegistry *GetAssetRegistry() const;
	void ResetWorld(FAssetRegistry *AssetRegistry, ID3D11Device *Device);

	FActorScene &GetActorScene() noexcept override;
	const FActorScene &GetActorScene() const noexcept override;
	FWorldContext &GetWorldContext() noexcept override;
	const FWorldContext &GetWorldContext() const noexcept override;

private:
	FRenderScene RenderScene;
	FCollisionScene CollisionScene;
	FEditorCameraSubsystem EditorCameraSubsystem;
	FEditorSceneController EditorSceneController;
	FWorldComponentRegistry ComponentRegistry;
	FActorScene ActorScene;
	FWorldContext WorldContext;
	FEditorWorldCommandRouter EditorWorldCommandRouter;
	FSceneSerializer SceneSerializer;
};
