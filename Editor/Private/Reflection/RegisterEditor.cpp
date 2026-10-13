#include "pch.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "Editor/Settings/FEditorSettings.h"
#include "Editor/UEditorEngine.h"
#include "Editor/UndoSystem/FUndoInfo.h"

void ReflectionModules::RegisterEditor() {
    FEditorSettings::StaticStruct();
    UEditorEngine::StaticClass();
    FObjectStateChangedMessage::StaticStruct();
}
