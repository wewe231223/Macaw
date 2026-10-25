#include "Engine/ABillboardTextActor.h"
#include "Engine/Component/UBillboardTextComponent.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/Cast.h"

#include <utility>

ABillboardTextActor::ABillboardTextActor() = default;

ABillboardTextActor::~ABillboardTextActor() = default;

bool ABillboardTextActor::InitializeDefaultComponents() {
    if (!GetComponents().empty()) {
        return false;
    }
    auto Component{NewObject<UBillboardTextComponent>()};
    Component->SetName("BillboardTextComponent");
    return AddComponent(std::move(Component));
}

UBillboardTextComponent* ABillboardTextActor::GetTextComponent() const {
    return Cast<UBillboardTextComponent>(GetRootComponent());
}
