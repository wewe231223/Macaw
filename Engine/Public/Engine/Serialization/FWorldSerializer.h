#pragma once

#include "Core/Common.h"
#include "Core/Base/FGuid.h"
#include "Core/Base/FName.h"
#include <memory>

class AActor;
class FArchive;
class UWorld;

class FWorldSerializer final {
private:
    struct FObjectRecord {
        FGuid mGuid{};
        FName mTypeName{};
    };

    struct FActorRecord {
        FObjectRecord mObject{};
        TArray<FObjectRecord> mComponents{};
    };

    class FLoadScope final {
    public:
        explicit FLoadScope(UWorld& World);
        ~FLoadScope();
        FLoadScope(const FLoadScope&) = delete;
        FLoadScope& operator=(const FLoadScope&) = delete;

    private:
        UWorld& mWorld;
        bool mWasPlaying{};
    };

public:
    static bool CanSerialize(const UWorld& World);
    static bool Save(UWorld& World, FArchive& Archive);
    static bool Load(UWorld& World, FArchive& Archive);

private:
    friend class FSceneSerializer;

    static bool LoadInternal(UWorld& World, FArchive& Archive);
    static bool SerializeObjectRecord(FArchive& Archive, FObjectRecord& Record);
    static bool SerializeObjectTable(FArchive& Archive, TArray<FActorRecord>& Records);
    static bool ValidateObjectTable(const UWorld& World, const TArray<FActorRecord>& Records);
    static std::unique_ptr<AActor> CreateActor(const FActorRecord& Record);
    static bool ValidateLoadedActors(const TArray<std::unique_ptr<AActor>>& Actors, const TArray<FActorRecord>& Records);
    static bool InstallActors(UWorld& World, TArray<std::unique_ptr<AActor>> Actors);
};
