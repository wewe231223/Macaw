#pragma once

#include "Core/Base/TObjectRef.h"

class USceneComponent;
struct FWorldSelectionChangedMessage;

class FEditorSelection
{
public:
	void HandleSelectionChanged(const FWorldSelectionChangedMessage& Message);

	USceneComponent* GetSelectedComponent() const;

private:
	TObjectRef<USceneComponent> SelectedComponent;
};