#pragma once
#include "Core/Base/FName.h"
#include "Core/Common.h"
#include "Core/Channel/FMessageTypeInfo.h"

// =========================================================
// [Event] 단방향 메시지 데이터 (FMessageChannel 용)
// =========================================================
struct FMessageSpawnComponent {
public:
    FMessageSpawnComponent(FName InputComponentType, FString InputMeshType, Uint32 InputCount) noexcept;
    DECLARE_CHANNEL_MESSAGE(FMessageSpawnComponent);

public:
    FName mComponentType{};
    FString mMeshType{};
    Uint32 mSpawnCount{};
};

struct FMessageSaveScene {
public:
    FMessageSaveScene(FString InputSceneName) noexcept;
    DECLARE_CHANNEL_MESSAGE(FMessageSaveScene);

public:
    FString mSceneName{};
};

struct FMessageLoadScene {
public:
    FMessageLoadScene(FString InputFilePath) noexcept;
    DECLARE_CHANNEL_MESSAGE(FMessageLoadScene);

public:
    FString mFilePath{};
};

struct FMessageImportMesh {
public:
    FMessageImportMesh(FString InputAssetName, FString InputFilePath, FString InputMetaPath) noexcept;
    DECLARE_CHANNEL_MESSAGE(FMessageImportMesh);

public:
    FString mAssetName{};
    FString mFilePath{};
    FString mMetaPath{};
};

enum class EGizmoMode : Uint8 {
    Translate,
    Rotate,
    Scale
};

enum class EGizmoCoordinateSpace : Uint8 {
    World,
    Local
};

struct FStatDisplayFlags {
    bool mBShowFps{false};
    bool mBShowMemory{false};
    bool mBObjectSystem{false};
    bool mBShowPicking{false};
    bool mBShowRender{false};
    bool mBShowLOD{false};
};
