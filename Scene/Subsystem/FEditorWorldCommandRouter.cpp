#include "PCH.h"
#include "FEditorWorldCommandRouter.h"

FEditorWorldCommandRouter::FEditorWorldCommandRouter(FActorScene &InActorScene, FCollisionScene &InCollisionScene, FEditorCameraSubsystem &InEditorCameraSubsystem, FEditorSceneController &InEditorSceneController)
	: ActorScene(InActorScene),
	  CollisionScene(InCollisionScene),
	  EditorCameraSubsystem(InEditorCameraSubsystem),
	  EditorSceneController(InEditorSceneController) {
}

void FEditorWorldCommandRouter::HandleMousePickRequest(const FMousePickRequestMessage &Message) {
	EditorSceneController.HandleMousePickRequest(
		Message,
		EditorCameraSubsystem.GetMainCamera(),
		EditorCameraSubsystem.GetWindowInfoReader(),
		CollisionScene);
}

void FEditorWorldCommandRouter::HandleMousePickReleaseRequest(const FMousePickReleaseRequestMessage &Message) {
	EditorSceneController.HandleMousePickReleaseRequest(Message);
}

void FEditorWorldCommandRouter::HandleTransformEditRequest(const FTransformEditRequestMessage &Message) {
	EditorSceneController.HandleTransformEditRequest(Message);
}

void FEditorWorldCommandRouter::HandleMouseCameraRotateRequest(const FMouseCameraRotateRequestMessage &Message) {
	EditorCameraSubsystem.HandleMouseCameraRotateRequest(Message);
}

void FEditorWorldCommandRouter::HandleKeyboardCameraMoveRequest(const FKeyboardCameraMoveRequestMessage &Message) {
	EditorCameraSubsystem.HandleKeyboardCameraMoveRequest(Message);
}

void FEditorWorldCommandRouter::HandleSpawnPrimitive(const FMessageSpawnPrimitive &Message, FAssetRegistry &AssetRegistry) {
	EditorSceneController.HandleSpawnPrimitive(ActorScene, Message, AssetRegistry);
}

void FEditorWorldCommandRouter::HandleNewScene(const FMessageNewScene &Message) {
	EditorSceneController.HandleNewScene(Message);
}

void FEditorWorldCommandRouter::HandleLoadScene(const FMessageLoadScene & /*Message*/) {
}

void FEditorWorldCommandRouter::HandleChangeGizmoMode(const FMessageChangeGizmoMode &Message) {
	EditorSceneController.HandleChangeGizmoMode(Message);
}
