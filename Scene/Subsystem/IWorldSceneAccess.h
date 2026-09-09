#pragma once

#include "Scene/Subsystem/FActorScene.h"
#include "Scene/Subsystem/FWorldContext.h"

class IWorldSceneAccess {
  public:
	virtual ~IWorldSceneAccess() = default;

	virtual FActorScene &GetActorScene() noexcept = 0;
	virtual const FActorScene &GetActorScene() const noexcept = 0;
	virtual FWorldContext &GetWorldContext() noexcept = 0;
	virtual const FWorldContext &GetWorldContext() const noexcept = 0;
};
