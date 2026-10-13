#include "pch.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "World/AActor.h"
#include "World/Component/UActorComponent.h"
#include "World/Component/UBillboardComponent.h"
#include "World/Component/UBillboardTextComponent.h"
#include "World/Component/UBoxColliderComponent.h"
#include "World/Component/UCameraComponent.h"
#include "World/Component/UCollisionComponent.h"
#include "World/Component/UDirectionalLightComponent.h"
#include "World/Component/UFlipBookComponent.h"
#include "World/Component/ULightComponent.h"
#include "World/Component/ULightComponentBase.h"
#include "World/Component/ULocalLightComponent.h"
#include "World/Component/UMeshComponent.h"
#include "World/Component/UPointLightComponent.h"
#include "World/Component/UPrimitiveComponent.h"
#include "World/Component/USceneComponent.h"
#include "World/Component/USpotLightComponent.h"
#include "World/Component/UStaticMeshComponent.h"
#include "World/Component/USubUVComponent.h"
#include "World/Component/UUVScrollComponent.h"
#include "World/EEndPlayReason.h"
#include "World/EWorldType.h"
#include "World/Subsystem/UCameraSubsystem.h"
#include "World/Subsystem/UCollisionSubsystem.h"
#include "World/Subsystem/UOverlaySubsystem.h"
#include "World/Subsystem/UPickingSubsystem.h"
#include "World/Subsystem/URenderSubsystem.h"
#include "World/Subsystem/UWorldSubsystem.h"
#include "World/ULevel.h"
#include "World/UWorld.h"

void ReflectionModules::RegisterWorld() {
    AActor::StaticClass();
    UActorComponent::StaticClass();
    UBillboardComponent::StaticClass();
    UBillboardTextComponent::StaticClass();
    FBoxColliderBounds::StaticStruct();
    UBoxColliderComponent::StaticClass();
    UCameraComponent::StaticClass();
    UCollisionComponent::StaticClass();
    UDirectionalLightComponent::StaticClass();
    UFlipBookComponent::StaticClass();
    ULightComponent::StaticClass();
    ULightComponentBase::StaticClass();
    ULocalLightComponent::StaticClass();
    UMeshComponent::StaticClass();
    UPointLightComponent::StaticClass();
    UPrimitiveComponent::StaticClass();
    USceneComponent::StaticClass();
    USpotLightComponent::StaticClass();
    UStaticMeshComponent::StaticClass();
    USubUVComponent::StaticClass();
    UUVScrollComponent::StaticClass();
    StaticEnumEEndPlayReason();
    StaticEnumEWorldType();
    UCameraSubsystem::StaticClass();
    UCollisionSubsystem::StaticClass();
    UOverlaySubsystem::StaticClass();
    UPickingSubsystem::StaticClass();
    URenderSubsystem::StaticClass();
    UWorldSubsystem::StaticClass();
    ULevel::StaticClass();
    UWorld::StaticClass();
}
