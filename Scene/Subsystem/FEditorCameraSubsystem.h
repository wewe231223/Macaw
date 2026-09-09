#pragma once

#include <optional>

#include "Core/Channel/FStateChannel.h"
#include "FKeyboardCameraMoveRequestMessage.h"
#include "FMouseCameraRotateRequestMessage.h"
#include "Render/Panel/FEditorInfo.h"
#include "Render/RenderWindowInfo.h"
#include "Scene/Component/UCameraComponent.h"

class FEditorCameraSubsystem {
  public:
	void InitializeState(FStateChannel<FMessageEditorCameraState>::FWriter InWriter, FStateChannel<FMessageEditorCameraState>::FReader InReader);
	void SetWindowInfoReader(FStateChannel<RenderWindowInfo>::FReader InReader);

	void SetMainCamera(UCameraComponent *InCamera);
	void ClearMainCamera(UCameraComponent *InCamera);
	UCameraComponent *GetMainCamera() const noexcept;
	std::optional<FStateChannel<FMessageEditorCameraState>::FWriter> &GetLegacyWriter() noexcept;
	std::optional<FStateChannel<FMessageEditorCameraState>::FReader> &GetLegacyReader() noexcept;

	FStateChannel<RenderWindowInfo>::FReader &GetWindowInfoReader() noexcept;

	void Tick();
	void HandleMouseCameraRotateRequest(const FMouseCameraRotateRequestMessage &Message);
	void HandleKeyboardCameraMoveRequest(const FKeyboardCameraMoveRequestMessage &Message);
	void UpdateEditorCameraState();

private:
	void ApplyEditorCameraState();
	void PublishEditorCameraState();

	UCameraComponent *Camera = nullptr;
	FStateChannel<RenderWindowInfo>::FReader WindowInfoReader;

	std::optional<FStateChannel<FMessageEditorCameraState>::FWriter> EditorCameraWriter;
	std::optional<FStateChannel<FMessageEditorCameraState>::FReader> EditorCameraReader;
	std::optional<FStateChannel<FMessageEditorCameraState>::FWriter> EditorCameraStateWriter;
	std::optional<FStateChannel<FMessageEditorCameraState>::FReader> EditorCameraStateReader;
};
