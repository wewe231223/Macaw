#pragma once

#include "RenderCore/FSceneHandle.h"

struct FSceneUpdateBatch;

class FSceneInterface {
public:
    virtual ~FSceneInterface();

public:
    virtual FSceneHandle GetHandle() const = 0;
    virtual bool ApplyUpdates(FSceneUpdateBatch& Updates) = 0;
    virtual void Release() = 0;
};
