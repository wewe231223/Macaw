#pragma once
#include "Core/CoreMinimal.h"
#include "Core/Console/Console.h"
#include "Core/Channel/FStateChannel.h"
#include "Editor/Message/FEditorInfo.h"

void DrawConsoleContents(FConsoleOutputHandle Handle, FStateChannel<FStatDisplayFlags>::FWriter Writer);
