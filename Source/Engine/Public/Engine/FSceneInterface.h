#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Guid.h"

struct FPrimitiveSceneProxy;
struct FMatrix;
struct FBox;

class FSceneInterface {
public:
    ReflectTypes(FSceneInterface);

public:
    virtual ~FSceneInterface();

public:
    ReflectStruct(FSceneInterface);

    virtual void AddPrimitive(const FPrimitiveSceneProxy& Proxy) = 0;
    virtual void UpdatePrimitiveTransform(const FGuid& Id, const FMatrix& World, const FBox& Bounds) = 0;
    virtual void RemovePrimitive(const FGuid& Id) = 0;

    virtual std::size_t GetPrimitiveCount() const = 0;
    virtual Uint64 GetRevision() const = 0;

private:
    ReflectMembers();
};
