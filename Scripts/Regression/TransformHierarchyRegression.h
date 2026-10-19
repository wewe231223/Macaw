#pragma once

#include "World/Component/UPrimitiveComponent.h"

#include <functional>

class UTransformProbe final : public UPrimitiveComponent {
public:
    ReflectTypes(UTransformProbe, UPrimitiveComponent);

public:
    UTransformProbe();
    ~UTransformProbe() override = default;

public:
    ReflectClass(UTransformProbe);
    void OnTransformUpdate() override;
    bool ShouldCreateRenderState() const override;
    std::unique_ptr<FPrimitiveSceneProxy> CreateSceneProxy() const override;
    Uint64 GetNotificationCount() const;
    void SetOnUpdate(std::function<void()> OnUpdate);

private:
    Uint64 mNotificationCount{};
    std::function<void()> mOnUpdate{};

    ReflectMembers();
};
