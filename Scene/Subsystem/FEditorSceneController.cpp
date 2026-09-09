#include "PCH.h"
#include "FEditorSceneController.h"

#include <random>

#include "Core/Asset/FAssetRegistry.h"
#include "Core/Asset/UMesh.h"
#include "FCollisionScene.h"
#include "FMousePickRequestMessage.h"
#include "FTransformEditRequestMessage.h"
#include "FWorldSelectionChangedMessage.h"
#include "Render/Panel/FEditorInfo.h"
#include "Render/RenderWindowInfo.h"
#include "Scene/AActor.h"
#include "Scene/Component/UCameraComponent.h"
#include "Scene/Component/UCollisionComponent.h"
#include "Scene/Component/USceneComponent.h"
#include "Scene/Subsystem/FActorScene.h"

FStateChannel<FEditorSelectionState>::FReader FEditorSceneController::GetSelectionStateReader() const noexcept {
	return EditorSelectionState.GetReader();
}

UCollisionComponent *FEditorSceneController::GetSelectedCollider() const noexcept {
	return SelectedCollider.Get();
}

void FEditorSceneController::InitializeEventSender(FMessageChannel::FSender &&InSender) {
	EditorEventSender.emplace(std::move(InSender));
}

void FEditorSceneController::Tick() {
	PublishSelectionState();
}

void FEditorSceneController::HandleMousePickRequest(const FMousePickRequestMessage &Message, UCameraComponent *Camera, FStateChannel<RenderWindowInfo>::FReader &WindowInfoReader, const FCollisionScene &CollisionScene) {
	FObjectHandle SelectedComponentHandle{};

	if (Camera != nullptr && WindowInfoReader.Read().Viewport.Width != 0 && WindowInfoReader.Read().Viewport.Height != 0) {
		const float NdcX = (2.0f * static_cast<float>(Message.ScreenX) / static_cast<float>(WindowInfoReader.Read().Viewport.Width)) - 1.0f;
		const float NdcY = 1.0f - (2.0f * static_cast<float>(Message.ScreenY) / static_cast<float>(WindowInfoReader.Read().Viewport.Height));

		FMatrix InverseViewProjection;
		if (!Camera->GetViewProjectionMatrix().TryInverse(InverseViewProjection)) {
			return;
		}

		FVector3 RayOrigin;
		FVector3 RayEnd;
		if (!InverseViewProjection.TransformCoord({NdcX, NdcY, 0.0f}, RayOrigin) or !InverseViewProjection.TransformCoord({NdcX, NdcY, 1.0f}, RayEnd)) {
			return;
		}

		FVector3 RayDirection = RayEnd - RayOrigin;
		if (RayDirection.LengthSquared() > 0.0f) {
			RayDirection.Normalize();
			const std::optional<FCollisionHit> Hit = CollisionScene.Raycast(
				FRay{RayOrigin.ToSimpleMath(), RayDirection.ToSimpleMath()});

			if (Hit.has_value()) {
				SelectedCollider = Hit->Component;

				if (AActor *Owner = Hit->Component->GetOwner()) {
					if (USceneComponent *RootComponent = Owner->GetRootComponent()) {
						SelectedComponentHandle = RootComponent->GetHandle();
					}
				}
			} else {
				SelectedCollider.Reset();
				EditorSelectionState.GetWriter().Clear();
			}
		}
	}

	if (EditorEventSender.has_value()) {
		EditorEventSender->TryEmplace<FWorldSelectionChangedMessage>(SelectedComponentHandle);
	}
}

void FEditorSceneController::HandleMousePickReleaseRequest(const FMousePickReleaseRequestMessage & /*Message*/) {
}

void FEditorSceneController::HandleTransformEditRequest(const FTransformEditRequestMessage &Message) {
	TObjectRef<USceneComponent> TargetRef{Message.TargetHandle};
	USceneComponent *Target = TargetRef.Get();
	if (Target == nullptr) {
		ActiveTransformEdit.reset();
		return;
	}

	switch (Message.Phase) {
	case ETransformEditPhase::Begin:
		if (Message.ExpectedTransformRevision != TransformRevision ||
			ActiveTransformEdit.has_value()) {
			return;
		}

		ActiveTransformEdit = FActiveTransformEdit{
			.SessionId = Message.SessionId,
			.TargetHandle = Message.TargetHandle,
			.OriginalWorld = Target->GetWorldMatrix()};
		break;

	case ETransformEditPhase::Update:
		if (!ActiveTransformEdit.has_value() ||
			ActiveTransformEdit->SessionId != Message.SessionId ||
			ActiveTransformEdit->TargetHandle != Message.TargetHandle) {
			return;
		}

		if (ApplyWorldMatrix(*Target, Message.DesiredWorld)) {
			++TransformRevision;
		}
		break;

	case ETransformEditPhase::Commit:
		if (ActiveTransformEdit.has_value() &&
			ActiveTransformEdit->SessionId == Message.SessionId &&
			ActiveTransformEdit->TargetHandle == Message.TargetHandle) {
			ActiveTransformEdit.reset();
		}
		break;

	case ETransformEditPhase::Cancel:
		if (ActiveTransformEdit.has_value() &&
			ActiveTransformEdit->SessionId == Message.SessionId &&
			ActiveTransformEdit->TargetHandle == Message.TargetHandle) {
			if (ApplyWorldMatrix(*Target, ActiveTransformEdit->OriginalWorld)) {
				++TransformRevision;
			}
			ActiveTransformEdit.reset();
		}
		break;
	}
}

