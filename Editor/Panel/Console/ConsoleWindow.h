#pragma once

#include "pch.h"
#include "Core/Console/Console.h"
#include "../../../Core/Channel/FStateChannel.h"
#include "Core/Channel/FEditorInfo.h"

void DrawConsoleContents(FConsoleOutputHandle Handle, FStateChannel<FStatDisplayFlags>::FWriter Writer);
