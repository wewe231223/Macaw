#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <array>
#include <memory>

class FEditorContext;
class FEditorViewport;
class FPlayInEditorSession;
class IRendererModule;

class FEditorViewportLayout {
public:
    ReflectTypes(FEditorViewportLayout);

public:
    FEditorViewportLayout(FEditorContext* Context, FEditorViewport* Viewport, FPlayInEditorSession* PlaySession);
    ~FEditorViewportLayout();
    FEditorViewportLayout(const FEditorViewportLayout&) = delete;
    FEditorViewportLayout& operator=(const FEditorViewportLayout&) = delete;

public:
    ReflectStruct(FEditorViewportLayout);
    bool Initialize(IRendererModule* Renderer);
    void Shutdown();
    void DrawUI(float DeltaTime);
    void Draw(float DeltaTime);
    void SetQuadView(bool QuadView);
    bool IsQuadView() const;
    bool CanChangeLayout() const;
    FEditorViewport* GetActiveViewport() const;

private:
    ReflectMembers();
    FEditorViewport* GetViewport(Uint32 Index) const;
    void UpdateViewportContexts();
    void DrawViewport(Uint32 Index, float DeltaTime, FEditorViewport*& CapturedViewport);

private:
    FEditorContext* mContext{};
    ReflectProperty(mContext, "Context", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FEditorViewport* mViewport{};
    ReflectProperty(mViewport, "Viewport", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FPlayInEditorSession* mPlaySession{};
    ReflectProperty(mPlaySession, "PlaySession", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::array<std::unique_ptr<FEditorViewport>, 3> mOrthographicViewports{};
    ReflectProperty(mOrthographicViewports, "OrthographicViewports", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::array<bool, 4> mVisible{};
    ReflectProperty(mVisible, "Visible", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mActiveViewport{};
    ReflectProperty(mActiveViewport, "ActiveViewport", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mSingleViewport{};
    ReflectProperty(mSingleViewport, "SingleViewport", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mQuadView{};
    ReflectProperty(mQuadView, "QuadView", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mHorizontalRatio{0.5f};
    ReflectProperty(mHorizontalRatio, "HorizontalRatio", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mVerticalRatio{0.5f};
    ReflectProperty(mVerticalRatio, "VerticalRatio", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
