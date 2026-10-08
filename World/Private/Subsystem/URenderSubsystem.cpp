#include "pch.h"
#include "World/Subsystem/URenderSubsystem.h"
#include "World/AActor.h"

void URenderSubsystem::RegisterComponent(UActorComponent* Component) {
    if (Component == nullptr || !Component->GetHandle().IsValid()) {
        return;
    }

    const FObjectHandle Handle{Component->GetHandle()};

    if (!mComponentIndices.emplace(GetComponentKey(Handle), mComponents.size()).second) {
        return;
    }

    mComponents.push_back(Component);
}

void URenderSubsystem::UnregisterComponent(UActorComponent* Component) {
    if (Component == nullptr) {
        return;
    }

    const FObjectHandle Handle{Component->GetHandle()};
    const auto Position{mComponentIndices.find(GetComponentKey(Handle))};

    if (Position == mComponentIndices.end() || mComponents[Position->second] != Component) {
        return;
    }

    const std::size_t Index{Position->second};

    if (Index + 1 != mComponents.size()) {
        mComponents[Index] = mComponents.back();
        mComponentIndices[GetComponentKey(mComponents[Index]->GetHandle())] = Index;
    }

    mComponents.pop_back();
    mComponentIndices.erase(Position);
}

void URenderSubsystem::MarkComponentDirty(UActorComponent* Component) {
    if (Component != nullptr && Component->GetHandle().IsValid() && mDirtyComponentKeys.insert(GetComponentKey(Component->GetHandle())).second) {
        mDirtyComponents.emplace_back(Component);
    }
}

void URenderSubsystem::RecreateRenderStates() {
    for (UActorComponent* Component : mComponents) {
        Component->MarkRenderStateDirty();
    }
}

void URenderSubsystem::AddPrimitive(std::unique_ptr<FPrimitiveSceneProxy> Proxy) {
    if (Proxy == nullptr || !Proxy->GetComponentHandle().IsValid()) {
        return;
    }

    FPrimitiveSceneUpdate& Update{FindOrAddPrimitiveUpdate(Proxy->GetComponentHandle())};

    Update.mType = ESceneUpdateType::Create;
    Update.mProxy = std::move(Proxy);
    Update.mTransform = {};
}

void URenderSubsystem::UpdatePrimitiveTransform(FObjectHandle ComponentHandle, const FPrimitiveTransform& Transform) {
    if (!ComponentHandle.IsValid()) {
        return;
    }

    const auto Position{mPrimitiveUpdateIndices.find(GetComponentKey(ComponentHandle))};

    if (Position != mPrimitiveUpdateIndices.end() && mPrimitiveUpdates[Position->second].mType == ESceneUpdateType::Remove) {
        return;
    }

    FPrimitiveSceneUpdate& Update{FindOrAddPrimitiveUpdate(ComponentHandle)};

    if (Update.mType == ESceneUpdateType::Create && Update.mProxy != nullptr) {
        Update.mProxy->SetTransform(Transform);
    } else {
        Update.mType = ESceneUpdateType::Transform;
        Update.mTransform = Transform;
    }
}

void URenderSubsystem::RemovePrimitive(FObjectHandle ComponentHandle) {
    if (!ComponentHandle.IsValid()) {
        return;
    }

    FPrimitiveSceneUpdate& Update{FindOrAddPrimitiveUpdate(ComponentHandle)};

    Update.mType = ESceneUpdateType::Remove;
    Update.mProxy.reset();
    Update.mTransform = {};
}

void URenderSubsystem::BuildSceneUpdates(FSceneUpdateBatch& Updates) {
    FlushDeferredRenderUpdates();
    Updates.mPrimitiveUpdates.clear();
    Updates.mPrimitiveUpdates.swap(mPrimitiveUpdates);
    mPrimitiveUpdateIndices.clear();
    Updates.mLightUpdates.clear();
    Updates.mLightUpdates.swap(mLightUpdates);
    mLightUpdateIndices.clear();
}

