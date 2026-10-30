#include "pch.h"
#include "Editor/Property/FReflectedDetails.h"
#include "Editor/Property/IPropertyEditorContext.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/UObjectSystem.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "World/AActor.h"
#include "World/Component/UActorComponent.h"
#include <cmath>
#include <limits>

void FReflectedDetails::Draw(UObject& Object, IPropertyEditorContext& Context, const FCustomization& Customization) {
    Draw(*Object.GetClass(), &Object, Context, [&Object](const FProperty& Property, const FPropertyValue& Value) {
        Property.SetValue(Object, Value);
    }, {}, &Object, Customization);
}

void FReflectedDetails::Draw(const UStruct& Type, const void* Container, IPropertyEditorContext& Context, const FWriter& Writer, const FString& Category, UObject* Owner, const FCustomization& Customization) {
    const TArray<const FProperty*> Properties{Type.GetProperties()};
    TArray<FString> Categories{};

    for (const FProperty* Property : Properties) {
        if (Property->HasAnyFlags(EPropertyFlags::Edit) && Property->CanWrite() && (Category.empty() || Property->GetCategory() == Category) && std::ranges::find(Categories, Property->GetCategory()) == Categories.end()) {
            Categories.push_back(Property->GetCategory());
        }
    }

    for (const FString& CurrentCategory : Categories) {
        if (Category.empty() && !CurrentCategory.empty() && !Context.BeginCategory(CurrentCategory.c_str())) {
            continue;
        }

        for (const FProperty* Property : Properties) {
            if (Property->HasAnyFlags(EPropertyFlags::Edit) && Property->CanWrite() && Property->GetCategory() == CurrentCategory) {
                DrawProperty(Container, *Property, Context, Writer, Owner);
            }
        }

        if (Customization) {
            Customization(CurrentCategory);
        }
    }
}

