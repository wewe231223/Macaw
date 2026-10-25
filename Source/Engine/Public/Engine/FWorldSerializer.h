#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

class FArchive;
class FWorldContext;
class AActor;

class FWorldSerializer {
public:
    ReflectTypes(FWorldSerializer);

public:
    ReflectStruct(FWorldSerializer);
    static void Serialize(FArchive& Archive, const FWorldContext* Context);
    static void SaveActors(FArchive& Archive, const TArray<AActor*>& Actors);
    static TArray<std::unique_ptr<AActor>> LoadActors(FArchive& Archive);

private:
    ReflectMembers();
};
