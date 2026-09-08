#include "PCH.h"

#include "FEditorSelection.h"
#include "FWorldSelectionChangedMessage.h"

#include "Scene/Component/USceneComponent.h"

void FEditorSelection::HandleSelectionChanged(const FWorldSelectionChangedMessage& Message)
{
	if (Message.SelectedComponentHandle.IsValid())
	{
		SelectedComponent.SetHandle(Message.SelectedComponentHandle);
	}
	else
	{
		SelectedComponent.Reset();
	}
}

USceneComponent* FEditorSelection::GetSelectedComponent() const
{
	return nullptr;
}