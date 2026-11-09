#pragma once

#include "Engine/AActor.h"
#include "ADecalActor.generated.h"

namespace Engine {

    class UDecalComponent;

    UCLASS()
    class ADecalActor final : public Engine::AActor {
        GENERATED_BODY()

    public:
        bool InitializeDefaultComponents() override;
        Engine::UDecalComponent* GetDecalComponent() const;
    };

}
