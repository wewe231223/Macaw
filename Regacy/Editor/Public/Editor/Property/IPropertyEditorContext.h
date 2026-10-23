#pragma once
#include "Core/Base/FAssetHandle.h"
#include "Core/Base/FTransform.h"
#include "Core/STL.h"

#include <functional>
#include <vector>
#include "CoreUObject/Reflection/UClass.h"

struct FPropertyReferenceOption {
    const void* mId{nullptr};
    FString mLabel{};
    bool mBSelected{false};
    std::function<void()> mOnSelected{};
};

class IPropertyEditorContext {
public:
    virtual ~IPropertyEditorContext() = default;

public:
    virtual void BeginProperty(const char* Label) const = 0;
    virtual void EndProperty() const = 0;
    virtual bool BeginCategory(const char* Label, bool BDefaultOpen = true) const = 0;
    virtual void DrawDisabledText(const char* Text) const = 0;
    virtual void DrawButton(const char* Label, const std::function<void()>& OnClicked) const = 0;

    virtual void DrawBool(const char* Label, bool Value, const std::function<void(bool)>& Setter) const = 0;
    virtual void DrawInteger(const char* Label, Int64 Value, float Speed, Int64 Min, Int64 Max, const std::function<void(Int64)>& Setter) const = 0;
    virtual void DrawFloat(const char* Label, float Value, float Speed, float Min, float Max, const std::function<void(float)>& Setter) const = 0;
    virtual void DrawVector2(const char* Label, const FVector2& Value, float Speed, float Min, float Max, const std::function<void(const FVector2&)>& Setter) const = 0;
    virtual void DrawVector3(const char* Label, const FVector3& Value, float Speed, float Min, float Max, const std::function<void(const FVector3&)>& Setter) const = 0;
    virtual void DrawColor(const char* Label, const FVector4& Value, const std::function<void(const FVector4&)>& Setter) const = 0;
    virtual void DrawText(const char* Label, const FString& Value, const std::function<void(const FString&)>& Setter) const = 0;
    virtual void DrawTransform(const char* Label, const FTransform& Value, const std::function<void(const FTransform&)>& Setter) = 0;

    virtual void DrawReferencePicker(const char* Label, const char* Preview, bool BNoneSelected, const std::function<void()>& ClearSelection, const std::vector<FPropertyReferenceOption>& Options) const = 0;
    virtual void DrawAssetPicker(const char* Label, const UClass& AssetType, FAssetHandle CurrentHandle, const std::function<void(FAssetHandle)>& Setter) const = 0;
};
