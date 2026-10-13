#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include "World/Subsystem/UWorldSubsystem.h"
#include "RenderCore/FOverlayRenderData.h"

class UOverlaySubsystem final : public UWorldSubsystem {
public:
    ReflectTypes(UOverlaySubsystem, UWorldSubsystem);

public:
    UOverlaySubsystem();
    ~UOverlaySubsystem() override = default;

public:
    ReflectClass(UOverlaySubsystem);

    void BuildDrawData(FOverlayRenderData& Overlay, FObjectHandle SelectedActor) const;
};
