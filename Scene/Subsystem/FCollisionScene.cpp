#include "PCH.h"
#include "FCollisionScene.h"

#include <limits>
#include <ranges>

#include "Core/Console/Console.h"
#include "Scene/Component/UCollisionComponent.h"

void FCollisionScene::Register(UCollisionComponent *Component) {
	if (Component == nullptr) {
		return;
	}

	const auto FoundComponent = std::ranges::find_if(
		CollisionComponents,
		[Component](const TObjectRef<UCollisionComponent> &ComponentRef) {
			return ComponentRef.Get() == Component;
		});

	if (FoundComponent != CollisionComponents.end()) {
		return;
	}

	CollisionComponents.emplace_back(Component);
}

void FCollisionScene::Unregister(UCollisionComponent *Component) {
	std::erase_if( CollisionComponents, [Component](const TObjectRef<UCollisionComponent> &ComponentRef) {
			UCollisionComponent *RegisteredComponent = ComponentRef.Get();
			return RegisteredComponent == nullptr || RegisteredComponent == Component;
		});
}

std::optional<FCollisionHit> FCollisionScene::Raycast(const FRay &Ray) const {
	float NearestDistance = std::numeric_limits<float>::max();
	UCollisionComponent *NearestCollision = nullptr;

	for (const TObjectRef<UCollisionComponent> &CollisionRef : CollisionComponents) {
		UCollisionComponent *CollisionComponent = CollisionRef.Get();
		if (CollisionComponent == nullptr) {
			continue;
		}

		float Distance = 0.0f;
		if (CollisionComponent->Raycast(Ray, Distance) && Distance < NearestDistance) {
			NearestDistance = Distance;
			NearestCollision = CollisionComponent;
			Console::AddLog(Console::STDOutHandle, ELogLevel::Log, ELogCategory::Etc, "Raycast hit bounds of collision component %f", Distance);
		}
	}

	if (NearestCollision == nullptr) {
		return std::nullopt;
	}

	return FCollisionHit{TObjectRef<UCollisionComponent>{NearestCollision}, NearestDistance};
}
