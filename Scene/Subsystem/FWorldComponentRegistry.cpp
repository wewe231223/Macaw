#include "PCH.h"
#include "FWorldComponentRegistry.h"

FWorldComponentRegistry::FWorldComponentRegistry(FRenderScene &InRenderScene, FCollisionScene &InCollisionScene, FEditorCameraSubsystem &InEditorCameraSubsystem)
	: RenderScene(InRenderScene),
	  CollisionScene(InCollisionScene),
	  EditorCameraSubsystem(InEditorCameraSubsystem) {
}

void FWorldComponentRegistry::RegisterRenderable(UStaticMeshComponent *Component) {
	RenderScene.Register(Component);
}

void FWorldComponentRegistry::UnregisterRenderable(UStaticMeshComponent *Component) {
	RenderScene.Unregister(Component);
}

void FWorldComponentRegistry::RegisterCollision(UCollisionComponent *Component) {
	CollisionScene.Register(Component);
}

void FWorldComponentRegistry::UnregisterCollision(UCollisionComponent *Component) {
	CollisionScene.Unregister(Component);
}

void FWorldComponentRegistry::SetMainCamera(UCameraComponent *Camera) {
	EditorCameraSubsystem.SetMainCamera(Camera);
}

void FWorldComponentRegistry::ClearMainCamera(UCameraComponent *Camera) {
	EditorCameraSubsystem.ClearMainCamera(Camera);
}
