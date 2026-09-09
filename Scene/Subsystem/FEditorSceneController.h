#pragma once

#include <cstdint>
#include <optional>

#include "Core/Base/TObjectRef.h"
#include "Core/Asset/FAssetRegistry.h"
#include "Core/Channel/FMessageChannel.h"
#include "Core/Channel/FStateChannel.h"
#include "FMousePickRequestMessage.h"
#include "FTransformEditRequestMessage.h"
#include "FEditorSelectionState.h"
#include "Render/Panel/FEditorInfo.h"
#include "Render/RenderWindowInfo.h"
#include "Scene/Component/UCameraComponent.h"
#include "Scene/Component/UCollisionComponent.h"
#include "Scene/Component/USceneComponent.h"
#include "Scene/Subsystem/FActorScene.h"
#include "Scene/Subsystem/FCollisionScene.h"

class FEditorSceneController {
  public:
	FStateChannel<FEditorSelectionState>::FReader GetSelectionStateReader() const noexcept;
	UCollisionComponent *GetSelectedCollider() const noexcept;

	void InitializeEventSender(FMessageChannel::FSender &&InSender);
	void Tick();

	void HandleMousePickRequest(const FMousePickRequestMessage &Message, UCameraComponent *Camera, FStateChannel<RenderWindowInfo>::FReader &WindowInfoReader, const FCollisionScene &CollisionScene);
	void HandleMousePickReleaseRequest(const FMousePickReleaseRequestMessage &Message);
	void HandleTransformEditRequest(const FTransformEditRequestMessage &Message);
	void HandleSpawnPrimitive(FActorScene &ActorScene, const FMessageSpawnPrimitive &Message, FAssetRegistry &AssetRegistry);
	void HandleNewScene(const FMessageNewScene &Message);
	void HandleChangeGizmoMode(const FMessageChangeGizmoMode &Message);

  private:
	struct FActiveTransformEdit {
		std::uint64_t SessionId = 0;
		FObjectHandle TargetHandle{};
		FMatrix OriginalWorld{FMatrix::Identity};
	};

	static bool ApplyWorldMatrix(USceneComponent &Component, const FMatrix &DesiredWorld);
	void PublishSelectionState();

	TObjectRef<UCollisionComponent> SelectedCollider;
	FStateChannel<FEditorSelectionState> EditorSelectionState;
	std::optional<FActiveTransformEdit> ActiveTransformEdit;
	std::uint64_t TransformRevision = 1;
	std::optional<FMessageChannel::FSender> EditorEventSender;
};
