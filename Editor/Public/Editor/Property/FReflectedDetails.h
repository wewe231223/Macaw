#pragma once

class UObject;
class FProperty;
class IPropertyEditorContext;

class FReflectedDetails {
public:
    static void Draw(UObject& Object, IPropertyEditorContext& Context);

private:
    static void DrawProperty(UObject& Object, const FProperty& Field, IPropertyEditorContext& Context);
};
