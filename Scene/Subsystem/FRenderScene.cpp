#include "PCH.h"
#include "FRenderScene.h"

#include <ranges>

#include "Scene/AActor.h"
#include "Scene/Component/UCameraComponent.h"
#include "Scene/Component/UCollisionComponent.h"
#include "Scene/Component/UStaticMeshComponent.h"

void FRenderScene::Register(UStaticMeshComponent *Component) {
	if (Component == nullptr || std::ranges::find(RenderableComponents, Component) != RenderableComponents.end()) {
		return;
	}

	RenderableComponents.push_back(Component);
}

void FRenderScene::Unregister(UStaticMeshComponent *Component) {
	std::erase(RenderableComponents, Component);
}

FRenderProbe &FRenderScene::BuildProbe(const UCameraComponent *Camera, const UCollisionComponent *SelectedCollider) {
	Probe.ActorProbes.clear();

	for (const UStaticMeshComponent *Component : RenderableComponents) {
		FActorProbe ActorProbe{};
		Component->MakeRender(ActorProbe);

		if (SelectedCollider != nullptr &&
			Component->GetOwner() == SelectedCollider->GetOwner()) {
			ActorProbe.Flags |= 0x0000'0001;
		}

		Probe.ActorProbes.push_back(ActorProbe);
	}

	if (Camera != nullptr) {
		Probe.MainCameraProbe.View = Camera->GetViewMatrix();
		Probe.MainCameraProbe.Projection = Camera->GetProjectionMatrix();
		Probe.MainCameraProbe.ViewProjection = Camera->GetViewProjectionMatrix();
	}

	return Probe;
}
