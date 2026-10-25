#pragma once

#include "Editor/FEditorContext.h"
#include "Engine/FGameViewportClient.h"
#include <utility>

class UEngine;
class FEditorViewport;

class FPlayInEditorSession {
public:
    ReflectTypes(FPlayInEditorSession);

public:
    FPlayInEditorSession(UEngine* Engine, FEditorContext* Context, FEditorViewport* Viewport);
    ~FPlayInEditorSession();
    FPlayInEditorSession(const FPlayInEditorSession&) = delete;
    FPlayInEditorSession& operator=(const FPlayInEditorSession&) = delete;

public:
    ReflectStruct(FPlayInEditorSession);
    bool CanStart() const;
    bool Start();
    bool Stop();
    bool SetPaused(bool Paused);
    bool Step(float DeltaTime = 1.0f / 60.0f);
    void Update();
    bool IsActive() const;
    bool IsPaused() const;
    FWorldHandle GetWorldHandle() const;
    FEditorContext* GetInspectionContext();
    FGameViewportClient* GetViewportClient();
    FObjectHandle FindPlayObject(FObjectHandle Original) const;
    FObjectHandle FindEditorObject(FObjectHandle Duplicate) const;

private:
    ReflectMembers();

private:
    UEngine* mEngine{};
    ReflectProperty(mEngine, "Engine", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FEditorContext* mContext{};
    ReflectProperty(mContext, "Context", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FEditorViewport* mViewport{};
    ReflectProperty(mViewport, "Viewport", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FEditorContext mInspectionContext;
    ReflectProperty(mInspectionContext, "InspectionContext", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FGameViewportClient mGameClient;
    ReflectProperty(mGameClient, "GameClient", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FWorldHandle mPlayWorld{};
    ReflectProperty(mPlayWorld, "PlayWorld", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FWorldHandle mPreviousWorld{};
    ReflectProperty(mPreviousWorld, "PreviousWorld", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<std::pair<FObjectHandle, FObjectHandle>> mObjectPairs{};
    ReflectProperty(mObjectPairs, "ObjectPairs", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mTransitioning{};
    ReflectProperty(mTransitioning, "Transitioning", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mStopRequested{};
    ReflectProperty(mStopRequested, "StopRequested", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
