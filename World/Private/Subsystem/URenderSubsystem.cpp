#include "pch.h"
#include "World/Subsystem/URenderSubsystem.h"
#include "World/AActor.h"

URenderSubsystem::URenderSubsystem()
	: mSceneId{AllocateRenderSceneId()} {
}

void URenderSubsystem::RegisterComponent(UStaticMeshComponent* Component) {
    if (Component == nullptr || !Component->GetHandle().IsValid()) {
        return;
    }

    const FObjectHandle Handle{Component->GetHandle()};

    if (!mComponentIndices.emplace(GetComponentKey(Handle), mComponents.size()).second) {
        return;
    }

    mComponents.push_back(Component);
}

void URenderSubsystem::UnregisterComponent(UStaticMeshComponent* Component) {
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
    for (UStaticMeshComponent* Component : mComponents) {
        Component->MarkRenderStateDirty();
    }
}

void URenderSubsystem::AddPrimitive(std::unique_ptr<FPrimitiveSceneProxy> Proxy) {
    if (Proxy == nullptr || !Proxy->GetComponentHandle().IsValid()) {
        return;
    }

    FPrimitiveSceneUpdate& Update{FindOrAddPrimitiveUpdate(Proxy->GetComponentHandle())};

    Update.mType = EPrimitiveSceneUpdate::Create;
    Update.mProxy = std::move(Proxy);
    Update.mTransform = {};
}

void URenderSubsystem::UpdatePrimitiveTransform(FObjectHandle ComponentHandle, const FPrimitiveTransform& Transform) {
    if (!ComponentHandle.IsValid()) {
        return;
    }

    const auto Position{mPrimitiveUpdateIndices.find(GetComponentKey(ComponentHandle))};

    if (Position != mPrimitiveUpdateIndices.end() && mPrimitiveUpdates[Position->second].mType == EPrimitiveSceneUpdate::Remove) {
        return;
    }

    FPrimitiveSceneUpdate& Update{FindOrAddPrimitiveUpdate(ComponentHandle)};

    if (Update.mType == EPrimitiveSceneUpdate::Create && Update.mProxy != nullptr) {
        Update.mProxy->SetTransform(Transform);
    } else {
        Update.mType = EPrimitiveSceneUpdate::Transform;
        Update.mTransform = Transform;
    }
}

void URenderSubsystem::RemovePrimitive(FObjectHandle ComponentHandle) {
    if (!ComponentHandle.IsValid()) {
        return;
    }

    FPrimitiveSceneUpdate& Update{FindOrAddPrimitiveUpdate(ComponentHandle)};

    Update.mType = EPrimitiveSceneUpdate::Remove;
    Update.mProxy.reset();
    Update.mTransform = {};
}

void URenderSubsystem::BuildSceneUpdates(FSceneUpdateBatch& Updates) {
    FlushDeferredRenderUpdates();
    FinishPrimitiveUpdates(Updates.mRenderData);
    Updates.mRenderData.mObjectUpdates.clear();
    Updates.mPrimitiveUpdates.clear();
    Updates.mPrimitiveUpdates.swap(mPrimitiveUpdates);
    mPrimitiveUpdateIndices.clear();
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

void URenderSubsystem::BuildRenderProbes(FSceneRenderData& Scene) {
    FlushDeferredRenderUpdates();
    FinishPrimitiveUpdates(Scene);
    Scene.mObjectUpdates.clear();
    Scene.mObjectUpdates.reserve(mPrimitiveUpdates.size());

    for (const FPrimitiveSceneUpdate& PrimitiveUpdate : mPrimitiveUpdates) {
        FRenderObjectUpdate Update{};

        Update.mComponentHandle = PrimitiveUpdate.mComponentHandle;
        Update.mRemoved = PrimitiveUpdate.mType == EPrimitiveSceneUpdate::Remove;

        if (PrimitiveUpdate.mProxy != nullptr) {
            PrimitiveUpdate.mProxy->BuildLegacyProbe(Update.mProbe);
        } else if (!Update.mRemoved) {
            const auto Position{mComponentIndices.find(GetComponentKey(Update.mComponentHandle))};
            const UStaticMeshComponent* Component{Position != mComponentIndices.end() ? mComponents[Position->second] : nullptr};
            const std::unique_ptr<FPrimitiveSceneProxy> Proxy{Component != nullptr ? Component->CreateSceneProxy() : nullptr};

            Update.mRemoved = Proxy == nullptr;

            if (Proxy != nullptr) {
                Proxy->BuildLegacyProbe(Update.mProbe);
            }
        }

        Scene.mObjectUpdates.push_back(Update);
    }

    mPrimitiveUpdates.clear();
    mPrimitiveUpdateIndices.clear();
}

void URenderSubsystem::FinishPrimitiveUpdates(FSceneRenderData& Scene) {
    if (!mPrimitiveUpdates.empty()) {
        ++mRevision;
    }

    Scene.mSceneId = mSceneId;
    Scene.mRevision = mRevision;
}

bool URenderSubsystem::ContainsComponent(const UStaticMeshComponent* Component) const {
    if (Component == nullptr) {
        return false;
    }

    const auto Position{mComponentIndices.find(GetComponentKey(Component->GetHandle()))};

    return Position != mComponentIndices.end() && mComponents[Position->second] == Component;
}

const TArray<UStaticMeshComponent*>& URenderSubsystem::GetRegisteredComponents() const {
    return mComponents;
}

Uint64 URenderSubsystem::GetSceneId() const {
    return mSceneId;
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

    mSceneId = AllocateRenderSceneId();
    mRevision = {};
}
