#pragma once

#include <optional>

#include "Core/Base/TObjectRef.h"
#include "Scene/Component/UCollisionComponent.h"
#include "FMath.h"

struct FCollisionHit {
	TObjectRef<UCollisionComponent> Component;
	float Distance = 0.0f;
};


class FCollisionScene {
  public:
	void Register(UCollisionComponent *Component);
	void Unregister(UCollisionComponent *Component);

	std::optional<FCollisionHit> Raycast(const FRay &Ray) const;

  private:
	TArray<TObjectRef<UCollisionComponent>> CollisionComponents;
};
