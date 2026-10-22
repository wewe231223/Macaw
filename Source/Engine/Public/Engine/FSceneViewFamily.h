#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FSceneView.h"

class FSceneInterface;
class FRenderTarget;
class FPrimitiveDrawInterface;

struct FSceneViewFamily {
public:
    ReflectTypes(FSceneViewFamily);

public:
    ReflectStruct(FSceneViewFamily);

private:
    ReflectMembers();

public:
    const FSceneInterface* mScene{};
    ReflectProperty(mScene, "Scene", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FRenderTarget* mTarget{};
    ReflectProperty(mTarget, "Target", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FSceneView mView{};
    ReflectProperty(mView, "View", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    const FPrimitiveDrawInterface* mEditorElements{};
    ReflectProperty(mEditorElements, "EditorElements", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
