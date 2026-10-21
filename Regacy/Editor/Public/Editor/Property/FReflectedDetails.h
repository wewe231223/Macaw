#pragma once
#include "CoreUObject/Reflection/FPropertyValue.h"

class UObject;
class UStruct;
class FProperty;
class IPropertyEditorContext;

class FReflectedDetails {
public:
    using FCustomization = std::function<void(const FString&)>;
    using FWriter = std::function<void(const FProperty&, const FPropertyValue&)>;

public:
    static void Draw(UObject& Object, IPropertyEditorContext& Context, const FCustomization& Customization = {});
    static void Draw(const UStruct& Type, const void* Container, IPropertyEditorContext& Context, const FWriter& Writer, const FString& Category = {}, UObject* Owner = nullptr, const FCustomization& Customization = {});

private:
    static void DrawProperty(const void* Container, const FProperty& Field, IPropertyEditorContext& Context, const FWriter& Writer, UObject* Owner, const char* Label = nullptr);
    static void DrawObject(const void* Container, const FProperty& Field, IPropertyEditorContext& Context, const FWriter& Writer, UObject* Owner, const char* Label);
};
