#pragma once

class UActorComponent;
class IPropertyEditorContext;
class USceneComponent;
class USubUVComponent;

class FComponentDetails {
public:
    static void Draw(UActorComponent& Component, IPropertyEditorContext& Context);

private:
    static void DrawAttachmentActions(USceneComponent& Component, IPropertyEditorContext& Context);
    static void DrawPlaybackActions(USubUVComponent& Component, IPropertyEditorContext& Context);
};
