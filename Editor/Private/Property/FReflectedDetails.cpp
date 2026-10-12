#include "pch.h"
#include "Editor/Property/FReflectedDetails.h"
#include "Editor/Property/IPropertyEditorContext.h"
#include "CoreUObject/Reflection/UClass.h"

void FReflectedDetails::Draw(UObject& Object, IPropertyEditorContext& Context) {
    const TArray<const FProperty*> Properties{Object.GetClass()->GetProperties()};
    TArray<FString> Categories{};

    for (const FProperty* Property : Properties) {
        if (Property->HasAnyFlags(EPropertyFlags::Edit) && Property->CanWrite() && std::ranges::find(Categories, Property->GetCategory()) == Categories.end()) {
            Categories.push_back(Property->GetCategory());
        }
    }

    for (const FString& Category : Categories) {
        if (!Category.empty() && !Context.BeginCategory(Category.c_str())) {
            continue;
        }

        for (const FProperty* Property : Properties) {
            if (Property->HasAnyFlags(EPropertyFlags::Edit) && Property->CanWrite() && Property->GetCategory() == Category) {
                DrawProperty(Object, *Property, Context);
            }
        }
    }
}

void FReflectedDetails::DrawProperty(UObject& Object, const FProperty& Field, IPropertyEditorContext& Context) {
    const FProperty* Property{&Field};
    const FPropertyValue Value{Property->GetValue(Object)};
    const char* Label{Property->GetDisplayName().c_str()};
    const auto Write{[&Object, Property](const auto& NewValue) {
        Property->SetValue(Object, FPropertyValue{NewValue});
    }};

    if (const bool* Current{Value.Get<bool>()}) {
        Context.DrawBool(Label, *Current, Write);
    } else if (const float* Current{Value.Get<float>()}) {
        Context.DrawFloat(Label, *Current, Property->GetSpeed(), Property->GetMinimum(), Property->GetMaximum(), Write);
    } else if (const FVector2* Current{Value.Get<FVector2>()}) {
        Context.DrawVector2(Label, *Current, Property->GetSpeed(), Property->GetMinimum(), Property->GetMaximum(), Write);
    } else if (const FVector3* Current{Value.Get<FVector3>()}) {
        if (Property->HasAnyFlags(EPropertyFlags::Color)) {
            Context.DrawColor(Label, FVector4{*Current, 1.0f}, [&Object, Property](const FVector4& Color) {
                Property->SetValue(Object, FPropertyValue{FVector3{Color.mX, Color.mY, Color.mZ}});
            });
        } else {
            Context.DrawVector3(Label, *Current, Property->GetSpeed(), Property->GetMinimum(), Property->GetMaximum(), Write);
        }
    } else if (const FVector4* Current{Value.Get<FVector4>()}) {
        Context.DrawColor(Label, *Current, Write);
    } else if (const FString* Current{Value.Get<FString>()}) {
        Context.DrawText(Label, *Current, Write);
    } else if (const FTransform* Current{Value.Get<FTransform>()}) {
        Context.DrawTransform(Label, *Current, Write);
    } else {
        Context.DrawDisabledText(Label);
    }
}
