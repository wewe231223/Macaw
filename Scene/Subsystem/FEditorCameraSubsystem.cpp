#include "PCH.h"
#include "FEditorCameraSubsystem.h"

#include <algorithm>

#include "FKeyboardCameraMoveRequestMessage.h"
#include "FMouseCameraRotateRequestMessage.h"
#include "Scene/Component/UCameraComponent.h"

void FEditorCameraSubsystem::InitializeState(FStateChannel<FMessageEditorCameraState>::FWriter InWriter, FStateChannel<FMessageEditorCameraState>::FReader InReader) {
	EditorCameraStateWriter.emplace(std::move(InWriter));
	EditorCameraStateReader.emplace(std::move(InReader));

	if (Camera != nullptr) {
		PublishEditorCameraState();
	}
}

void FEditorCameraSubsystem::SetWindowInfoReader(FStateChannel<RenderWindowInfo>::FReader InReader) {
	WindowInfoReader = std::move(InReader);
}

void FEditorCameraSubsystem::SetMainCamera(UCameraComponent *InCamera) {
	Camera = InCamera;
	PublishEditorCameraState();
}

void FEditorCameraSubsystem::ClearMainCamera(UCameraComponent *InCamera) {
	if (Camera == InCamera) {
		Camera = nullptr;
	}
}

UCameraComponent *FEditorCameraSubsystem::GetMainCamera() const noexcept {
	return Camera;
}

std::optional<FStateChannel<FMessageEditorCameraState>::FWriter> &FEditorCameraSubsystem::GetLegacyWriter() noexcept {
	return EditorCameraWriter;
}

std::optional<FStateChannel<FMessageEditorCameraState>::FReader> &FEditorCameraSubsystem::GetLegacyReader() noexcept {
	return EditorCameraReader;
}

FStateChannel<RenderWindowInfo>::FReader &FEditorCameraSubsystem::GetWindowInfoReader() noexcept {
	return WindowInfoReader;
}

void FEditorCameraSubsystem::Tick() {
	if (WindowInfoReader.HasChanged()) {
		Camera->SetAspectRatio( static_cast<float>(WindowInfoReader.Read().ScreenWidth) / static_cast<float>(WindowInfoReader.Read().ScreenHeight));
	}

	ApplyEditorCameraState();
}

void FEditorCameraSubsystem::HandleMouseCameraRotateRequest(const FMouseCameraRotateRequestMessage &Message) {
	if (Camera == nullptr) {
		return;
	}

	constexpr float RotationSensitivity = 0.003f;
	constexpr float MaximumPitch = 1.5f;

	FTransform &CameraTransform = Camera->GetTransform();
	FRotator Rotation = CameraTransform.GetRotation();

	Rotation.y += Message.DeltaX * RotationSensitivity;
	Rotation.x = std::clamp(Rotation.x - Message.DeltaY * RotationSensitivity, -MaximumPitch, MaximumPitch);

	CameraTransform.SetRotation(Rotation);
	PublishEditorCameraState();

	if (EditorCameraWriter.has_value()) {
		EditorCameraWriter->Write(FMessageEditorCameraState{ CameraTransform.GetPosition(), CameraTransform.GetRotation(), Camera->GetFOV()});
	}
}

void FEditorCameraSubsystem::HandleKeyboardCameraMoveRequest(const FKeyboardCameraMoveRequestMessage &Message) {
	if (Camera == nullptr || Message.DeltaTime <= 0.0f) {
		return;
	}

	const FMatrix CameraWorldMatrix = Camera->GetWorldMatrix();
	const FVector3 ForwardDirection = CameraWorldMatrix.Forward();
	const FVector3 RightDirection = CameraWorldMatrix.Right();
	const FVector3 Forward = ForwardDirection * Message.ForwardAxis;

	FVector3 MoveDirection = ForwardDirection * Message.ForwardAxis + RightDirection * Message.RightAxis;

	if (MoveDirection.LengthSquared() <= 0.0f) {
		return;
	}

	MoveDirection.Normalize();
	constexpr float CameraMoveSpeed = 5.0f;

	FTransform &CameraTransform = Camera->GetTransform();
	CameraTransform.SetPosition(CameraTransform.GetPosition() + MoveDirection * CameraMoveSpeed * Message.DeltaTime);

	PublishEditorCameraState();
}

void FEditorCameraSubsystem::UpdateEditorCameraState() {
	if (!EditorCameraReader.has_value() || Camera == nullptr) {
		return;
	}

	const auto Result = EditorCameraReader->ReadIfChanged();
	if (!Result.Changed || Result.Value == nullptr) {
		return;
	}

	FTransform &Transform = Camera->GetTransform();
	Transform.SetPosition(Result.Value->Position);
	Transform.SetRotation(Result.Value->Rotation);
	Camera->SetFOV(Result.Value->FOV);
}

void FEditorCameraSubsystem::ApplyEditorCameraState() {
	if (!EditorCameraStateReader.has_value() || Camera == nullptr) {
		return;
	}

	const auto Result = EditorCameraStateReader->ReadIfChanged();
	if (!Result.Changed || Result.Value == nullptr) {
		return;
	}

	FTransform &CameraTransform = Camera->GetTransform();
	CameraTransform.SetPosition(Result.Value->Position);
	CameraTransform.SetRotation(Result.Value->Rotation);
	Camera->SetFOV(Result.Value->FOV);
}

void FEditorCameraSubsystem::PublishEditorCameraState() {
	if (!EditorCameraStateWriter.has_value() || Camera == nullptr) {
		return;
	}

	const FTransform &CameraTransform = Camera->GetTransform();
	EditorCameraStateWriter->Write(FMessageEditorCameraState{ CameraTransform.GetPosition(), CameraTransform.GetRotation(), Camera->GetFOV()});
}
