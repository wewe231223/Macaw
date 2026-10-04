#include "Editor/World/FWorldEditorContext.h"
#include "Editor/Property/FComponentDetails.h"
#include "Editor/Property/IPropertyEditorContext.h"
#include "World/UWorld.h"
#include "World/Component/UScrollUVComponent.h"
#include "World/Component/USubUVComponent.h"

#include <iostream>
#include <stdexcept>
#include <string_view>

class FDetailsTestContext final : public IPropertyEditorContext {
public:
    bool BeginCategory(const char* Label, bool DefaultOpen) const override;
    void DrawDisabledText(const char* Text) const override;
    void DrawButton(const char* Label, const std::function<void()>& OnClicked) const override;
    void DrawBool(const char* Label, bool Value, const std::function<void(bool)>& Setter) const override;
    void DrawFloat(const char* Label, float Value, float Speed, float Min, float Max, const std::function<void(float)>& Setter) const override;
    void DrawVector2(const char* Label, const FVector2& Value, float Speed, float Min, float Max, const std::function<void(const FVector2&)>& Setter) const override;
    void DrawVector3(const char* Label, const FVector3& Value, float Speed, float Min, float Max, const std::function<void(const FVector3&)>& Setter) const override;
    void DrawColor(const char* Label, const FVector4& Value, const std::function<void(const FVector4&)>& Setter) const override;
    void DrawText(const char* Label, const FString& Value, const std::function<void(const FString&)>& Setter) const override;
    void DrawTransform(const char* Label, const FTransform& Value, const std::function<void(const FTransform&)>& Setter) override;
    void DrawReferencePicker(const char* Label, const char* Preview, bool NoneSelected, const std::function<void()>& ClearSelection, const std::vector<FPropertyReferenceOption>& Options) const override;
    void DrawAssetPicker(const char* Label, const FTypeInfo& AssetType, FAssetHandle Handle, const std::function<void(FAssetHandle)>& Setter) const override;
};

bool FDetailsTestContext::BeginCategory(const char*, bool) const {
    return true;
}

void FDetailsTestContext::DrawDisabledText(const char*) const {
}

void FDetailsTestContext::DrawButton(const char*, const std::function<void()>&) const {
}

void FDetailsTestContext::DrawBool(const char* Label, bool, const std::function<void(bool)>& Setter) const {
    if (std::string_view{Label} == "Active") {
        Setter(false);
    }
}

void FDetailsTestContext::DrawFloat(const char* Label, float, float, float, float, const std::function<void(float)>& Setter) const {
    if (std::string_view{Label} == "FrameRate") {
        Setter(12.0f);
    }
}

void FDetailsTestContext::DrawVector2(const char* Label, const FVector2&, float, float, float, const std::function<void(const FVector2&)>& Setter) const {
    if (std::string_view{Label} == "ScrollSpeed") {
        Setter(FVector2{0.3f, 0.4f});
    } else if (std::string_view{Label} == "SubImage") {
        Setter(FVector2{4.0f, 4.0f});
    }
}

void FDetailsTestContext::DrawVector3(const char*, const FVector3&, float, float, float, const std::function<void(const FVector3&)>&) const {
}

void FDetailsTestContext::DrawColor(const char*, const FVector4&, const std::function<void(const FVector4&)>&) const {
}

void FDetailsTestContext::DrawText(const char*, const FString&, const std::function<void(const FString&)>&) const {
}

void FDetailsTestContext::DrawTransform(const char*, const FTransform&, const std::function<void(const FTransform&)>&) {
}

void FDetailsTestContext::DrawReferencePicker(const char*, const char*, bool, const std::function<void()>&, const std::vector<FPropertyReferenceOption>&) const {
}

void FDetailsTestContext::DrawAssetPicker(const char*, const FTypeInfo&, FAssetHandle, const std::function<void(FAssetHandle)>&) const {
}

static void Require(bool Condition, const char* Message) {
    if (!Condition) {
        throw std::runtime_error{Message};
    }
}

static void CheckSelectionLifetime() {
    FWorldEditorContext Context{};
    {
        UWorld World{};
        Context.SetWorld(&World);
        Context.InitializeChannels(nullptr);
        AActor* Actor{World.AdoptActor<AActor>()};
        Actor->SetRootComponent(Actor->AddComponent<USceneComponent>());
        Context.SetSelectedActor(Actor);
        Require(Context.GetSelectedActor() == Actor && Context.GetSelectedComponent() != nullptr, "selection failed");
        World.DestroyActor(Actor);
        World.FlushPendingDestroyActors();
        Require(Context.GetSelectedActor() == nullptr && Context.GetSelectedComponent() == nullptr, "selection survived actor removal");
        Context.GetEditorToWorldSender().TryEmplace<FMessageLoadScene>(FString{"MissingScene.json"});
    }
    Require(Context.GetWorld() == nullptr, "editor context retained a destroyed world");
    Context.Dispatch();
    UWorld NextWorld{};
    Context.SetWorld(&NextWorld);
    AActor* Actor{NextWorld.AdoptActor<AActor>()};
    Context.SetSelectedActor(Actor);
    Context.SetWorld(nullptr);
    Require(Context.GetSelectedActor() == nullptr, "world detachment retained selection");
}

static void CheckComponentDetails() {
    FDetailsTestContext Details{};
    UScrollUVComponent Scroll{};
    FComponentDetails::Draw(Scroll, Details);
    Require(!Scroll.IsActive() && Scroll.GetScrollSpeed().mX == 0.3f, "scroll details lost inherited or specific properties");
    USubUVComponent SubUV{};
    FComponentDetails::Draw(SubUV, Details);
    Require(SubUV.GetFrameRate() == 12.0f && SubUV.GetSubImageHorizontal() == 4 && SubUV.GetSubImageVertical() == 4, "sub-UV details failed to apply edits");
}

int main() {
    try {
        CheckSelectionLifetime();
        CheckComponentDetails();
        std::cout << "Editor boundary tests passed.\n";
        return 0;
    } catch (const std::exception& Error) {
        std::cerr << Error.what() << '\n';
        return 1;
    }
}
