#include "pch.h"
#include "Editor/Settings/FEditorSettings.h"
#include "Core/Archive/FArchive.h"
#include "CoreUObject/Reflection/UStruct.h"

void FEditorSettings::Serialize(FArchive& Ar) {
    StaticStruct()->SerializeProperties(this, Ar);
}
