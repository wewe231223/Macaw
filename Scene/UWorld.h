#pragma once

#include "Common.h"
#include "Core/Base/UObject.h"
#include "Core/Base/UObjectSystem.h"
#include "Core/Base/FRenderProbe.h"
#include "URenderableObject.h"
#include "UCamera.h"

class UWorld : public UObject
{
public:
    UWorld() = default;
    ~UWorld() override = default;

    template<typename T>
    T* SpawnObject()
    {
        std::unique_ptr<T> NewObject = std::make_unique<T>();

        T* ObjectPtr = NewObject.get();

        UObjectSystem::Register(ObjectPtr);

        Objects.push_back(std::move(NewObject));

        if constexpr (std::is_base_of_v<URenderableObject, T>)
        {
            RenderableObjects.push_back(ObjectPtr);
        }

        return ObjectPtr;
    }

    const std::vector<std::unique_ptr<UObject>>& GetObjects() const;
    FRenderProbe BuildRenderProbe() const;

private:
    std::vector<std::unique_ptr<UObject>> Objects;
    std::vector<URenderableObject*> RenderableObjects;

    UCamera* Camera = nullptr;
};