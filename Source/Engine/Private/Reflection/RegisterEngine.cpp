#include "Engine/Reflection/RegisterEngine.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/Component/UStaticMeshComponent.h"
#include "Engine/AActor.h"
#include "Engine/AStaticMeshActor.h"
#include "Engine/ULevel.h"
#include "Engine/UWorld.h"
#include "CoreUObject/Reflection/RegisterCoreUObject.h"

void ReflectionModules::RegisterEngine() {
    ReflectionModules::RegisterCoreUObject();

    UActorComponent::StaticClass();
    USceneComponent::StaticClass();
    UPrimitiveComponent::StaticClass();
    UStaticMeshComponent::StaticClass();

    AActor::StaticClass();
    AStaticMeshActor::StaticClass();

    ULevel::StaticClass();
    UWorld::StaticClass();
}
