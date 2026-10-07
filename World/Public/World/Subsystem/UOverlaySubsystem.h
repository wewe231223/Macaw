#pragma once

#include "World/Subsystem/UWorldSubsystem.h"
#include "RenderCore/FOverlayRenderData.h"

class UOverlaySubsystem final : public UWorldSubsystem {
public:
    UOverlaySubsystem() = default;
    ~UOverlaySubsystem() override = default;

public:
    JG_DECLARE_DERIVED_TYPEINFO(UOverlaySubsystem, UWorldSubsystem)

    void BuildRenderProbes(FOverlayRenderData& Overlay, FObjectHandle SelectedActor) const;
};
