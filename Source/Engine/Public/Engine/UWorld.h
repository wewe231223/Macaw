#pragma once

#include "CoreUObject/UObject.h"

class ULevel;
class FSceneInterface;
class AStaticMeshActor;
struct FWorldHit;
struct FRay;
enum class EPrimitiveType : Uint8;

class UWorld final : public UObject {
public:
    ReflectTypes(UWorld, UObject);

public:
    UWorld();
    ~UWorld() override;

public:
    ReflectClass(UWorld);

    bool SetPersistentLevel(std::unique_ptr<ULevel> Level);
    ULevel* GetPersistentLevel() const;

    bool AttachScene(std::unique_ptr<FSceneInterface> Scene);
    FSceneInterface* GetScene() const;

    AStaticMeshActor* SpawnPrimitive(EPrimitiveType Type);
    bool DestroyActor(FObjectHandle Handle);

    void EnqueueRenderUpdate(FObjectHandle Handle);
    void SendAllEndOfFrameUpdates();

    bool Raycast(const FRay& Ray, FWorldHit& Hit) const;

    void Shutdown();

private:
    ReflectMembers();

private:
    std::unique_ptr<ULevel> mPersistentLevel{};
    std::unique_ptr<FSceneInterface> mScene{};

    TArray<FObjectHandle> mDirtyComponents{};
    Uint32 mSpawnSerial{};
};
