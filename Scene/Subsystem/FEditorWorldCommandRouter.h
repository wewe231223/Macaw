#pragma once

#include "Core/Asset/FAssetRegistry.h"
#include "FKeyboardCameraMoveRequestMessage.h"
#include "FMouseCameraRotateRequestMessage.h"
#include "FMousePickRequestMessage.h"
#include "FTransformEditRequestMessage.h"
#include "Render/Panel/FEditorInfo.h"
#include "Scene/Subsystem/FActorScene.h"
#include "Scene/Subsystem/FCollisionScene.h"
#include "Scene/Subsystem/FEditorCameraSubsystem.h"
#include "Scene/Subsystem/FEditorSceneController.h"

class FEditorWorldCommandRouter {
  public:
	FEditorWorldCommandRouter(FActorScene &InActorScene, FCollisionScene &InCollisionScene, FEditorCameraSubsystem &InEditorCameraSubsystem, FEditorSceneController &InEditorSceneController);

	void HandleMousePickRequest(const FMousePickRequestMessage &Message);
	void HandleMousePickReleaseRequest(const FMousePickReleaseRequestMessage &Message);
	void HandleTransformEditRequest(const FTransformEditRequestMessage &Message);
	void HandleMouseCameraRotateRequest(const FMouseCameraRotateRequestMessage &Message);
	void HandleKeyboardCameraMoveRequest(const FKeyboardCameraMoveRequestMessage &Message);
	void HandleSpawnPrimitive(const FMessageSpawnPrimitive &Message, FAssetRegistry &AssetRegistry);
	void HandleNewScene(const FMessageNewScene &Message);
	void HandleLoadScene(const FMessageLoadScene &Message);
	void HandleChangeGizmoMode(const FMessageChangeGizmoMode &Message);

  private:
	FActorScene &ActorScene;
	FCollisionScene &CollisionScene;
	FEditorCameraSubsystem &EditorCameraSubsystem;
	FEditorSceneController &EditorSceneController;
};
