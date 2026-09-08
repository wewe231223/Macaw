#pragma once

#include "Core/Base/TypeInfo.h"

// =========================================================
// [State] 양방향 상태 데이터 (TStateChannel 용)
// =========================================================
struct FMessageEditorCameraState;
struct FMessageEditorTransformState;


// =========================================================
// [Event] 단방향 메시지 데이터 (FMessageChannel 용)
// =========================================================
struct FMessageSpawnPrimitiveMessage;
struct FMessageSaveSceneMessage;
struct FMessageLoadSceneMessage;
enum class EGizmoMode;
struct FMessageChangeGizmoModeMessage;