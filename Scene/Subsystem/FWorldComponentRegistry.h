#pragma once

#include "Scene/Component/UCameraComponent.h"
#include "Scene/Component/UCollisionComponent.h"
#include "Scene/Component/UStaticMeshComponent.h"
#include "Scene/Subsystem/FCollisionScene.h"
#include "Scene/Subsystem/FEditorCameraSubsystem.h"
#include "Scene/Subsystem/FRenderScene.h"


class FWorldComponentRegistry {
  public:
	FWorldComponentRegistry(FRenderScene &InRenderScene, FCollisionScene &InCollisionScene, FEditorCameraSubsystem &InEditorCameraSubsystem);

	void RegisterRenderable(UStaticMeshComponent *Component);
	void UnregisterRenderable(UStaticMeshComponent *Component);
	void RegisterCollision(UCollisionComponent *Component);
	void UnregisterCollision(UCollisionComponent *Component);
	void SetMainCamera(UCameraComponent *Camera);
	void ClearMainCamera(UCameraComponent *Camera);

  private:
	FRenderScene &RenderScene;
	FCollisionScene &CollisionScene;
	FEditorCameraSubsystem &EditorCameraSubsystem;
};
