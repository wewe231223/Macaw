#pragma once

#include <filesystem>

#include "Common.h"
#include "Scene/Subsystem/IWorldSceneAccess.h"

class FSceneSerializer {
  public:
	bool Save(const IWorldSceneAccess &World, const FString &SceneName) const;
	bool Load(IWorldSceneAccess &World, const std::filesystem::path &ScenePath) const;
	void Reset(IWorldSceneAccess &World) const;
};