void URenderSubsystem::FlushDeferredRenderUpdates() {
    TArray<TObjectRef<UActorComponent>> Components{};

    Components.swap(mDirtyComponents);
    mDirtyComponentKeys.clear();

    for (const TObjectRef<UActorComponent>& Reference : Components) {
        if (UActorComponent* Component{Reference.Get()}) {
            Component->DoDeferredRenderUpdates();
        }
    }

    Components.clear();

    if (mDirtyComponents.empty()) {
        mDirtyComponents.swap(Components);
    }
}

bool URenderSubsystem::ContainsComponent(const UActorComponent* Component) const {
    if (Component == nullptr) {
        return false;
    }

    const auto Position{mComponentIndices.find(GetComponentKey(Component->GetHandle()))};

    return Position != mComponentIndices.end() && mComponents[Position->second] == Component;
}

const TArray<UActorComponent*>& URenderSubsystem::GetRegisteredComponents() const {
    return mComponents;
}

Uint64 URenderSubsystem::GetComponentKey(FObjectHandle Handle) {
    return static_cast<Uint64>(Handle.mGeneration) << 32 | Handle.mIndex;
}

FPrimitiveSceneUpdate& URenderSubsystem::FindOrAddPrimitiveUpdate(FObjectHandle Handle) {
    const auto Position{mPrimitiveUpdateIndices.emplace(GetComponentKey(Handle), mPrimitiveUpdates.size())};

    if (Position.second) {
        mPrimitiveUpdates.emplace_back();
        mPrimitiveUpdates.back().mComponentHandle = Handle;
    }

    return mPrimitiveUpdates[Position.first->second];
}

void URenderSubsystem::OnDeinitialize() {
    mComponents.clear();
    mComponentIndices.clear();

    mDirtyComponents.clear();
    mDirtyComponentKeys.clear();
    mPrimitiveUpdates.clear();
    mPrimitiveUpdateIndices.clear();

    mLightUpdates.clear();
    mLightUpdateIndices.clear();
}

void URenderSubsystem::AddLight(std::unique_ptr<FLightSceneProxy> Proxy) {
    if (Proxy == nullptr || !Proxy->GetComponentHandle().IsValid()) {
        return;
    }

    FLightSceneUpdate& Update{FindOrAddLightUpdate(Proxy->GetComponentHandle())};

    Update.mType = ESceneUpdateType::Create;
    Update.mProxy = std::move(Proxy);
    Update.mWorld = {};
}

void URenderSubsystem::UpdateLightTransform(FObjectHandle ComponentHandle, const FMatrix& World) {
    if (!ComponentHandle.IsValid()) {
        return;
    }

    const auto Position{mLightUpdateIndices.find(GetComponentKey(ComponentHandle))};

    if (Position != mLightUpdateIndices.end() && mLightUpdates[Position->second].mType == ESceneUpdateType::Remove) {
        return;
    }

    FLightSceneUpdate& Update{FindOrAddLightUpdate(ComponentHandle)};

    if (Update.mType == ESceneUpdateType::Create && Update.mProxy != nullptr) {
        Update.mProxy->SetTransform(World);
    } else {
        Update.mType = ESceneUpdateType::Transform;
        Update.mWorld = World;
    }
}

void URenderSubsystem::RemoveLight(FObjectHandle ComponentHandle) {
    if (!ComponentHandle.IsValid()) {
        return;
    }

    FLightSceneUpdate& Update{FindOrAddLightUpdate(ComponentHandle)};

    Update.mType = ESceneUpdateType::Remove;
    Update.mProxy.reset();
    Update.mWorld = {};
}

FLightSceneUpdate& URenderSubsystem::FindOrAddLightUpdate(FObjectHandle Handle) {
    const auto Position{mLightUpdateIndices.emplace(GetComponentKey(Handle), mLightUpdates.size())};

    if (Position.second) {
        mLightUpdates.emplace_back();
        mLightUpdates.back().mComponentHandle = Handle;
    }

    return mLightUpdates[Position.first->second];
}
