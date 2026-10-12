#pragma once

#include "World/Subsystem/UWorldSubsystem.h"
#include "RenderCore/FOverlayRenderData.h"
#include "World/Subsystem/UOverlaySubsystem.generated.h"

UCLASS()
class UOverlaySubsystem final : public UWorldSubsystem {
public:
    GENERATED_TYPES(UOverlaySubsystem)

public:
    UOverlaySubsystem();
    ~UOverlaySubsystem() override = default;

public:
    GENERATED_BODY(UOverlaySubsystem)

    void BuildDrawData(FOverlayRenderData& Overlay, FObjectHandle SelectedActor) const;
};