void FEditorSceneController::HandleSpawnPrimitive(FActorScene &ActorScene, const FMessageSpawnPrimitive &Message, FAssetRegistry &AssetRegistry) {
	static std::mt19937 RandomEngine{std::random_device{}()};
	const FAssetHandle MeshHandle = AssetRegistry.GetAsset(Message.PrimitiveType);
	const FAssetHandle PipelineHandle = AssetRegistry.GetAsset("BasePipeline");
	const FAssetHandle Materials[] = {
		AssetRegistry.GetAsset("GreyMaterial"), AssetRegistry.GetAsset("RedMaterial"),
		AssetRegistry.GetAsset("GreenMaterial"), AssetRegistry.GetAsset("BlueMaterial"),
		AssetRegistry.GetAsset("YellowMaterial"), AssetRegistry.GetAsset("AmberMaterial"),
		AssetRegistry.GetAsset("BrownMaterial"), AssetRegistry.GetAsset("CyanMaterial"),
		AssetRegistry.GetAsset("LimeMaterial"), AssetRegistry.GetAsset("MagentaMaterial"),
		AssetRegistry.GetAsset("NavyMaterial"), AssetRegistry.GetAsset("OrangeMaterial"),
		AssetRegistry.GetAsset("PinkMaterial"), AssetRegistry.GetAsset("PurpleMaterial"),
		AssetRegistry.GetAsset("TealMaterial"), AssetRegistry.GetAsset("WhiteMaterial")};

	int Count = _countof(Materials);
	std::uniform_int_distribution<decltype(Count)> Distribution(0, Count - 1);
	const FAssetHandle MaterialHandle = Materials[Distribution(RandomEngine)];
	UMesh *Mesh = AssetRegistry.ResolveAsset<UMesh>(MeshHandle);
	if (Mesh == nullptr) {
		return;
	}

	std::uniform_real_distribution<float> RandomX(-5.0f, 5.0f);
	std::uniform_real_distribution<float> RandomY(-5.0f, 5.0f);
	std::uniform_real_distribution<float> RandomZ(-3.0f, 3.0f);
	constexpr FVector3 SpawnCenter{0.0f, 0.0f, 5.0f};

	for (uint32 Index = 0; Index < Message.SpawnCount; ++Index) {
		ActorScene.SpawnActor( MeshHandle, PipelineHandle, MaterialHandle, FVector3{ SpawnCenter.x + RandomX(RandomEngine), SpawnCenter.y + RandomY(RandomEngine), SpawnCenter.z + RandomZ(RandomEngine)}, Mesh);
	}
}

void FEditorSceneController::HandleNewScene(const FMessageNewScene & /*Message*/) {
}

void FEditorSceneController::HandleChangeGizmoMode(const FMessageChangeGizmoMode & /*Message*/) {
}

bool FEditorSceneController::ApplyWorldMatrix(USceneComponent &Component, const FMatrix &DesiredWorld) {
	FMatrix LocalMatrix = DesiredWorld;
	if (USceneComponent *Parent = Component.GetParent()) {
		FMatrix ParentInverse;
		if (!Parent->GetWorldMatrix().TryInverse(ParentInverse)) {
			return false;
		}
		LocalMatrix = DesiredWorld * ParentInverse;
	}

	FVector3 Scale{};
	FQuat Rotation{};
	FVector3 Translation{};
	if (!LocalMatrix.Decompose(Scale, Rotation, Translation)) {
		return false;
	}

	FTransform &Transform = Component.GetTransform();
	Transform.SetPosition(Translation);
	Transform.SetRotation(FVector3(Rotation.ToEuler()));
	Transform.SetScale(Scale);
	return true;
}

void FEditorSceneController::PublishSelectionState() {
	UCollisionComponent *Collision = SelectedCollider.Get();
	if (Collision == nullptr) {
		SelectedCollider.Reset();
		EditorSelectionState.GetWriter().Clear();
		return;
	}

	AActor *Owner = Collision->GetOwner();
	USceneComponent *Target = Owner != nullptr ? Owner->GetRootComponent() : nullptr;
	if (Target == nullptr) {
		SelectedCollider.Reset();
		EditorSelectionState.GetWriter().Clear();
		return;
	}

	EditorSelectionState.GetWriter().Emplace(FEditorSelectionState{
		.TransformTargetHandle = Target->GetHandle(),
		.PickedColliderHandle = Collision->GetHandle(),
		.TargetWorld = Target->GetWorldMatrix(),
		.ColliderWorld = Collision->GetWorldMatrix(),
		.BoundsCenter = Collision->GetBoundsCenter(),
		.BoundsExtent = Collision->GetExtent(),
		.BoundsOrientation = Collision->GetBoundsOrientation(),
		.TransformRevision = TransformRevision});
}
