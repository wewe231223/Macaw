#pragma once
#include "World/Subsystem/UWorldSubsystem.h"

#include "Core/Base//FRenderProbe.h"
#include "World/Component/UBillboardComponent.h"
#include "Asset/IAssetRegistryMutator.h"

class UBillboardSubsystem : public UWorldSubsystem {
public:
    UBillboardSubsystem() = default;
    ~UBillboardSubsystem() override = default;

    JG_DECLARE_DERIVED_TYPEINFO(UBillboardSubsystem, UWorldSubsystem);

    void RegisterComponent(UBillboardComponent* Component);
    void UnregisterComponent(UBillboardComponent* Component);
    void BuildRenderProbes(IAssetRegistryMutator* AssetRegistryMutator, FRenderProbe& Probe) const;

    bool ContainsComponent(const UBillboardComponent* Component);
    const TArray<UBillboardComponent*>& GetRegisteredComponents() const;

protected:
    void OnDeinitialize() override;

private:
    TArray<UBillboardComponent*> mComponents{};
};
