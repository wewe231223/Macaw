#pragma once

#include "World/Subsystem/UWorldSubsystem.h"
#include "RenderCore/FOverlayRenderData.h"

class UNameTagComponent;

class UOverlaySubsystem final : public UWorldSubsystem {
public:
    UOverlaySubsystem() = default;
    ~UOverlaySubsystem() override = default;

public:
    JG_DECLARE_DERIVED_TYPEINFO(UOverlaySubsystem, UWorldSubsystem)

    void RegisterComponent(UNameTagComponent* Component);
    void UnregisterComponent(UNameTagComponent* Component);
    void BuildRenderProbes(FOverlayRenderData& Overlay, FObjectHandle SelectedActor) const;

private:
    void OnDeinitialize() override;

private:
    TArray<UNameTagComponent*> mNameTags{};
};
