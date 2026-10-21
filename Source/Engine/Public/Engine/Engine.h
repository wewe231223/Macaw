#pragma once

#include "CoreUObject/UObject.h"

class UWorld;
class FWorldContext;
class IRendererModule;

class UEngine : public UObject {
public:
    ReflectTypes(UEngine, UObject);

public:
    UEngine();
    ~UEngine() override;

public:
    ReflectClass(UEngine);

    virtual bool Initialize(IRendererModule& Renderer);
    virtual void Shutdown();

    UWorld* GetWorld() const;
    FWorldContext* GetWorldContext() const;
    IRendererModule* GetRenderer() const;

    bool ReplaceWorld(std::unique_ptr<FWorldContext> Context);
    Uint64 GetWorldGeneration() const;

    void SynchronizeWorld();

private:
    ReflectMembers();

private:
    std::unique_ptr<FWorldContext> mWorldContext{};
    IRendererModule* mRenderer{};
    Uint64 mWorldGeneration{};
};