void FReflectedDetails::DrawProperty(const void* Container, const FProperty& Field, IPropertyEditorContext& Context, const FWriter& Writer, UObject* Owner, const char* Label) {
    const FProperty* Property{&Field};
    const FPropertyValue Value{Property->ReadContainer(Container)};
    const char* DisplayName{Label != nullptr ? Label : Property->GetDisplayName().c_str()};

    if (!Property->CanEdit(Container)) {
        Context.DrawDisabledText(DisplayName);
        return;
    }

    const double Minimum{Property->GetMinimum(Container)};
    const double Maximum{std::max(Minimum, Property->GetMaximum(Container))};
    const float FloatMinimum{static_cast<float>(std::clamp(Minimum, -static_cast<double>(std::numeric_limits<float>::max()), static_cast<double>(std::numeric_limits<float>::max())))};
    const float FloatMaximum{static_cast<float>(std::clamp(Maximum, -static_cast<double>(std::numeric_limits<float>::max()), static_cast<double>(std::numeric_limits<float>::max())))};

    const auto Write{[Property, Writer](const auto& NewValue) {
        Writer(*Property, FPropertyValue{NewValue});
    }};

    if (Property->HasAnyFlags(EPropertyFlags::Asset) && Property->GetAssetClass() != nullptr && Value.Is<FAssetHandle>()) {
        Context.DrawAssetPicker(DisplayName, *Property->GetAssetClass(), *Value.Get<FAssetHandle>(), Write);
    } else if (Property->GetKind() == EPropertyKind::Object) {
        DrawObject(Container, Field, Context, Writer, Owner, DisplayName);
    } else if (const bool* Current{Value.Get<bool>()}) {
        Context.DrawBool(DisplayName, *Current, Write);
    } else if (const Int32* Current{Value.Get<Int32>()}) {
        const Int64 Min{static_cast<Int64>(std::ceil(std::clamp(Minimum, static_cast<double>(std::numeric_limits<Int32>::min()), static_cast<double>(std::numeric_limits<Int32>::max()))))};
        const Int64 Max{static_cast<Int64>(std::floor(std::clamp(Maximum, static_cast<double>(Min), static_cast<double>(std::numeric_limits<Int32>::max()))))};
        Context.DrawInteger(DisplayName, *Current, Property->GetSpeed(), Min, Max, [Write](Int64 Number) {
            Write(static_cast<Int32>(Number));
        });
    } else if (const Uint32* Current{Value.Get<Uint32>()}) {
        const Int64 Min{static_cast<Int64>(std::ceil(std::clamp(Minimum, 0.0, static_cast<double>(std::numeric_limits<Uint32>::max()))))};
        const Int64 Max{static_cast<Int64>(std::floor(std::clamp(Maximum, static_cast<double>(Min), static_cast<double>(std::numeric_limits<Uint32>::max()))))};
        Context.DrawInteger(DisplayName, *Current, Property->GetSpeed(), Min, Max, [Write](Int64 Number) {
            Write(static_cast<Uint32>(Number));
        });
    } else if (const float* Current{Value.Get<float>()}) {
        Context.DrawFloat(DisplayName, *Current, Property->GetSpeed(), FloatMinimum, FloatMaximum, Write);
    } else if (const FVector2* Current{Value.Get<FVector2>()}) {
        Context.DrawVector2(DisplayName, *Current, Property->GetSpeed(), FloatMinimum, FloatMaximum, Write);
    } else if (const FVector3* Current{Value.Get<FVector3>()}) {
        if (Property->HasAnyFlags(EPropertyFlags::Color)) {
            Context.DrawColor(DisplayName, FVector4{*Current, 1.0f}, [Write](const FVector4& Color) {
                Write(FVector3{Color.mX, Color.mY, Color.mZ});
            });
        } else {
            Context.DrawVector3(DisplayName, *Current, Property->GetSpeed(), FloatMinimum, FloatMaximum, Write);
        }
    } else if (const FVector4* Current{Value.Get<FVector4>()}) {
        Context.DrawColor(DisplayName, *Current, Write);
    } else if (const FString* Current{Value.Get<FString>()}) {
        Context.DrawText(DisplayName, *Current, Write);
    } else if (const FName* Current{Value.Get<FName>()}) {
        if (Property->HasOptions()) {
            const TArray<FName> Names{Property->GetOptions(Container)};
            std::vector<FPropertyReferenceOption> Options{};

            for (const FName& Name : Names) {
                Options.push_back({nullptr, Name.ToString(), Name == *Current, [Write, Name] {
                    Write(Name);
                }});
            }

            const FString Preview{Current->IsNone() ? FString{"None"} : Current->ToString()};
            Context.DrawReferencePicker(DisplayName, Preview.c_str(), Current->IsNone(), [Write] {
                Write(FName{});
            }, Options);
        } else {
            Context.DrawText(DisplayName, Current->ToString(), [Write](const FString& Text) {
                Write(FName{Text});
            });
        }
    } else if (const FTransform* Current{Value.Get<FTransform>()}) {
        Context.DrawTransform(DisplayName, *Current, Write);
    } else if (const UScriptStruct* Struct{Property->GetStruct()}; Struct != nullptr && !Value.IsEmpty()) {
        const auto WriteChild{[Container, Property, Writer](const FProperty& Child, const FPropertyValue& ChildValue) {
            FPropertyValue Edited{Property->ReadContainer(Container)};

            if (Child.WriteContainer(Edited.GetMutableData(), ChildValue)) {
                Writer(*Property, Edited);
            }
        }};

        const TArray<const FProperty*> Children{Struct->GetProperties()};
        const auto Editable{std::ranges::count_if(Children, [](const FProperty* Child) {
            return Child->HasAnyFlags(EPropertyFlags::Edit) && Child->CanWrite();
        })};

        Context.BeginProperty(DisplayName);

        if (Editable == 1) {
            for (const FProperty* Child : Children) {
                if (Child->HasAnyFlags(EPropertyFlags::Edit) && Child->CanWrite()) {
                    DrawProperty(Value.GetData(), *Child, Context, WriteChild, Owner, DisplayName);
                }
            }
        } else if (Context.BeginCategory(DisplayName)) {
            Draw(*Struct, Value.GetData(), Context, WriteChild, {}, Owner);
        }

        Context.EndProperty();
    } else {
        Context.DrawDisabledText(DisplayName);
    }
}

void FReflectedDetails::DrawObject(const void* Container, const FProperty& Field, IPropertyEditorContext& Context, const FWriter& Writer, UObject* Owner, const char* Label) {
    const FProperty* Property{&Field};
    UObject* Current{Field.ReadObject(Container)};
    const FString Preview{Current != nullptr ? Current->GetName().ToString() : FString{"None"}};
    std::vector<FPropertyReferenceOption> Options{};
    const UActorComponent* Component{Cast<UActorComponent>(Owner)};
    const AActor* Actor{Component != nullptr ? Component->GetOwner() : Cast<AActor>(Owner)};
    const auto AddCandidate{[&Options, Container, Property, Current, Writer](UObject* Candidate) {
        if (Candidate != nullptr && !Candidate->IsClassDefaultObject() && Property->CanSelectObject(Container, Candidate)) {
            Options.push_back({Candidate, Candidate->GetName().ToString(), Candidate == Current, [Property, Candidate, Writer] {
                Writer(*Property, Property->MakeObjectValue(Candidate));
            }});
        }
    }};

    if (Field.HasAnyFlags(EPropertyFlags::OwnerOnly)) {
        if (Actor != nullptr) {
            for (const std::unique_ptr<UActorComponent>& Candidate : Actor->GetComponents()) {
                AddCandidate(Candidate.get());
            }
        }
    } else {
        for (UObject* Candidate : UObjectSystem::GetObjects()) {
            AddCandidate(Candidate);
        }
    }

    Context.DrawReferencePicker(Label, Preview.c_str(), Current == nullptr, [Container, Property, Writer] {
        if (Property->CanSelectObject(Container, nullptr)) {
            Writer(*Property, Property->MakeObjectValue(nullptr));
        }
    }, Options);
}
