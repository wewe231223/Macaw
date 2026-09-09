#pragma once

#include "Core/Base/FRenderProbe.h"
#include "Scene/Component/UCameraComponent.h"
#include "Scene/Component/UCollisionComponent.h"
#include "Scene/Component/UStaticMeshComponent.h"

// Owns the render-facing registration for one world.  Components continue to
// register through UWorld; this class deliberately has no ownership of them.
class FRenderScene {
  public:
	void Register(UStaticMeshComponent *Component);
	void Unregister(UStaticMeshComponent *Component);

	FRenderProbe &BuildProbe(const UCameraComponent *Camera, const UCollisionComponent *SelectedCollider);

  private:
	TArray<UStaticMeshComponent *> RenderableComponents;
	FRenderProbe Probe{};
};
