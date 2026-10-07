#pragma once

class UActorComponent;
class IPropertyEditorContext;
class UBillboardComponent;
class UBillboardTextComponent;
class UBoxColliderComponent;
class UCameraComponent;
class UCollisionComponent;
class ULightComponentBase;
class ULocalLightComponent;
class UMeshComponent;
class UPrimitiveComponent;
class USceneComponent;
class UScrollUVComponent;
class USpotLightComponent;
class UStaticMeshComponent;
class USubUVComponent;

class FComponentDetails {
public:
    static void Draw(UActorComponent& Component, IPropertyEditorContext& Context);

private:
    static void DrawActorComponent(UActorComponent& Component, IPropertyEditorContext& Context);
    static void DrawBillboardComponent(UBillboardComponent& Component, IPropertyEditorContext& Context);
    static void DrawBillboardTextComponent(UBillboardTextComponent& Component, IPropertyEditorContext& Context);
    static void DrawBoxColliderComponent(UBoxColliderComponent& Component, IPropertyEditorContext& Context);
    static void DrawCameraComponent(UCameraComponent& Component, IPropertyEditorContext& Context);
    static void DrawCollisionComponent(UCollisionComponent& Component, IPropertyEditorContext& Context);
    static void DrawLightComponentBase(ULightComponentBase& Component, IPropertyEditorContext& Context);
    static void DrawLocalLightComponent(ULocalLightComponent& Component, IPropertyEditorContext& Context);
    static void DrawMeshComponent(UMeshComponent& Component, IPropertyEditorContext& Context);
    static void DrawPrimitiveComponent(UPrimitiveComponent& Component, IPropertyEditorContext& Context);
    static void DrawSceneComponent(USceneComponent& Component, IPropertyEditorContext& Context);
    static void DrawScrollUVComponent(UScrollUVComponent& Component, IPropertyEditorContext& Context);
    static void DrawSpotLightComponent(USpotLightComponent& Component, IPropertyEditorContext& Context);
    static void DrawStaticMeshComponent(UStaticMeshComponent& Component, IPropertyEditorContext& Context);
    static void DrawSubUVComponent(USubUVComponent& Component, IPropertyEditorContext& Context);
};
