#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FVector4.h"
#include "Engine/FHitProxy.h"
#include "Engine/EEditorMesh.h"

struct FDrawMesh {
public:
    ReflectTypes(FDrawMesh);

public:
    ReflectStruct(FDrawMesh);

private:
    ReflectMembers();

public:
    EEditorMesh mMesh{EEditorMesh::Count};
    ReflectProperty(mMesh, "Mesh", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FMatrix mWorld{};
    ReflectProperty(mWorld, "World", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
    ReflectProperty(mColor, "Color", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FHitProxy mHitProxy{};
    ReflectProperty(mHitProxy, "HitProxy", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mPriority{2};
    ReflectProperty(mPriority, "Priority", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mDepthTest{};
    ReflectProperty(mDepthTest, "DepthTest", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
