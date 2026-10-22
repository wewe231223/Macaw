#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/Component/USceneComponent.h"

enum class EPrimitiveType : Uint8;
struct FPrimitiveSceneProxy;
struct FBox;

class UPrimitiveComponent : public USceneComponent {
public:
    ReflectTypes(UPrimitiveComponent, USceneComponent);

public:
    UPrimitiveComponent();
    ~UPrimitiveComponent() override;

public:
    ReflectClass(UPrimitiveComponent);

    void RegisterComponent(UWorld& World) override;
    void UnregisterComponent() override;

    bool SetTransform(const FTransform& Transform) override;

    void MarkRenderStateDirty();
    void MarkRenderTransformDirty();
    void SendRenderUpdates();

    virtual EPrimitiveType GetPrimitiveType() const = 0;
    FPrimitiveSceneProxy CreateSceneProxy() const;
    FBox GetWorldBounds() const;

private:
    ReflectMembers();

private:
    void QueueUpdate();

private:
    bool mRenderStateDirty{true};
    ReflectProperty(mRenderStateDirty, "RenderStateDirty", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mRenderTransformDirty{};
    ReflectProperty(mRenderTransformDirty, "RenderTransformDirty", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mQueued{};
    ReflectProperty(mQueued, "Queued", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mRenderStateCreated{};
    ReflectProperty(mRenderStateCreated, "RenderStateCreated", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
